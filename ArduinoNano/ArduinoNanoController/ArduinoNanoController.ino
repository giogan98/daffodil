#include <ArduinoBLE.h>
#include <Arduino_LSM9DS1.h>

#define BLE_UUID_DATA_SERVICE                 "5aaeb650-c2cb-44d1-b4ab-7144e08aed2e"
#define BLE_UUID_GYROSCOPE_CHARACTERISTIC     "9936153d-65bc-4479-b079-aa25569f9ab1"
#define BLE_UUID_ACCELEROMETER_CHARACTERISTIC "f4055745-6f5a-4e2b-8433-2704337cc3b5"

BLEService dataService(BLE_UUID_DATA_SERVICE);

BLEFloatCharacteristic gyroscopeValuesCharacteristic(BLE_UUID_GYROSCOPE_CHARACTERISTIC, BLERead | BLENotify);
BLEFloatCharacteristic accelerometerValuesCharacteristic(BLE_UUID_ACCELEROMETER_CHARACTERISTIC, BLERead | BLENotify);

unsigned long ulPreviousTime = 0;
const unsigned long ulInterval = 2000;

//------------------------------------------------------------------------------
void initializeSerial(const unsigned int &iBaudRate)
{
  Serial.begin(iBaudRate);
  while (!Serial);
}
//------------------------------------------------------------------------------
void initializeBLE(void)
{
  if (!BLE.begin())
  {
    Serial.println("BLE failed");
    while (1);
  }
}
//------------------------------------------------------------------------------
void initializeIMU(void)
{
  if (!IMU.begin())
  {
    Serial.println("Failed initializing IMU");
    while (1);
  }
}
//------------------------------------------------------------------------------
void setupBLE(void)
{
  BLE.setLocalName("SenseBLE");
  
  BLE.setAdvertisedService(dataService);  
                           
  dataService.addCharacteristic(gyroscopeValuesCharacteristic);
  dataService.addCharacteristic(accelerometerValuesCharacteristic);
  
  BLE.addService(dataService);
     
  BLE.advertise();
}
//------------------------------------------------------------------------------
void setup(void)
{
  initializeSerial(9600);
  initializeBLE();
  initializeIMU();
  setupBLE();
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("Waiting for connection..");
}
//------------------------------------------------------------------------------
bool getSensorValues(void)
{
  BLEDevice central = BLE.central();

  if (central)                                          
  {
    while (central.connected()) 
    {
      getGyroscopeValues();
      getAccelerometerValues();
      delay(1000);                  
    }
  }  
}
//------------------------------------------------------------------------------
void getGyroscopeValues(void)
{
  float x, y, z;

  if (IMU.gyroscopeAvailable()) 
  {
    IMU.readGyroscope(x, y, z);
    Serial.print("Gyro data:\n\t");
    Serial.print(x);
    Serial.print('\t');
    Serial.print(y);
    Serial.print('\t');
    Serial.println(z);
    gyroscopeValuesCharacteristic.writeValue(x);
    gyroscopeValuesCharacteristic.writeValue(y);
    gyroscopeValuesCharacteristic.writeValue(z);
  } 
}
//------------------------------------------------------------------------------
void getAccelerometerValues(void)
{
  float x, y, z;
  
  if (IMU.accelerationAvailable()) 
  {
    IMU.readAcceleration(x, y, z);
    Serial.print("Accelerometer data:\n\t");
    Serial.print(x);
    Serial.print('\t');
    Serial.print(y);
    Serial.print('\t');
    Serial.println(z);
    accelerometerValuesCharacteristic.writeValue(x);
    accelerometerValuesCharacteristic.writeValue(y);
    accelerometerValuesCharacteristic.writeValue(z);
  }
}
//------------------------------------------------------------------------------
void loop(void) 
{
  unsigned long ulTimer = millis();
  if (ulTimer > (ulPreviousTime + ulInterval))
  {
    ulPreviousTime = ulTimer;
    getSensorValues();
  }
}
//------------------------------------------------------------------------------