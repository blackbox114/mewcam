/*巴法云的例程
 * 2020-07-07
 * QQ交流群：824273231
 * 微信：19092550573
 * 官网https://bemfa.com  
 *
 *此版本需要json库，再arduino IDE 选项栏，点击“工具”-->"管理库"-->搜索arduinojson 第一个库就是，点击安装即可
 *
分辨率默认配置：config.frame_size = FRAMESIZE_UXGA;
其他配置：
FRAMESIZE_UXGA （1600 x 1200）
FRAMESIZE_QVGA （320 x 240）
FRAMESIZE_CIF （352 x 288）
FRAMESIZE_VGA （640 x 480）
FRAMESIZE_SVGA （800 x 600）
FRAMESIZE_XGA （1024 x 768）
FRAMESIZE_SXGA （1280 x 1024）

config.jpeg_quality = 10;（10-63）越小照片质量最好
数字越小表示质量越高，但是，如果图像质量的数字过低，尤其是在高分辨率时，可能会导致ESP32-CAM崩溃

*支持发布订阅模式，当图片上传时，订阅端会自动获取图片url地址，可做图片识别，人脸识别，图像分析
*/

#include <HTTPClient.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <ArduinoJson.h>

/*********************需要修改的地方**********************/
const char* ssid = "newhtc";           //WIFI名称
const char* password = "qq123456";     //WIFI密码
int capture_interval = 20*1000;        // 默认20秒上传一次，可更改（本项目是自动上传，如需条件触发上传，在需要上传的时候，调用take_send_photo()即可）
const char*  post_url = "http://images.bemfa.com/upload/v1/upimages.php"; // 默认上传地址
const char*  uid = "4d9ec352e0376f2110a0c601a2857225";    //用户私钥，巴法云控制台获取
const char*  topic = "mypicture";     //主题名字，可在控制台新建
/********************************************************/


bool internet_connected = false;
long current_millis;
long last_capture_millis = 0;

// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void setup()
{
  Serial.begin(115200);

  if (init_wifi()) { // Connected to WiFi
    internet_connected = true;
    Serial.println("Internet connected");
  }

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}


/********初始化WIFI*********/
bool init_wifi()
{
  int connAttempts = 0;
  Serial.println("\r\nConnecting to: " + String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
    if (connAttempts > 10) return false;
    connAttempts++;
  }
  return true;
}



/********推送图片*********/
static esp_err_t take_send_photo()
{
    //初始化相机并拍照
    Serial.println("Taking picture...");
    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return ESP_FAIL;
    }
  
    HTTPClient http;
    //设置请求url
    http.begin(post_url);
    
    //设置请求头部信息
    http.addHeader("Content-Type", "image/jpg");
    http.addHeader("Authorization", uid);
    http.addHeader("Authtopic", topic);
    
    //发起请求，并获取状态码
    int httpResponseCode = http.POST((uint8_t *)fb->buf, fb->len);
    
    if(httpResponseCode==200){
        //获取post请求后的服务器响应信息，json格式
        String response = http.getString();  //Get the response to the request
        Serial.print("Response Msg:");
        Serial.println(response);           // 打印服务器返回的信息
        

        //json数据解析
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, response);
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.c_str());
        }
        const char* url = doc["url"];
        Serial.print("Get URL:");
        Serial.println(url);//打印获取的URL
    
        
    }else{
        //错误请求
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
    }
   
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
  
    //清空数据
    esp_camera_fb_return(fb);  
    //回收下次再用
    http.end();
  
}



void loop()
{
  // TODO check Wifi and reconnect if needed

  //定时发送
  current_millis = millis();
  if (current_millis - last_capture_millis > capture_interval) { // 当前时间减去上次时间大于20S就执行拍照上传函数
    last_capture_millis = millis();
    take_send_photo(); //拍照上传函数，在需要的地方调用即可，这里是定时拍照
  }
}
