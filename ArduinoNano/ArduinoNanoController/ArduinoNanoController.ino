#include <ArduinoBLE.h>
#include <Arduino_LSM9DS1.h>

#define BLE_UUID_DATA_SERVICE                 "5aaeb650-c2cb-44d1-b4ab-7144e08aed2e"
#define BLE_UUID_GYROSCOPE_CHARACTERISTIC     "9936153d-65bc-4479-b079-aa25569f9ab1"
#define BLE_UUID_ACCELEROMETER_CHARACTERISTIC "f4055745-6f5a-4e2b-8433-2704337cc3b5"

BLEService dataService(BLE_UUID_DATA_SERVICE);

BLEFloatCharacteristic gyroscopeValuesCharacteristic(BLE_UUID_GYROSCOPE_CHARACTERISTIC, BLERead | BLENotify);
BLEFloatCharacteristic accelerometerValuesCharacteristic(BLE_UUID_ACCELEROMETER_CHARACTERISTIC, BLERead | BLENotify);


//------------------------------------------------------------------------------
/**
 * Initialize the serial and wait till it's connected
 */
void initializeSerial(const unsigned int &iBaudRate)
{
  Serial.begin(iBaudRate);
  while (!Serial);
}
//------------------------------------------------------------------------------
/**
 * Initialize the BLE module, if initialization fails the program will be stuck here
 */
void initializeBLE(void)
{
  if (!BLE.begin())
  {
    Serial.println("BLE failed");
    while (1);
  }
}
//------------------------------------------------------------------------------
/**
 * Initialize the IMU module, if initialization fails the program will be stuck here
 */
 void initializeIMU(void)
{
  if (!IMU.begin())
  {
    Serial.println("Failed initializing IMU");
    while (1);
  }
}
//------------------------------------------------------------------------------
/**
 * Set the local name, add the data service and the accelerometer and gyro
 * characteristics, start the BLE advertisement
 */
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
/**
 * Check if the central is connected. If so, gets the data from the accelerometer
 * and the gyroscope
 * Until the central is disconnected, the program will loop in this function and
 * continue to retrieve and send the sensors data
 */
bool getSensorValues(void)
{
  BLEDevice central = BLE.central();

  if (central)                                          
  {
    while (central.connected()) 
    {
      getGyroscopeValues();
      getAccelerometerValues();
      delay(3000);                  
    }
  }  
}
//------------------------------------------------------------------------------
/**
 * If gyroscope is avaiable, get values from it, print them and send them via BLE
 */
void getGyroscopeValues(void)
{
  float x, y, z;

  if (IMU.gyroscopeAvailable()) 
  {
    IMU.readGyroscope(x, y, z);
    gyroscopeValuesCharacteristic.writeValue(x);
    gyroscopeValuesCharacteristic.writeValue(y);
    gyroscopeValuesCharacteristic.writeValue(z);
    Serial.print("Gyro data:\n\t");
    Serial.print(x);
    Serial.print('\t');
    Serial.print(y);
    Serial.print('\t');
    Serial.println(z);
  } 
}
//------------------------------------------------------------------------------
/**
 * If accelerometer is avaiable, get values from it, print them and send them via BLE
 */
void getAccelerometerValues(void)
{
  float x, y, z;
  
  if (IMU.accelerationAvailable()) 
  {
    IMU.readAcceleration(x, y, z);
    accelerometerValuesCharacteristic.writeValue(x);
    accelerometerValuesCharacteristic.writeValue(y);
    accelerometerValuesCharacteristic.writeValue(z);
    Serial.print("Accelerometer data:\n\t");
    Serial.print(x);
    Serial.print('\t');
    Serial.print(y);
    Serial.print('\t');
    Serial.println(z);
  }
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
void loop(void) 
{
  getSensorValues();
}
//------------------------------------------------------------------------------
