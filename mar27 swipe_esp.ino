#include <Adafruit_LSM9DS1.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Nordic UART Service UUIDs (standard for BLE serial)
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define TX_CHAR_UUID        "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1();
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) { deviceConnected = true; }
  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    pServer->getAdvertising()->start(); // restart advertising
  }
};

void setup() {
  Serial.begin(115200);

  if (!lsm.begin()) {
    Serial.println("LSM9DS1 Sensor Not Found!");
    while (1);
  }
  lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_2G);
  lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_245DPS);

  // BLE setup
  BLEDevice::init("ESP32_Sensor");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(
    TX_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY
  );
  pTxCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("BLE advertising started...");
}

void loop() {
  lsm.read();
  sensors_event_t a, m, g, temp;
  lsm.getEvent(&a, &m, &g, &temp);

  char buf[80];
  snprintf(buf, sizeof(buf), "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
    a.acceleration.x, a.acceleration.y, a.acceleration.z,
    g.gyro.x, g.gyro.y, g.gyro.z);

  // Always send over USB Serial
  Serial.println(buf);

  // Send over BLE if connected
  if (deviceConnected) {
    pTxCharacteristic->setValue((uint8_t *)buf, strlen(buf));
    pTxCharacteristic->notify();
  }

  delay(50);
}
