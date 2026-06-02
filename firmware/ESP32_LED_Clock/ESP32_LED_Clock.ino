/**
 * ESP32-LED-Clock
 * 基于 ESP32 的 Hub75 接口 LED 点阵屏时钟
 * 
 * 功能：
 * - WiFi 自动同步时钟（NTP）
 * - 农历、节气、节日显示
 * - 天气预报显示（和风天气 API）
 * - 温湿度显示（DHT11）
 * - 整点报时（蜂鸣器）
 * - 夜间模式（光敏传感器自动调光）
 * - 老虎 GIF 动画
 * - 星星闪烁背景效果
 * - 远程控制（Web 配置 + 云端 API）
 * - 音乐频谱模式
 * - 变色时钟模式
 * - 俄罗斯方块时钟模式
 * 
 * 依赖库：
 * - ESP32_HUB75_LED_MATRIX_PANEL_DMA_Display
 * - DHT sensor library
 * - ArduinoJson
 * - WiFi / HTTPClient / WebServer
 * 
 * WiFi 配置方式：
 * 1. 首次使用：ESP32 会创建名为 "ESP32-LED-Clock" 的 WiFi 热点
 * 2. 手机连接该热点后，打开浏览器访问 192.168.4.1
 * 3. 在页面中输入 WiFi 名称和密码完成配置
 * 4. 配置成功后 ESP32 会自动重启连接 WiFi
 * 5. 如需重新配置，按住 BOOT 键上电即可进入配网模式
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Preferences.h>
#include <EEPROM.h>
#include <ESP32-HUB75-MatrixPanel-DMA.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <SPIFFS.h>
#include <FS.h>

// ==================== 配置区域 ====================

// NTP 时间服务器配置
const char* NTP_SERVER = "ntp.aliyun.com";
const long GMT_OFFSET_SEC = 8 * 3600;
const int DAYLIGHT_OFFSET_SEC = 0;

// DHT11 温湿度传感器引脚
#define DHT_PIN 15
#define DHT_TYPE DHT11

// Hub75 矩阵面板引脚配置
#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 1

// 配网模式引脚（BOOT 按键）
#define CONFIG_BUTTON_PIN 0

// 蜂鸣器引脚
#define BUZZER_PIN 2

// ==================== 全局对象 ====================

DHT dht(DHT_PIN, DHT_TYPE);
Preferences preferences;
WebServer server(80);
MatrixPanel_I2S_DMA *display = nullptr;
WiFiUDP udp;

// 时间变量
struct tm timeinfo;
unsigned long lastTimeSync = 0;
const unsigned long TIME_SYNC_INTERVAL = 3600000;

// 传感器数据
float temperature = 0.0;
float humidity = 0.0;
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_READ_INTERVAL = 2000;

// WiFi 配置
String wifiSSID = "";
String wifiPassword = "";
bool wifiConfigured = false;

// ==================== 借鉴代码功能变量 ====================

int hou, minu, sec, year1, month1, day1, week;
int sec_ten, sec_one;
char *china_year, *china_month, *china_day;
String jieri = "";
char *jieqi;
char wea_temp1[4] = "";
int wea_code = 999, wea_hm = 0;
char tem_day1_min[4]="", tem_day1_max[4]="";
char tem_day2_min[4]="", tem_day2_max[4]="";
char tem_day3_min[4]="", tem_day3_max[4]="";
char day1_date[11]="", day2_date[11]="", day3_date[11]="";
char temp_day_date[6]="";
int wea_code_day1, wea_code_night1;
int wea_code_day2, wea_code_night2;
int wea_code_day3, wea_code_night3;
int temp_mod = -3, hum_mod = 0, light = 0;
bool soundon = true, caidaion = true, twopannel = true;
bool isnightmode = true, isnight = false;
int starnum = 20;
int star_x[20], star_y[20];
uint16_t star_color[20];
bool isGeneralStar = true;
const char* laohugif[] = {"/hlh1.bmp","/hlh2.bmp","/hlh1.bmp","/hlh2.bmp","/hlh1.bmp","/hlh2.bmp","/hlh1.bmp","/hlh2.bmp","/hlh1.bmp","/hlh2.bmp","/hlh1.bmp","/hlh2.bmp"};
int gif_i = 0, screen_num = 0, scroll_x = 0;
int yy1 = 0, yy2 = 0, yy3 = 0;
uint16_t color = 0xf0b0, color2 = 0x780F, color3 = 0xf000, color4 = 0xfff0, color5 = 0xFDA0;
String macAddr = "", zx_key = "", city = "";
int len_city, len_key;
File fsUploadFile;

// NTP 数据包缓冲区
byte packetBuffer[48];

// WiFi 配置变量（用于 WiFiConfig.ino）
String wifi_ssid = "";
String wifi_pass = "";
String scanNetworksID = "";
String ROOT_HTML = "";

// ==================== 函数声明 ====================

void setupWiFi();
void startConfigMode();
void handleConfigPage();
void handleSaveConfig();
void syncTime();
void readSensor();
uint16_t getRainbowColor(int index);
void loadWiFiConfig();
void saveWiFiConfig(const String& ssid, const String& password);

// 借鉴代码功能函数（实现在其他 .ino 文件中）
void GetTime();
void getNongli(String nian, String yue, String ri);
void getWeather();
void get3DayWeather();
void getBirth();
void getConf();
void showTime();
void showTigger();
void onlyShowTime();
void onlyShowTime2();
void nightMode();
void common_play();
void showlaohu();
void drawBmp(const char *filename, int16_t x, int16_t y);
void myconfig();
void saveconfig();
void dht11read();
float sensor_Read();
void setBrightness(float voltage);
void updateServer();
void handleSet();
void handlereg();
void uploadFinish();
void handleFileUpload();
bool handleFileRead(String path);
String getContentType(String filename);
String formatBytes(size_t bytes);
uint16_t read16(File f);
uint32_t read32(File f);
char * showWeek(int v_week);
void refreshData(void * parameter);
void refreshTQ(void * parameter);
void housound(void * parameter);
void showJieri(void * parameter);
void show3dayWeather();
void showTQ(int code, int x, int y);
void drawChars(int x, int y, const char* str, uint16_t c);
void drawHanziS(int x, int y, const char* str, uint16_t c);
void displayNumbers(int num, int x, int y, uint16_t c);
void displayNumbers2(int num, int x, int y, uint16_t c);
void disSmallNumbers(int num, int x, int y, uint16_t c);
void dis30Numbers(int num, int x, int y, uint16_t c);
void drawBit(int x, int y, const uint16_t* data, int w, int h, uint16_t c);
void drawColorBit(int x, int y, const uint16_t* data, int w, int h);
void drawSmBit(int x, int y, const uint16_t* data, int w, int h, uint16_t c);
void drawLine(int x, int y, int len, uint16_t c);
void drawHLine(int x, int y, int len, uint16_t c);
void fillTab(int x, int y, uint16_t c);
void cleanTab();
void fillScreenTab();
void initOLED();
void clearOLED();
void drawText(const char* text, int x, int y);
void connectToWiFi(int timeOut_s);
void wifiConfig();
void initSoftAP();
void initWebServer();
bool scanWiFi();
void handleRoot();
void handleConfigWifi();
void handleNotFound();
time_t getNtpTime();
void sendNTPpacket(IPAddress &address);

// ==================== 初始化 ====================

void setup() {
  Serial.begin(115200);
  Serial.println(F("\nESP32 LED Clock Starting..."));

  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);
  display = new MatrixPanel_I2S_DMA(mxconfig);
  display->begin();
  display->setBrightness8(80);
  display->fillScreen(0);
  display->setTextSize(1);

  dht.begin();
  preferences.begin("wifi-config", false);
  myconfig();
  macAddr = WiFi.macAddress();

  pinMode(CONFIG_BUTTON_PIN, INPUT_PULLUP);
  if (digitalRead(CONFIG_BUTTON_PIN) == LOW) {
    Serial.println(F("BOOT button pressed, entering config mode..."));
    startConfigMode();
    return;
  }

  loadWiFiConfig();
  if (!wifiConfigured) {
    Serial.println(F("No WiFi config found, entering config mode..."));
    startConfigMode();
    return;
  }

  setupWiFi();
  syncTime();

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
  } else {
    Serial.println("SPIFFS OK!");
  }

  if (WiFi.status() == WL_CONNECTED) {
    updateServer();
    GetTime();
    String year_ = String(year1), month_ = String(month1), day_ = String(day1);
    getNongli(year_, month_, day_);
    getWeather();
    get3DayWeather();
    getBirth();
    dht11read();
  }

  display->fillScreen(0);
  display->setCursor(0, 0);
  display->setTextColor(display->color565(0, 255, 0));
  display->println("ESP32");
  display->println("CLOCK");
  display->println("READY");
  delay(2000);
  display->fillScreen(0);
}

// ==================== 主循环 ====================

void loop() {
  if (!wifiConfigured) {
    server.handleClient();
    return;
  }

  server.handleClient();

  if (millis() - lastTimeSync > TIME_SYNC_INTERVAL) {
    syncTime();
    lastTimeSync = millis();
  }

  if (millis() - lastSensorRead > SENSOR_READ_INTERVAL) {
    readSensor();
    lastSensorRead = millis();
  }

  if (sensor_Read() < 1.2) {
    nightMode();
  } else {
    isnight = false;
  }
  setBrightness(sensor_Read());

  if (minu == 0 && sec == 0) {
    setSyncProvider(getNtpTime);
    GetTime();
    String year_ = String(year1), month_ = String(month1), day_ = String(day1);
    getNongli(year_, month_, day_);
  }

  if (minu % 10 == 0 && sec == 0 && minu != 0) {
    getWeather();
    get3DayWeather();
    getBirth();
  }

  if (sec % 10 == 0) {
    getConf();
  }

  cleanTab();

  if (WiFi.status() == WL_CONNECTED) {
    if (isnightmode) {
      if (hour() > 5 && hour() < 23) {
        GetTime();
        showTime();
        showTigger();
      } else {
        if (twopannel) { screen_num = 0; onlyShowTime(); }
        else { screen_num = 0; onlyShowTime2(); }
        isnight = true;
      }
    } else {
      GetTime();
      showTime();
      showTigger();
    }
    fillScreenTab();
  } else {
    connectToWiFi(15);
  }

  delay(350);
}

// ==================== WiFi 配置存储 ====================

void loadWiFiConfig() {
  wifiSSID = preferences.getString("ssid", "");
  wifiPassword = preferences.getString("password", "");
  wifiConfigured = (wifiSSID.length() > 0);
  if (wifiConfigured) {
    Serial.printf("Loaded WiFi config: SSID=%s\n", wifiSSID.c_str());
  }
}

void saveWiFiConfig(const String& ssid, const String& password) {
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  wifiSSID = ssid;
  wifiPassword = password;
  wifiConfigured = true;
  Serial.println(F("WiFi config saved!"));
}

// ==================== 配网模式 ====================

void startConfigMode() {
  display->fillScreen(0);
  display->setCursor(0, 0);
  display->setTextColor(display->color565(255, 255, 0));
  display->println("CONFIG");
  display->println("MODE");
  display->setTextColor(display->color565(0, 255, 255));
  display->println("Connect to");
  display->println("ESP32-LED");
  display->println("192.168.4.1");

  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32-LED-Clock", "12345678");
  IPAddress apIP = WiFi.softAPIP();
  Serial.printf("AP IP address: %s\n", apIP.toString().c_str());
  Serial.println(F("Connect to WiFi: ESP32-LED-Clock, Password: 12345678"));
  Serial.println(F("Open browser and visit: 192.168.4.1"));

  server.on("/", handleConfigPage);
  server.on("/save", handleSaveConfig);
  server.begin();
  Serial.println(F("HTTP server started"));
}

void handleConfigPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 LED Clock 配置</title>
  <style>
    body { font-family: Arial, sans-serif; background: #1a1a2e; color: #eee; display: flex; justify-content: center; align-items: center; min-height: 100vh; margin: 0; padding: 20px; box-sizing: border-box; }
    .container { background: #16213e; padding: 30px; border-radius: 15px; box-shadow: 0 8px 32px rgba(0,0,0,0.3); max-width: 400px; width: 100%; }
    h1 { text-align: center; color: #0f3460; margin-bottom: 30px; font-size: 24px; }
    h1 span { color: #e94560; }
    .form-group { margin-bottom: 20px; }
    label { display: block; margin-bottom: 8px; color: #aaa; font-size: 14px; }
    input[type="text"], input[type="password"] { width: 100%; padding: 12px; border: 2px solid #0f3460; background: #1a1a2e; color: #eee; border-radius: 8px; font-size: 16px; box-sizing: border-box; }
    .btn { width: 100%; padding: 14px; background: #e94560; color: white; border: none; border-radius: 8px; font-size: 18px; cursor: pointer; }
    .btn:hover { background: #c73650; }
    .info { text-align: center; margin-top: 20px; color: #666; font-size: 12px; }
    .scan-btn { display: block; width: 100%; padding: 10px; margin-bottom: 20px; background: #0f3460; color: #eee; border: none; border-radius: 8px; font-size: 14px; cursor: pointer; }
    .scan-btn:hover { background: #1a4a8a; }
    #scanResult { margin-top: 10px; max-height: 150px; overflow-y: auto; }
    .wifi-item { padding: 8px 12px; margin: 4px 0; background: #1a1a2e; border: 1px solid #0f3460; border-radius: 5px; cursor: pointer; font-size: 14px; }
    .wifi-item:hover { background: #0f3460; }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 <span>LED Clock</span></h1>
    <h2 style="text-align:center;font-size:18px;color:#e94560;margin-bottom:20px;">WiFi 配置</h2>
    <button class="scan-btn" onclick="scanWiFi()">📶 扫描 WiFi</button>
    <div id="scanResult"></div>
    <form action="/save" method="POST" onsubmit="return validateForm()">
      <div class="form-group">
        <label for="ssid">WiFi 名称 (SSID)</label>
        <input type="text" id="ssid" name="ssid" placeholder="请输入 WiFi 名称" required>
      </div>
      <div class="form-group">
        <label for="password">WiFi 密码</label>
        <input type="password" id="password" name="password" placeholder="请输入 WiFi 密码">
      </div>
      <button type="submit" class="btn">保存并连接</button>
    </form>
    <div class="info">配置成功后，设备将自动重启并连接 WiFi<br>如需重新配置，按住 BOOT 键重新上电</div>
  </div>
  <script>
    function scanWiFi() {
      var btn = event.target; btn.textContent = "⏳ 扫描中..."; btn.disabled = true;
      fetch('/scan').then(r=>r.json()).then(d=>{
        var html = '';
        d.forEach(function(n){ html += '<div class="wifi-item" onclick="selectWiFi(\''+n.ssid+'\')">📶 '+n.ssid+(n.rssi>-50?' ████':n.rssi>-70?' ███':n.rssi>-80?' ██':' █')+'</div>'; });
        document.getElementById('scanResult').innerHTML = html;
        btn.textContent = "📶 重新扫描"; btn.disabled = false;
      }).catch(e=>{ document.getElementById('scanResult').innerHTML = '<div style="color:red;">扫描失败</div>'; btn.textContent = "📶 扫描 WiFi"; btn.disabled = false; });
    }
    function selectWiFi(ssid) { document.getElementById('ssid').value = ssid; document.getElementById('password').focus(); }
    function validateForm() { var s = document.getElementById('ssid').value.trim(); if(s.length===0){alert('请输入 WiFi 名称');return false;} return true; }
  </script>
</body>
</html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleSaveConfig() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  if (ssid.length() == 0) {
    server.send(200, "text/html", "<html><body style='font-family:Arial;text-align:center;padding:50px;background:#1a1a2e;color:#eee;'><h2 style='color:#e94560;'>❌ 错误</h2><p>WiFi 名称不能为空！</p><a href='/' style='color:#0f3460;'>返回重试</a></body></html>");
    return;
  }
  saveWiFiConfig(ssid, password);
  Serial.printf("Saving WiFi config - SSID: %s\n", ssid.c_str());

  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>配置成功</title><style>body{font-family:Arial,sans-serif;background:#1a1a2e;color:#eee;display:flex;justify-content:center;align-items:center;min-height:100vh;margin:0;padding:20px;box-sizing:border-box;}.container{background:#16213e;padding:30px;border-radius:15px;box-shadow:0 8px 32px rgba(0,0,0,0.3);max-width:400px;width:100%;text-align:center;}.success-icon{font-size:64px;margin-bottom:20px;}h2{color:#e94560;margin-bottom:20px;}.info{color:#aaa;font-size:14px;line-height:1.6;}.ssid-display{background:#1a1a2e;padding:10px;border-radius:8px;margin:15px 0;color:#0f3460;font-weight:bold;}</style></head><body><div class='container'><div class='success-icon'>✅</div><h2>配置成功！</h2><div class='ssid-display'>WiFi: " + ssid + "</div><div class='info'>设备正在尝试连接 WiFi...<br>连接成功后，时钟将自动显示时间<br><br>如需重新配置，按住 BOOT 键重新上电</div></div></body></html>";
  server.send(200, "text/html", html);
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  display->fillScreen(0);
  display->setCursor(0, 0);
  display->setTextColor(display->color565(255, 255, 0));
  display->println("Connecting");
  display->println("to WiFi...");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    display->print(".");
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    display->fillScreen(0);
    display->setCursor(0, 0);
    display->setTextColor(display->color565(0, 255, 0));
    display->println("WiFi OK!");
    display->setTextColor(display->color565(0, 255, 255));
    display->println(WiFi.localIP());
    delay(2000);
    ESP.restart();
  } else {
    Serial.println("\nWiFi connection failed!");
    display->fillScreen(0);
    display->setCursor(0, 0);
    display->setTextColor(display->color565(255, 0, 0));
    display->println("WiFi FAIL!");
    display->setTextColor(display->color565(255, 255, 0));
    display->println("Restarting...");
    delay(3000);
    preferences.clear();
    ESP.restart();
  }
}

// ==================== WiFi 连接 ====================

void setupWiFi() {
  display->fillScreen(0);
  display->setCursor(0, 0);
  display->setTextColor(display->color565(255, 255, 0));
  display->println("WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    display->print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    display->setTextColor(display->color565(0, 255, 0));
    display->println("\nOK!");
    Serial.print("Connected! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    display->setTextColor(display->color565(255, 0, 0));
    display->println("\nFAIL");
    Serial.println("WiFi connection failed!");
  }
  delay(1000);
}

// ==================== NTP 时间同步 ====================

void syncTime() {
  if (WiFi.status() != WL_CONNECTED) return;
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  Serial.print("Syncing time...");
  int retry = 0;
  while (!getLocalTime(&timeinfo) && retry < 10) {
    Serial.print(".");
    delay(500);
    retry++;
  }
  if (retry < 10) {
    Serial.println(" OK");
    Serial.printf("Time: %02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  } else {
    Serial.println(" FAIL");
  }
}

// ==================== 传感器读取 ====================

void readSensor() {
  float newTemp = dht.readTemperature();
  float newHum = dht.readHumidity();
  if (!isnan(newTemp) && !isnan(newHum)) {
    temperature = newTemp;
    humidity = newHum;
  }
}

// ==================== 辅助函数 ====================

uint16_t getRainbowColor(int index) {
  int r, g, b;
  index = index % 256;
  if (index < 85) { r = index * 3; g = 255 - index * 3; b = 0; }
  else if (index < 170) { index -= 85; r = 255 - index * 3; g = 0; b = index * 3; }
  else { index -= 170; r = 0; g = index * 3; b = 255 - index * 3; }
  return display->color565(r, g, b);
}
