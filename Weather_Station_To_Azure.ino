#include <ESP8266WiFi.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>

#define I2C_SCL D6
#define I2C_SDA D7
#define PIN D5
#define TYPE DHT22

#include "src/iotc/common/string_buffer.h"
#include "src/iotc/iotc.h"

//***********Define WIFI*************************************

#define WIFI_SSID "iPhone"
#define WIFI_PASSWORD "giahuy1234"
// #define WIFI_SSID "Idealogic_Office_2"
//#define WIFI_PASSWORD "123123789789"

//**************************************************************

//***********Provide Variable for sensor*************************************

DHT dht(PIN,TYPE);
Adafruit_BMP085 bmp;

//**************************************************************

//***********Define Azure Device*************************************
const char* SCOPE_ID = "0ne0005B5A6";
const char* DEVICE_ID = "318040f8-2734-4f4c-acce-025cc26cdb01";
const char* DEVICE_KEY = "7HSR/1tFCTItujlJdLHSRbbG1f5mfRSeedY8Cwa+bbo=";
//**************************************************************

void on_event(IOTContext ctx, IOTCallbackInfo* callbackInfo);
#include "src/connection.h"

//***********Checking Connection*************************************
void on_event(IOTContext ctx, IOTCallbackInfo* callbackInfo) {
  // ConnectionStatus
  if (strcmp(callbackInfo->eventName, "ConnectionStatus") == 0) {
    LOG_VERBOSE("Is connected ? %s (%d)",
                callbackInfo->statusCode == IOTC_CONNECTION_OK ? "YES" : "NO",
                callbackInfo->statusCode);
    isConnected = callbackInfo->statusCode == IOTC_CONNECTION_OK;
    return;
  }

  // payload buffer doesn't have a null ending.
  // add null ending in another buffer before print
  AzureIOT::StringBuffer buffer;
  if (callbackInfo->payloadLength > 0) {
    buffer.initialize(callbackInfo->payload, callbackInfo->payloadLength);
  }
  
  LOG_VERBOSE("- [%s] event was received. Payload => %s\n",
              callbackInfo->eventName, buffer.getLength() ? *buffer : "EMPTY");

  if (strcmp(callbackInfo->eventName, "Command") == 0) {
    LOG_VERBOSE("- Command name was => %s\r\n", callbackInfo->tag);
  }
}
//**************************************************************

void setup() {
  Serial.begin(115200);
  delay(2000);

  //**************************************************************	
  connect_wifi(WIFI_SSID, WIFI_PASSWORD);
  connect_client(SCOPE_ID, DEVICE_ID, DEVICE_KEY);
  //**************************************************************

  dht.begin();
  delay(1000);
  Wire.begin(I2C_SDA, I2C_SCL);
  delay(1000);
  bmp.begin();
  delay(1000);
  
  //**************************************************************
//  Serial.print(percent);
  if (context != NULL) {
    lastTick = 0;  // set timer in the past to enable first telemetry a.s.a.p
  }
}
//**************************************************************

void loop() {
  //**************************************************************
  float temperature = dht.readTemperature();
  float temp= temperature;
  float humidity = dht.readHumidity();
  float humid= humidity;
  float pressure=bmp.readPressure();
  float P=pressure/1000;
  //**************************************************************

  const int sendtime = 31000; // 31 seconds are the refresh time of Azure

  if (isConnected) {
    unsigned long ms = millis();
    //**************************************************************
    if (ms - lastTick > sendtime) {  // send telemetry every 31 seconds
      char msg[64] = {0};
      char msg2[64] = {0};
      char msg3[64] = {0};
      int pos = 0, errorCode = 0;
      lastTick = ms;
      //**************************************************************
       if (loopId++  % 2== 0) {  // send telemetry  //(loopId = loopId +1 ) % 2 ==0
        pos = snprintf(msg, sizeof(msg) - 1, "{\"temp\": %f}",temp);
        errorCode = iotc_send_telemetry(context, msg, pos);
        pos = snprintf(msg, sizeof(msg) - 1, "{\"press\": %f}",P);
        errorCode = iotc_send_telemetry(context, msg, pos);
        pos = snprintf(msg, sizeof(msg) - 1, "{\"humid\": %f}",humid);
        errorCode = iotc_send_telemetry(context, msg, pos);
      } 

      if(temp >30)
      {
        pos = snprintf(msg, sizeof(msg) - 1, "{\"temptoohigh\":%d}",30);
        errorCode = iotc_send_event(context, msg, pos);   
      }
       else if (isnan(temp)) {
        pos = snprintf(msg, sizeof(msg) - 1, "{\"sensorisnan\":%d}",0);
        errorCode = iotc_send_event(context, msg, pos);  
        }
        else
        { 
          
        }
        
       msg[pos] = 0;
//**************************************************************
      if (errorCode != 0) {
        LOG_ERROR("Sending message has failed with error code %d", errorCode);
      }
    }

    iotc_do_work(context);  // do background work for iotc
  } else {
    iotc_free_context(context);
    context = NULL;
    connect_client(SCOPE_ID, DEVICE_ID, DEVICE_KEY);
  }
}
