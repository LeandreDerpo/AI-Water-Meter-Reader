/**
 * @file main.cpp
 * @author Cherntay Shih
 * @brief 
 * @version 0.1
 * @date 2022-04-26
 * 
 * @copyright Copyright (c) Apache 2.0
 */
// Initialization
#include "Arduino.h"
#include "esp_camera.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include <WiFi.h>

#include <base64.h>
#include <PubSubClient.h> 
#include <libb64/cencode.h>

#include "soc/soc.h" // Diasble brownout problems
#include "soc/rtc_cntl_reg.h" // Diasble brownout problems
//#include "driver/rtc_io.h"
//#include <MQTTClient.h>
//#include <StringArray.h>

// Replace with your network credentials
const char* SSID = "";
const char* PASSWORD = "";

WiFiClient MQTTClient; 
PubSubClient client(MQTTClient); 

// MQTT BROKER IP ADDRESS
IPAddress mqtt_server(,,,);
const  unsigned  int  mqtt_port =  1883; 
#define MQTT_USER ""
#define MQTT_PASSWORD ""
#define  MQTT_PUBLISH_TOPIC "yourtopic/send" 

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER

// ESP32-CAM Essence module pin setting 
# define  PWDN_GPIO_NUM      32 
# define  RESET_GPIO_NUM     -1 
# define  XCLK_GPIO_NUM       0 
# define  SIOD_GPIO_NUM      26 
# define  SIOC_GPIO_NUM      27 
# define  Y9_GPIO_NUM        35 
# define  Y8_GPIO_NUM        34 
# define  Y7_GPIO_NUM        39 
# define  Y6_GPIO_NUM        36 
# define  Y5_GPIO_NUM        21 
# define  Y4_GPIO_NUM        19 
# define  Y3_GPIO_NUM        18 
# define  Y2_GPIO_NUM         5 
# define  VSYNC_GPIO_NUM     25 
# define  HREF_GPIO_NUM      23 
# define  PCLK_GPIO_NUM      22

int connection_time_counter;
//char base64_string[7500];
String base64_string;

char mqtt_json_topic[250];
char mqtt_message_buffer[50000];

//float battery_voltage;

float uS_TO_S_FACTOR = 1000000; //Conversion factor for micro second to second
float TIME_TO_SLEEP = 3600; // Time ESP32 will go to sleep in seconds

// Print serialprint easier way
void StreamPrint_progmem(Print &out,PGM_P format,...)
{
  // program memory version of printf - copy of format string and result share a buffer
  // so as to avoid too much memory use
  char formatString[128], *ptr;
  strncpy_P( formatString, format, sizeof(formatString) ); // copy in from program mem
  // null terminate - leave last char since we might need it in worst case for result's \0
  formatString[ sizeof(formatString)-2 ]='\0'; 
  ptr=&formatString[ strlen(formatString)+1 ]; // our result buffer...
  va_list args;
  va_start (args,format);
  vsnprintf(ptr, sizeof(formatString)-1-strlen(formatString), formatString, args );
  va_end (args);
  formatString[ sizeof(formatString)-1 ]='\0'; 
  out.print(ptr);
}
#define Serialprint(format, ...) StreamPrint_progmem(Serial,PSTR(format),##__VA_ARGS__)

// Inital setup to connect to router
void setup_Wifi(){
  delay(10);
  // connecting to the wifi
  Serialprint("Connecting to ");
  Serialprint(SSID);

  WiFi.disconnect();
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serialprint(".");
    delay(500);
    connection_time_counter++;
    if(connection_time_counter == 120){
      ESP.restart();
    }
  }

  Serialprint("WiFi connected\n");
  Serialprint("IP address: ");
  Serial.print(WiFi.localIP());
  Serial.println();
}

