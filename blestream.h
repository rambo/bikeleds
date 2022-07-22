#ifndef BLESTREAM_H
#define BLESTREAM_H
#include <Arduino.h>
#include <Stream.h>
#include <string> // for string class
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


class BLEStream: public Stream {
    private:
        std::string inbuffer;
    public:
        BLEStream();
        ~BLEStream();

        // Stream implementation
        int read();
        int available();
        int peek();

    // Print implementation
    virtual size_t write(uint8_t val);
    virtual size_t write(uint8_t *buf, size_t size);
    using Print::write; // pull in write(str) and write(buf, size) from Print
    virtual void flush();


};

size_t BLEStream::write(uint8_t val)
{
  pTxCharacteristic->setValue(&val, 1);
  pTxCharacteristic->notify();
  return 1;
}

size_t BLEStream::write(uint8_t *buf, size_t bufsize)
{
  pTxCharacteristic->setValue(buf, bufsize);
  pTxCharacteristic->notify();
  return 0;
}

void BLEStream::flush()
{
}

BLEStream::BLEStream(){
  inbuffer = std::string();
}

BLEStream::~BLEStream(){
  inbuffer.clear();
}

// Stream implementation
int BLEStream::read() {
  if (inbuffer.length() < 1)
  {
     return -1;
  }
  int ret = int(inbuffer[0]);
  inbuffer.erase(0,1);
  return ret;
}

int BLEStream::available() {
  return inbuffer.length();
}

int BLEStream::peek() {
  if (inbuffer.length() < 1)
  {
     return -1;
  }
  return int(inbuffer[0]);
};



class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      GLOBAL_BT_connected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      GLOBAL_BT_connected = false;
      pServer->startAdvertising(); // restart advertising
    }
};

BLEStream* GLOBAL_BLE_Stream;

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      GLOBAL_idle_timer = 0;
      std::string rxValue = pCharacteristic->getValue();
      GLOBAL_BLE_Stream->write(rxValue.c_str() ,rxValue.length());
    }
};


void ble_init() {
  GLOBAL_BLE_Stream = new BLEStream();
  // Create the BLE Device
  BLEDevice::init("UART Service");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                       CHARACTERISTIC_UUID_RX,
                      BLECharacteristic::PROPERTY_WRITE
                    );

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
}


#endif
