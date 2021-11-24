#include <ArduinoBLE.h>
#include <Arduino_LSM9DS1.h>

BLEService gyroService("5aaeb650-c2cb-44d1-b4ab-7144e08aed2e"); //Declare service for Gyroscope
BLEFloatCharacteristic gyroscopeValuesChar("2101", BLERead | BLENotify);

const unsigned long culInterval = 2000;
unsigned long ulPreviousTime = 0;

void setup()
{

  Serial.begin(9600); //BAUD rate fixed at 9600 Hz
  while (!Serial);    //Wait until serial connection is estabilished
  pinMode(LED_BUILTIN, OUTPUT); //Intializes the built in LED to indicate when a central device has connected
  
  if (!BLE.begin()) //Wait for BLE initialization
  {
    Serial.println("BLE failed");
    while (1);
  }
  
  if (!IMU.begin()) //Wait for IMU initialization
  {
    Serial.println("Failed initializing IMU");
    while (1);
  }
  
  BLE.setLocalName("Gyroscope");                       
  BLE.setAdvertisedService(gyroService);                       
  gyroService.addCharacteristic(gyroscopeValuesChar);   //Adds the gryoscope characteristics 
  BLE.addService(gyroService);                          //Adds the gyroscope service 
    
  BLE.advertise();                                      //Starts advertising the peripheral device over bluetooth
  Serial.println("Waiting for connection..");
}

void loop() 
{
  BLEDevice central = BLE.central();                    //Waits for BLE Central device to connect
  unsigned long ulTimer = millis();
  if (central)                                          
  {
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    digitalWrite(LED_BUILTIN, HIGH);                    //Turn on peripheral LED to indicate valid connection with Central Device

    while (central.connected()) {                       //While the Peripheral Device is connected to the Central Device
      float x,y,z;                                      //Declare variables to hold gyroscope values 
      if(IMU.gyroscopeAvailable()) {                    //If the gyroscope sensor is available, read the values into variables.
        IMU.readGyroscope(x,y,z);
         Serial.print(x);
         Serial.print('\t');
         Serial.print(y);
         Serial.print('\t');
         Serial.println(z);
      }
      gyroscopeValuesChar.writeValue(x);  
      delay(1000);

    }
  }
  else
  {
    if (ulTimer > (ulPreviousTime + culInterval))
    {
      ulPreviousTime = ulTimer;
      Serial.println("Disconnected from central");
    }
  }
}
