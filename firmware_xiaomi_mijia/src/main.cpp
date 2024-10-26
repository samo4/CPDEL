#include <Arduino.h>
#include <NimBLEDevice.h>

BLEClient *pClient;

BLEScan *pBLEScan;

#define SCAN_TIME 10 // seconds

bool connected = false;

#undef CONFIG_BTC_TASK_STACK_SIZE
#define CONFIG_BTC_TASK_STACK_SIZE 32768

// The remote service we wish to connect to.
static BLEUUID serviceUUID("ebe0ccb0-7a0a-4b0c-8a1a-6ff2997da3a6");
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID("ebe0ccc1-7a0a-4b0c-8a1a-6ff2997da3a6");

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *pclient) {
    connected = true;
    Serial.printf(" * Connected %s\n", pclient->getPeerAddress().toString().c_str());
  }

  void onDisconnect(BLEClient *pclient) {
    connected = false;
    Serial.printf(" * Disconnected %s\n", pclient->getPeerAddress().toString().c_str());
  }
};

static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
  float temp;
  float humi;
  float voltage;
  Serial.print(" + Notify callback for characteristic ");
  Serial.println(pBLERemoteCharacteristic->getUUID().toString().c_str());
  temp = (pData[0] | (pData[1] << 8)) * 0.01; // little endian
  humi = pData[2];
  voltage = (pData[3] | (pData[4] << 8)) * 0.001; // little endian
  Serial.printf("temp = %.1f C ; humidity = %.1f %% ; voltage = %.3f V\n", temp, humi, voltage);
  pClient->disconnect();
}

void registerNotification() {
  if (!connected) {
    Serial.println(" - Premature disconnection");
    return;
  }
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print(" - Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
  }
  Serial.println(" + Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  BLERemoteCharacteristic *pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print(" - Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
  }
  Serial.println(" + Found our characteristic");
  pRemoteCharacteristic->registerForNotify(notifyCallback);
}

void createBleClientWithCallbacks() {
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
}

void connectSensor(BLEAddress htSensorAddress) { pClient->connect(htSensorAddress); }

void setup() {
  Serial.begin(115200);
  Serial.println("+ Starting MJ client...");
  delay(500);

  BLEDevice::init("ESP32");
  createBleClientWithCallbacks();

  pBLEScan = BLEDevice::getScan(); // create new scan
  pBLEScan->setActiveScan(true);   // active scan uses more power, but get results faster
  pBLEScan->setInterval(0x50);
  pBLEScan->setWindow(0x30);
}

std::string addresses[10];

int addressCount = 0;

void loop() {
  delay(500);
  BLEScanResults foundDevices = pBLEScan->start(SCAN_TIME);
  int count = foundDevices.getCount();
  Serial.printf("+ Found device count : %d\n", count);
  for (int i = 0; i < count; i++) {
    BLEAdvertisedDevice b = foundDevices.getDevice(i);
    Serial.println(b.toString().c_str());
    if (!b.getName().compare("LYWSD03MMC")) {
      BLEAddress addr = b.getAddress();
      addresses[addressCount] = addr.toString();
      addressCount++;
    }
  }
  for (int i = 0; i < addressCount; i++) {
    std::string curAddr = addresses[i];
    bool found = false;
    for (int j = 0; j < count; j++) {
      if (foundDevices.getDevice(j).getAddress().equals(BLEAddress(curAddr))) {
        found = true;
      }
    }
    if (!found) {
      Serial.printf("* Remove offline address : %s\n", curAddr.c_str());
      for (int j = addressCount; j > max(i, 1); j--) {
        addresses[j - 1] = addresses[j];
      }
      continue;
    }
    Serial.printf("+ Connect : %s\n", curAddr.c_str());
    connectSensor(BLEAddress(curAddr));
    registerNotification();
    while (connected) {
      delay(10);
    };
  }

  delay(500);
}
