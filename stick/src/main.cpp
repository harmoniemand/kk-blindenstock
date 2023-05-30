#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEEddystoneURL.h>
#include <BLEEddystoneTLM.h>
#include <BLEBeacon.h>

#define UUID "1337bd0c-89c4-4318-ad62-10778bd06b95"
#define PIN_VIBRATOR 4

int scanTime = 2; // In seconds
BLEScan *pBLEScan;
BLEUUID uuid(uuid);
TaskHandle_t Task2;
int vibration_delay = 500;
bool enable_vibration = false;

QueueHandle_t queue_vib_delay;
QueueHandle_t queue_vib_enable;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {

    if (advertisedDevice.haveManufacturerData() == true)
    {
      std::string strManufacturerData = advertisedDevice.getManufacturerData();

      uint8_t cManufacturerData[100];
      strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);

      if (strManufacturerData.length() == 25 && cManufacturerData[0] == 0x4C && cManufacturerData[1] == 0x00)
      {
        // Serial.println("Found an iBeacon!");

        BLEBeacon oBeacon = BLEBeacon();
        oBeacon.setData(strManufacturerData);
        // Serial.printf("iBeacon Frame\n");
        // Serial.printf("ID: %04X Major: %d Minor: %d UUID: %s Power: %d\n", oBeacon.getManufacturerId(), oBeacon.getMajor(), oBeacon.getMinor(), oBeacon.getProximityUUID().toString().c_str(), oBeacon.getSignalPower());

        String proxUuid = oBeacon.getProximityUUID().toString().c_str();

        if (proxUuid.equals(UUID))
        {
          // Serial.println("Found our trafic light!");
          // Serial.print("RSSI: ");
          int rssi = advertisedDevice.getRSSI();
          Serial.println(rssi);

          if (rssi > -70)
          {
            Serial.println("You are close to the trafic light!");
            enable_vibration = true;
            vibration_delay = 300;
            xQueueSend(queue_vib_delay, &vibration_delay, portMAX_DELAY);
            xQueueSend(queue_vib_enable, &enable_vibration, portMAX_DELAY);
          }
          else if (rssi > -80)
          {
            Serial.println("You are near to the trafic light!");
            enable_vibration = true;
            vibration_delay = 100;
          }
          else
          {
            Serial.println("You are far from the trafic light!");
            enable_vibration = false;
          }
        }
      }
    }
  }
};

void Task2code(void *pvParameters)
{
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());

  for (;;)
  {
    int del;
    bool en;
    xQueueReceive(queue_vib_delay, &del, portMAX_DELAY);
    xQueueReceive(queue_vib_enable, &en, portMAX_DELAY);

    if (en)
    {
      Serial.println("Vibrating...");
      digitalWrite(PIN_VIBRATOR, HIGH);
      delay(del);
      digitalWrite(PIN_VIBRATOR, LOW);
      delay(del);
    }
  }
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Scanning...");
  pinMode(PIN_VIBRATOR, OUTPUT);

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); // create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // less or equal setInterval value

  queue_vib_delay = xQueueCreate(1, sizeof(int));
  queue_vib_enable = xQueueCreate(1, sizeof(bool));

  // create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
      Task2code, /* Task function. */
      "Task2",   /* name of task. */
      10000,     /* Stack size of task */
      NULL,      /* parameter of the task */
      1,         /* priority of the task */
      &Task2,    /* Task handle to keep track of created task */
      1);        /* pin task to core 1 */

  delay(1000);
}

bool CheckForUUID(BLEScanResults devices, BLEUUID uuid)
{
  for (int i = 0; i < devices.getCount(); i++)
  {
    BLEAdvertisedDevice device = devices.getDevice(i);

    if (device.getServiceUUID().equals(uuid))
    {
      return true;
    }
    return false;
  }
}

void loop()
{
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
  delay(100);
}
