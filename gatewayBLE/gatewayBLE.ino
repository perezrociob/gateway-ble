//Libraries
#include "WiFi.h"
#include "BLEDevice.h"
#include <PubSubClient.h>

//////WiFi Credentials
#define WIFI_SSID ""
#define WIFI_PASS ""


//////MQTT Credentials
const char* mqttServer = "";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";

////// BLE UUIDs (HM-10)
static BLEUUID serviceUUID("0000FFE0-0000-1000-8000-00805F9B34FB");
static BLEUUID    charUUID("0000FFE1-0000-1000-8000-00805F9B34FB");

WiFiClient clientWiFi;
PubSubClient client(clientWiFi);

static boolean doConnect = false;
static boolean connected = false;

static boolean resultsOK = false;

static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEScan* pBLEScan;
static BLEClient *pClient;
static BLEAdvertisedDevice *myDevice;

float valueVout = -99.0;

void mqttConect(){
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("", mqttUser, mqttPassword )) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(1000);
    }
  }
}

int volt2hum(float v){
float humArray[11]={0.0,7.1,14.6,22.6,31.1,40.3,50.2,60.9,72.6,85.6,100.0};
float voutArray[11]={2.3,2.2,2.1,2.0,1.9,1.8,1.7,1.6,1.5,1.4,1.3};
int pos;
boolean found=false;
float h;
    pos=0;
    while ((pos<10)&&(v<voutArray[pos+1])){
      pos++;
    }
    h=((humArray[pos+1]-humArray[pos])/(voutArray[pos+1]-voutArray[pos]))*(v-voutArray[pos]) + humArray[pos];
    Serial.print(" ");
    Serial.print(h);
    return round(h);
}

// HM-10 Notifications
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  char valueNot[]={((char*)pData)[8], ((char*)pData)[9], ((char*)pData)[10], ((char*)pData)[11],'\0'};
  valueVout =std::atof(valueNot);
  Serial.print(valueNot);
  pData=0; //clear
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
  }
};

// When the searched server (HM-10) is found, make the connection with the client (ESP32).
bool connectToServer() {
  if (pClient != nullptr){
    delete pClient;
  }
  pClient = BLEDevice::createClient();
  
  if (!pClient->connect(myDevice)){
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    return false;
  }
  // Obtain a reference to the service we are after in the remote BLE server.
  
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    pClient->disconnect();
    return false;
  }
  
  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    pClient->disconnect();
    return false;
  }

  // Read the value of the characteristic. Does not work with HM-10 module.
  /*if(pRemoteCharacteristic->canRead()) {
    std::string value = pRemoteCharacteristic->readValue();
  }*/
  
  if(pRemoteCharacteristic->canNotify()){
    pRemoteCharacteristic->registerForNotify(notifyCallback);
  }

  // Flag to indicate that it was successfully connected to the server
  connected = true;
  return true;
}

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void scanList(BLEScanResults results, BLEScan* pBLEScan){
  pBLEScan->stop();
  String valuesTS[2]={"--_-200","--_-200"};
  String RSSIValue;
  bool hasNodes=false;
  int channel;
  for(uint8_t i=0;i<results.getCount();i++) {
    doConnect = false;
    Serial.printf("Advertised Device: %s \n", results.getDevice(i).toString().c_str());
    //Analyze if it is the device with the same service and name (Nodo[Num]) (In case there is another device with that service).
    if (results.getDevice(i).haveServiceUUID() && results.getDevice(i).isAdvertisingService(serviceUUID)) {
      if (strncmp (results.getDevice(i).getName().c_str(),"Nodo",4)==0){
        char c[1]={results.getDevice(i).getName().c_str()[4]};
        channel = std::atoi(c);
          if (myDevice != nullptr){
            delete myDevice;
          }
        myDevice = new BLEAdvertisedDevice(results.getDevice(i));
        doConnect = true;
        RSSIValue = String(myDevice->getRSSI());
      } 
    }
    // Connect if device found.
    if (doConnect == true) {
      // Analyze if the connection is correct.
      if (connectToServer()) {
      } 
      else {
        Serial.printf("_UNABLE TO CONNECT TO DEVICE_");
        doConnect = false;
      }
    }
  // The connection was successful.
  if (connected) {
    hasNodes=true;
    Serial.printf("_CONNECTED_");
    // Command for reading the PIOB pin of the HM-10
    char humArray[] = "AT+ADCB?";
    pRemoteCharacteristic->writeValue(humArray, sizeof(humArray));
    pRemoteCharacteristic->registerForNotify(NULL); // Unsubscribe from notifications, otherwise continue notifying.
    // Vout to Humidity value convertion, then it send to the ThingSpeak Channel
     valuesTS[channel-1] = String(volt2hum(valueVout))+'_'+RSSIValue;
    // Disconnect the client, to continue with the next device that may be connected.
    // Problem: Cannot connect all servers at once.
    pClient->disconnect();

    // Make sure it's offline.
    while (pClient->isConnected()) {
      Serial.printf("_STILL CONNECTED_");
      delay(500);
    }
    Serial.printf("_DISCONNECTED_");
    connected = false;
   
    }
  delay(500);
  }
  
  resultsOK = false;
  if (hasNodes){
      mqttConect();
      String payload = "&field1="+"&field2=";
      client.publish("channels/num/publish", payload.c_str() );
  }
  //Interval between  BLE scans.
  delay(1000*30*60);
}

//  Empieza o reinicia el scaneo durante 5 segundos. Una vez listos los resultados analiza la lista de dispositivos founds.
void scan(){
  BLEScanResults resultados;

  pBLEScan = BLEDevice::getScan();
  MyAdvertisedDeviceCallbacks myCallbacks;
  pBLEScan->setAdvertisedDeviceCallbacks(&myCallbacks);
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  resultados = pBLEScan->start(5);
  resultsOK = true;
  scanList(resultados, pBLEScan);  
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID,WIFI_PASS);
  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
  }
  client.setServer(mqttServer, mqttPort);
 
  mqttConect();
  BLEDevice::init("");
}

void loop() {  
  if (!resultsOK) {
    client.loop();
    scan();
  }
  delay(200);   
}
