#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEServer.h"
#include "esp32-hal-adc.h"

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define ADC 32
#define CTRL 25
int cnt = 1;
bool overCurrent = false;

BLECharacteristic* pCharacteristic = NULL;

bool deviceConnected = false;
uint32_t value = 0;

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
  pinMode(ADC, INPUT);
  pinMode(CTRL, OUTPUT);
  dacWrite(CTRL, 0);
  BLEDevice::init("ESP32-BLE");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID); 
  pCharacteristic = pService->createCharacteristic(
                                                  CHARACTERISTIC_UUID, 
                                                  BLECharacteristic::PROPERTY_NOTIFY | // 启用通知
                                                  BLECharacteristic::PROPERTY_INDICATE // 启用通知
                                                  );
  pServer->setCallbacks(new MyServerCallbacks());
  pService->start(); 

  BLEDevice::startAdvertising();
}

void improve(double* vol, double x, double imp){
  if(*vol >= x) *vol -= imp;
}

double analogReadVoltage(int pin)
{
  int adc = 0;
  for(int i = 0; i < 100; i++){
    adc += analogReadMilliVolts(pin);
    delay(5);
  }
  adc /= 50;
  double voltage = (0.9997453062454001 * adc - 0.24662574992543795);
  if(voltage > 2850) voltage = 0.9903424719828523 * adc - 40.810851933039466;
  Serial.printf("%.2lfV    %.6lfV    %d\n", voltage, voltage, adc);
  return voltage;
}


void loop() {
  double voltage = analogReadVoltage(ADC);
  int current = voltage / 2;
  if (current > 570 || overCurrent) {
    dacWrite(CTRL, 255);
    overCurrent = true; 
  }

  
    
  if (deviceConnected) {
    if(overCurrent)
      pCharacteristic->setValue("Over Current! Press RST to reset.");
    else
      pCharacteristic->setValue((String("Cur:") + current + "mA Volt:" + voltage + "mV").c_str());
    pCharacteristic->notify();
  }

  if (overCurrent)
    Serial.println("Over Current! Press RST to reset.");
  else
    Serial.printf("Cur:%dmA \n", current);
  delay(1000);
}
