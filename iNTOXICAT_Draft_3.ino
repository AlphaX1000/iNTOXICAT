#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
//Initialise all the Pins and data type here
const int Analog_channel_pin= 2;
uint32_t BAC = 0;
uint32_t Null = 0;
int ADC_VALUE = 0;
float voltage =0;
float Rs =0;


//Custom UUID Generated from uuidgenerator.net

#define SERVICE_UUID        "cdcd6602-bc18-44fd-9c5e-f33d8176569c"
#define CHARACTERISTIC_UUID "9cfb6dc0-89f7-4176-bed5-0f237a0f5c2d"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};



void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("iNTOXICAT ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");

}

void loop() {
    ADC_VALUE = analogRead(Analog_channel_pin);
    voltage = (ADC_VALUE * 3.3) / (4095);
    Rs = ((5/2*voltage)-1)*2;
    
    //PPM =306.756/((Rs)^1.6168)
    //Actual BAC = PPM *1.06*10^-4
    //BAC =Actual BAC*10^3

    BAC = 1.0838*(pow(Rs,1.6168));
    
    Serial.print("BAC = ");
    Serial.print(BAC);
    Serial.print("% g/dL");
    Serial.print(" \n\n");
    delay(100);
    // notify changed value
    if (deviceConnected) {
        pCharacteristic->setValue((uint8_t*)&BAC, 4);
        pCharacteristic->notify();
        delay(3); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }

}
