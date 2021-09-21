#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "WiFiClientSecure.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// WiFi 連線資料
#define ssid "dlink"
#define wifipass "12345678"

// Broker 資訊以下固定
#define broker "mqtt.hover.ovh"
#define brokerPort 8883

// 使用創客助理 "我要註冊" 時的google帳號
#define brokerUser "ttwang@gmail.com"

// Mqtt ClientID 是google帳號尾巴加上1個字元
#define brokerClientId brokerUser "1"
// Broker Password 來自於對創客助理說 "我的連線資訊"
#define brokerPass "1q2w3e4r"

// 發佈訂閱使用的標題固定是這個格式
#define pubtop "actions/" brokerUser "/res"
#define subtop "actions/" brokerUser "/req"

// 繼電器導通是低電位,斷開是高電位
#define NC LOW
#define NO HIGH

// 使用 ESP32 上面的GPIO編號
const int RELAY1=14;
const int RELAY2=27;
const int RELAY3=26;
const int RELAY4=25;

// LetsEncrypt ROOT CA, https://letsencrypt.org/certs/isrgrootx1.pem
const char* rootCA=\
"-----BEGIN CERTIFICATE-----\n"\
"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\n"\
"-----END CERTIFICATE-----\n";

WiFiClientSecure espClient;
Adafruit_MQTT_Client mqtt(&espClient,broker,brokerPort,brokerClientId,brokerUser,brokerPass);
Adafruit_MQTT_Publish mqttWriter = Adafruit_MQTT_Publish(&mqtt, pubtop); 
Adafruit_MQTT_Subscribe mqttReader = Adafruit_MQTT_Subscribe(&mqtt, subtop);
// JSON 字元陣列者度
StaticJsonDocument<200> doc;
 
void setup() {
  Serial.begin(115200);
  // 設定“GPIO初始值
  pinMode(RELAY1,OUTPUT);
  digitalWrite(RELAY1,NO);
  pinMode(RELAY2,OUTPUT);
  digitalWrite(RELAY2,NO);
  pinMode(RELAY3,OUTPUT);
  digitalWrite(RELAY3,NO);
  pinMode(RELAY4,OUTPUT);
  digitalWrite(RELAY4,NO);

WiFi.begin(ssid,wifipass);
delay(2000);

  while (WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Connected");
  Serial.println(WiFi.localIP());

  // 設定MQTT使用加密連線
  espClient.setCACert(rootCA);
  mqtt.subscribe(&mqttReader);

}

void loop() {
  MQTT_connect();

  Adafruit_MQTT_Subscribe *sub;
  while ((sub=mqtt.readSubscription(5000))){
    if (sub==&mqttReader){
      Serial.print("Got:");
      Serial.println((char*)mqttReader.lastread);
      DeserializationError err=deserializeJson(doc,mqttReader.lastread);
      // JSON裡面有三個欄位 Location (表示這個裝置的位置), Device(這個裝置的名稱), Command(這個裝置的命令
      const char *cmd=doc["Command"];
      const char *device=doc["Device"];
      const char *location=doc["Location"];

      // 不管位置, 假如裝置式大門或鐵卷門
      if(strcmp("entry",device)==0){
        //Serial.println(strcmp("on",cmd));
        // 命令是打開,開啟
        if(strcmp("on",cmd)==0){
          digitalWrite(RELAY1,NC);
          delay(500);
          digitalWrite(RELAY1,NO);
          Serial.println("Door Up");
        }
        // 命令是關閉
        if(strcmp("off",cmd)==0){
          digitalWrite(RELAY3,NC);
          delay(500);
          digitalWrite(RELAY3,NO);
          Serial.println("Door Down");
        }
        // 命令是停止
        if(strcmp("stop",cmd)==0){
          digitalWrite(RELAY2,NC);
          delay(500);
          digitalWrite(RELAY2,NO);
          Serial.println("Door Emergy Stop");
        }
        // 不論如何都返回執行完成
        mqttWriter.publish("{\"Command\":\"完成.\"}");
      }else{
        // 假如不是這個裝置要做的
        mqttWriter.publish("{\"Command\":\"Error: 裝置未上線\"}");
      }
      
    }
  }

}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }

  Serial.println("MQTT Connected!");
}
