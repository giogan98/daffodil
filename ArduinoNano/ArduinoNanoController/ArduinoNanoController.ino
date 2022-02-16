#include <ArduinoBLE.h>
#include <Arduino_LSM9DS1.h>

#define BLE_UUID_DATA_SERVICE                   "5aaeb650-c2cb-44d1-b4ab-7144e08aed2e"

#define BLE_UUID_ACCELEROMETER_CHARACTERISTIC_X "f4055745-6f5a-4e2b-8433-2704337cc3b5"
#define BLE_UUID_ACCELEROMETER_CHARACTERISTIC_Y "ac7a390c-abb3-48c8-8c54-73c3a6a4bc73"
#define BLE_UUID_ACCELEROMETER_CHARACTERISTIC_Z "3c71aaec-128b-4f88-bb60-28a2026498be"

#define BLE_UUID_GYROSCOPE_CHARACTERISTIC_X     "9936153d-65bc-4479-b079-aa25569f9ab1"
#define BLE_UUID_GYROSCOPE_CHARACTERISTIC_Y     "ef1cdf9d-56fd-437d-8c4e-3a77cf8c8265"
#define BLE_UUID_GYROSCOPE_CHARACTERISTIC_Z     "69911b28-ffcc-4a65-85de-1b501f7a5e40"

BLEService dataService(BLE_UUID_DATA_SERVICE);

BLEFloatCharacteristic accelerometerCharacteristicX(BLE_UUID_ACCELEROMETER_CHARACTERISTIC_X, BLERead | BLENotify);
BLEFloatCharacteristic accelerometerCharacteristicY(BLE_UUID_ACCELEROMETER_CHARACTERISTIC_Y, BLERead | BLENotify);
BLEFloatCharacteristic accelerometerCharacteristicZ(BLE_UUID_ACCELEROMETER_CHARACTERISTIC_Z, BLERead | BLENotify);

BLEFloatCharacteristic gyroscopeCharacteristicX(BLE_UUID_GYROSCOPE_CHARACTERISTIC_X, BLERead | BLENotify);
BLEFloatCharacteristic gyroscopeCharacteristicY(BLE_UUID_GYROSCOPE_CHARACTERISTIC_Y, BLERead | BLENotify);
BLEFloatCharacteristic gyroscopeCharacteristicZ(BLE_UUID_GYROSCOPE_CHARACTERISTIC_Z, BLERead | BLENotify);

//------------------------------------------------------------------------------
/**
 * Initialize the serial, which is optionale to use
 */
void initializeSerial(const unsigned int &iBaudRate)
{
  Serial.begin(iBaudRate);
}
//------------------------------------------------------------------------------
/**
 * Initialize the BLE module. If initialization fails, the program will be stuck 
 * here because the BLE module is essential
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
 * Initialize the IMU module. If initialization fails, the program will be stuck 
 * here because the IMU module is essential
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
 * Set the local name, add the data service and the accelerometer and gyroscope
 * characteristics, start the BLE advertisement
 */
void setupBLE(void)
{
  BLE.setLocalName("SenseBLE");
  
  BLE.setAdvertisedService(dataService);  

  dataService.addCharacteristic(accelerometerCharacteristicX);
  dataService.addCharacteristic(accelerometerCharacteristicY);
  dataService.addCharacteristic(accelerometerCharacteristicZ);

  dataService.addCharacteristic(gyroscopeCharacteristicX);
  dataService.addCharacteristic(gyroscopeCharacteristicY);
  dataService.addCharacteristic(gyroscopeCharacteristicZ);

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
bool sendSensorsValues(void)
{
  BLEDevice central = BLE.central();

  if (central)                                          
  {
    while (central.connected()) 
    {
      sendAccelerometerValues();
      sendGyroscopeValues();
      delay(3000);                  
    }
  }  
}
//------------------------------------------------------------------------------
/**
 * If accelerometer is avaiable, get values from it, print them and send them via BLE
 */
void sendAccelerometerValues(void)
{
  char buffer[100];
  float x, y, z;
  
  if (IMU.accelerationAvailable()) 
  {
    IMU.readAcceleration(x, y, z);

    accelerometerCharacteristicX.writeValue(x);
    accelerometerCharacteristicY.writeValue(y);
    accelerometerCharacteristicZ.writeValue(z);

    sprintf(buffer, "Accelerometer data: \n %f \t %f \t %f \n", x, y, z);
    Serial.print(buffer);
  }
}
//------------------------------------------------------------------------------
/**
 * If gyroscope is avaiable, get values from it, print them and send them via BLE
 */
void sendGyroscopeValues(void)
{
  char buffer[100];
  float x, y, z;

  if (IMU.gyroscopeAvailable()) 
  {
    IMU.readGyroscope(x, y, z);

    gyroscopeCharacteristicX.writeValue(x);
    gyroscopeCharacteristicY.writeValue(y);
    gyroscopeCharacteristicZ.writeValue(z);

    sprintf(buffer, "Gyroscope data: \n %f \t %f \t %f \n", x, y, z);
    Serial.print(buffer);
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
  sendSensorsValues();
}
//------------------------------------------------------------------------------