// Reconnect if initial connection failed
void reconnect(){
    // Loop until we're reconnected
  while (!client.connected()) {
    Serialprint("Attempting MQTT connection...");
    // Attempt to connect
    client.setKeepAlive(60);
    String clientId = "ESP32Client";
    //clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)){
//    if (client.connect(clientId.c_str())) {
      Serialprint("connected!");
      Serial.println();
      delay(10);
      //for testing
      //client.publish("esp32/test/string", "Hello world!");

    } else {
      Serialprint("failed, rc=");
      Serialprint(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// Callback function of the wifi library
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(4, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(4, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

// Initialize ESP-32 Camera
void setup_Camera(){ 
  camera_config_t  config; 
  config.ledc_channel  = LEDC_CHANNEL_0; 
  config.ledc_timer = LEDC_TIMER_0; 
  config.pin_d0  = Y2_GPIO_NUM; 
  config.pin_d1  = Y3_GPIO_NUM; 
  config.pin_d2  = Y4_GPIO_NUM; 
  config.pin_d3  = Y5_GPIO_NUM; 
  config.pin_d4  = Y6_GPIO_NUM; 
  config.pin_d5  = Y7_GPIO_NUM; 
  config.pin_d6  = Y8_GPIO_NUM; 
  config.pin_d7  = Y9_GPIO_NUM; 
  config.pin_xclk  = XCLK_GPIO_NUM; 
  config.pin_pclk  = PCLK_GPIO_NUM; 
  config.pin_vsync  = VSYNC_GPIO_NUM; 
  config.pin_href  = HREF_GPIO_NUM; 
  config.pin_sscb_sda  = SIOD_GPIO_NUM; 
  config.pin_sscb_scl  = SIOC_GPIO_NUM; 
  config.pin_pwdn  = PWDN_GPIO_NUM; 
  config.pin_reset  = RESET_GPIO_NUM; 
  config.xclk_freq_hz  =  20000000 ; 
  config.pixel_format  = PIXFORMAT_JPEG; 
  
  /*  WARNING!!! 
    PSRAM IC required for UXGA resolution and high JPEG quality 
    Ensure ESP32 Wrover Module or other board with PSRAM is selected 
    Partial images will be transmitted if image exceeds buffer size 
    if PSRAM IC present, init with UXGA resolution and higher JPEG quality 
    for larger pre-allocated frame buffer.
  */

  //init with high specs to pre-allocate larger buffers
  if (psramFound()){ // Whether there is PSRAM (Psuedo SRAM) memory IC 
    config.frame_size = FRAMESIZE_UXGA; 
    config.jpeg_quality = 10; 
    config.fb_count = 2; 
  }  else  { 
    config.frame_size = FRAMESIZE_SVGA; 
    config.jpeg_quality = 12; 
    config.fb_count = 1; 
  } 

  // Initialize video; camera init
  esp_err_t  err =  esp_camera_init (&config); 
  if  (err != ESP_OK) { 
    Serial.printf ( " Camera init failed with error 0x%x " , err); 
    ESP.restart (); 
  } 


  // Can customize the default video frame size (resolution size) 
  sensor_t  * s =  esp_camera_sensor_get(); 
  //  initial sensors are flipped vertically and colors are a bit saturated 
  if (s->id.PID == OV2640_PID) { 
    s->set_brightness (s, 1);  //  up the brightness just a bit 
    s->set_saturation (s, 2);  //  lower the saturation 
  } 
  //  drop down frame size for higher initial frame rate 
  s->set_framesize(s, FRAMESIZE_CIF);     // SVGA(800x600), VGA(640x480), CIF(400x296), QVGA(320x240), HQVGA(240x176), QQVGA(160x120), QXGA(2048x1564 for OV3660) 
  // s->set_vflip(s, 1); // vertical flip 
  // s->set_hmirror(s, 1); //horizontal mirror }
}

//void battery_reading(){
//  battery_voltage = analogRead();
//}

void client_publish(){
  camera_fb_t *fb = NULL ; // Pointer

  sprintf(mqtt_json_topic, "tele/%x/CAM", ESP.getChipModel());

//  sprintf(mqtt_message_buffer, "{\"Water_Meter\":{\"Battery\":%s}}", battery_voltage);
  
//  sprintf(mqtt_json_topic, "tele/%x/Battery", ESP.getChipModel());
  Serial.println(mqtt_json_topic);
  digitalWrite(4, HIGH);
  fb = esp_camera_fb_get();
  if (!fb){
    Serial.println("Camera capture failed");
    return;
  }
  esp_camera_fb_return(fb);
  client.publish(mqtt_json_topic, fb->buf, fb->len, false);
  //client.publish(mqtt_json_topic, mqtt_message_buffer);
  Serial.println(mqtt_message_buffer);
  digitalWrite(4, LOW);
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Turns off brownout detector
  Serial.begin(115200);

  pinMode(4, OUTPUT); // Set the pin as output

  Serial.setDebugOutput(true);
  Serial.println();

  setup_Camera();

  setup_Wifi();

  client.setBufferSize(1024 * 50);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  Serial.println(client.getBufferSize());
  reconnect();
}

void loop() {
  esp_wifi_start();

  Serial.println("PSRAM located: " + String(psramFound()));
  digitalWrite(4,HIGH);
  delay(150);
  digitalWrite(4,LOW);
  delay(150);

  client.loop();
  delay(1000);
  if (!client.connected()){
    reconnect();
    delay(100);
  } else {
    Serialprint("failed with state");
    Serial.println(client.state());
    delay(500);
  }

  client_publish();

  delay(1000);

  client.disconnect();
  WiFi.disconnect(true);
  esp_wifi_disconnect();
  esp_wifi_stop();

  delay(100);

  Serialprint("going to sleep!");

  WiFi.mode(WIFI_OFF);
  delay(50);
  while (client.connected() || (WiFi.status() == WL_CONNECTED))
  {
    Serial.println("Waiting for shutdown before sleeping");
    delay(10);
  }
  delay(10);

  delay(1000);

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}
