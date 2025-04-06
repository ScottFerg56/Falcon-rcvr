#include "OMObject.h"
#include "Debug.h"
#include "Lights.h"
#include "Rectenna.h"
#include "Sounds.h"
#include "Ramp.h"
#include <esp_now.h>
#include <WiFi.h>
#include "SPIFFS.h"

Root root;

// #include <nvs_flash.h>

// our Mac Address = {0xE8, 0x9F, 0x6D, 0x20, 0x7D, 0x28}

uint8_t peerMacAddress[] = {0xD8, 0x3B, 0xDA, 0x87, 0x52, 0x58};

esp_now_peer_info_t PeerInfo;
bool DataSent = false;
bool DataConnected = false;

struct FilePacketHdr
{
    char        tag;
    uint32_t    packetNum;
};

uint32_t FilePacketCount = 0;
uint32_t FilePacketNumber = 0;
String FilePath;
bool FileXferComplete = false;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    DataSent = false;
    if (status != ESP_NOW_SEND_SUCCESS)
    {
        flogw("Delivery Fail");
        DataConnected = false;
    }
}

void SendCmd(String cmd);

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *pData, int len)
{
    DataConnected = true;
    if (pData[0] == '1')
    {
        FilePacketHdr hdr;
        memcpy(&hdr, pData, sizeof(hdr));
        FilePacketNumber = 1;
        FilePacketCount = hdr.packetNum;
        FilePath = String((char*)pData + sizeof(hdr));
        flogv("Starting transfer: %s  #packets: %lu", FilePath.c_str(), FilePacketCount);
        SPIFFS.remove(("/" + FilePath).c_str());
        SendCmd("2");
    }
    else if (pData[0] == '2')
    {
        FilePacketHdr hdr;
        memcpy(&hdr, pData, sizeof(hdr));
        uint32_t packetNumber = hdr.packetNum;
        if (packetNumber != FilePacketNumber)
        {
            floge("packet number %lu doesn't match expected %lu", packetNumber, FilePacketNumber);
            FilePacketNumber = 0;
            FilePacketCount = 0;
            FilePath = "";
            SendCmd("3");   // terminate transfer
            return;
        }
        //Serial.println("chunk NUMBER = " + String(currentTransmitCurrentPosition));
        File file = SPIFFS.open(("/" + FilePath).c_str(), FILE_APPEND);
        if (!file)
        {
            floge("Error opening file for append");
            FilePacketNumber = 0;
            FilePacketCount = 0;
            FilePath = "";
            SendCmd("3");   // terminate transfer
            return;
        }
        file.write(pData + sizeof(hdr), len - sizeof(hdr));
        uint32_t fileSize = file.size();
        file.close();
        // flogv("File packet %lu of %lu received, file size: %lu", FilePacketNumber, FilePacketCount, fileSize);
  
        if (FilePacketNumber == FilePacketCount)
        {
            FileXferComplete = true;
            FilePacketNumber = 0;
            FilePacketCount = 0;
            flogv("File transfer complete");
        }
        else
        {
            ++FilePacketNumber;
        }
        SendCmd("2");   // ACK the packet
    }
    else
    {
        String cmd(pData, len);
        root.Command(cmd);
    }
}

bool SendData(const uint8_t *pData, int len)
{
    if (DataSent)
    {
        // data sent but not acknowledged; wait it out a while
        unsigned long ms = millis();
        while (DataSent)
        {
            if (millis() - ms > 100)
                DataSent = false;
        }
    }
    DataSent = true;
    esp_err_t result = esp_now_send(PeerInfo.peer_addr, pData, len);
    if (result != ESP_OK)
    {
        DataConnected = false;   // don't get caught up in infinite logging loop!!
        floge("Error sending data: %s", esp_err_to_name(result));
        DataSent = false;
        return false;
    }
    return true;
}

void SendCmd(String cmd)
{
    SendData((uint8_t*)cmd.c_str(), cmd.length());
}

int flog_printer(const char* s)
{
    int len = Serial.print(s);
    if (DataConnected)
    {
        // echo the message only if the esp_now connection is active
        len = strlen(s);
        SendData((uint8_t*)s, len);
    }
    return len;
}

void setup()
{
    // nvs_flash_erase();      // erase the NVS partition and...
    // nvs_flash_init();       // initialize the NVS partition.
    // while (true);
    Serial.begin(115200);
    delay(2000);
    FLogger::setLogLevel(FLOG_VERBOSE);
    // FLogger::setPrinter(flog_printer);
    root.AddObject(new Debug());
    root.AddObject(new Sounds());
    // I2S audio initialization seems to mess with PIN_NEOPIXEL (pin 0).
    // So we need to setup Sounds first BEFORE Lights setup fixes pin 0!
    // Using that order, both seem to work properly.
    root.AddObject(new Lights());
    root.AddObject(new Ramp());
    root.AddObject(new Rectenna());

    root.SetSend([](String cmd) { SendCmd(cmd); });   // { Serial.println(cmd.c_str()); });

    flogi("WIFI init");
    if (!WiFi.mode(WIFI_STA))
        flogf("%s FAILED", "WIFI init");

    flogi("MAC addr: %s", WiFi.macAddress().c_str());

    flogi("ESP_NOW init");
    if (esp_now_init() != ESP_OK)
        flogf("%s FAILED", "ESP_NOW init");

    //flogi("ESP_NOW peer add");
    memset(&PeerInfo, 0, sizeof(PeerInfo));
    memcpy(PeerInfo.peer_addr, peerMacAddress, sizeof(PeerInfo.peer_addr));
    //PeerInfo.channel = 0;
    //PeerInfo.encrypt = false;
    if (esp_now_add_peer(&PeerInfo) != ESP_OK)
        flogf("%s FAILED", "ESP_NOW peer add");
    
    //flogi("ESP_NOW send cb");
    esp_now_register_send_cb(OnDataSent);

    //flogi("ESP_NOW recv cb");
    esp_now_register_recv_cb(OnDataRecv);

    flogi("ESP_NOW init complete");

    root.Setup();
    flogv("Setup done");
}

void loop()
{
    root.Run();
    if (FileXferComplete)
    {
        FileXferComplete = false;
        // flogv("File transfer complete");
        ((Sounds*)root.GetObject('s'))->Play(FilePath);
        FilePath = "";
    }
}
