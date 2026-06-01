/**
 * ESP32-LED-Clock
 * 基于 ESP32 的 Hub75 接口 LED 点阵屏时钟
 * 
 * 功能：
 * - WiFi 自动同步时钟（NTP）
 * - 音乐频谱模式
 * - 变色时钟模式
 * - 俄罗斯方块时钟模式
 * - 温湿度显示（DHT11）
 * - 远程控制（Web 配置）
 * - 整点报时
 * 
 * 依赖库：
 * - ESP32_HUB75_LED_MATRIX_PANEL_DMA_Display
 * - DHT sensor library
 * - ArduinoJson
 * - WiFi / HTTPClient
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
#include <ESP32-HUB75-MatrixPanel-DMA.h>

// ==================== 配置区域 ====================

// NTP 时间服务器配置
const char* NTP_SERVER = "ntp.aliyun.com";
const long GMT_OFFSET_SEC = 8 * 3600;  // 东八区
const int DAYLIGHT_OFFSET_SEC = 0;

// DHT11 温湿度传感器引脚
#define DHT_PIN 15
#define DHT_TYPE DHT11

// Hub75 矩阵面板引脚配置
#define PANEL_RES_X 64   // 面板宽度
#define PANEL_RES_Y 32   // 面板高度
#define PANEL_CHAIN 1    // 级联数量

// 配网模式引脚（BOOT 按键）
#define CONFIG_BUTTON_PIN 0

// ==================== 全局对象 ====================

DHT dht(DHT_PIN, DHT_TYPE);
Preferences preferences;
WebServer server(80);

// Hub75 矩阵面板配置
MatrixPanel_I2S_DMA *display = nullptr;

// 时间变量
struct tm timeinfo;
unsigned long lastTimeSync = 0;
const unsigned long TIME_SYNC_INTERVAL = 3600000;  // 1小时同步一次

// 传感器数据
float temperature = 0.0;
float humidity = 0.0;
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_READ_INTERVAL = 2000;  // 2秒读取一次

// 显示模式枚举
enum DisplayMode {
  MODE_CLOCK = 0,       // 时钟模式
  MODE_SPECTRUM = 1,    // 频谱模式
  MODE_TETRIS = 2,      // 俄罗斯方块模式
  MODE_COLOR_CLOCK = 3  // 变色时钟模式
};

DisplayMode currentMode = MODE_CLOCK;
unsigned long modeSwitchTime = 0;
const unsigned long MODE_SWITCH_INTERVAL = 30000;  // 30秒切换一次模式

// WiFi 配置
String wifiSSID = "";
String wifiPassword = "";
bool wifiConfigured = false;

// ==================== 函数声明 ====================

void setupWiFi();
void startConfigMode();
void handleConfigPage();
void handleSaveConfig();
void syncTime();
void readSensor();
void displayClock();
void displaySpectrum();
void displayTetris();
void displayColorClock();
uint16_t getRainbowColor(int index);
void loadWiFiConfig();
void saveWiFiConfig(const String& ssid, const String& password);

// ==================== 初始化 ====================

void setup() {
  Serial.begin(115200);
  Serial.println(F("\nESP32 LED Clock Starting..."));

  // 初始化 Hub75 矩阵面板
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,   // 宽度
    PANEL_RES_Y,   // 高度
    PANEL_CHAIN    // 级联数量
  );
  
  display = new MatrixPanel_I2S_DMA(mxconfig);
  display->begin();
  display->setBrightness8(80);  // 设置亮度 (0-255)
  display->fillScreen(0);       // 清屏
  display->setTextSize(1);

  // 初始化 DHT11
  dht.begin();

  // 初始化 NVS 存储
  preferences.begin("wifi-config", false);

  // 检查是否需要进入配网模式
  pinMode(CONFIG_BUTTON_PIN, INPUT_PULLUP);
  if (digitalRead(CONFIG_BUTTON_PIN) == LOW) {
    Serial.println(F("BOOT button pressed, entering config mode..."));
    startConfigMode();
    return;
  }

  // 加载 WiFi 配置
  loadWiFiConfig();

  if (!wifiConfigured) {
    // 首次使用，进入配网模式
    Serial.println(F("No WiFi config found, entering config mode..."));
    startConfigMode();
    return;
  }

  // 连接 WiFi
  setupWiFi();

  // 同步时间
  syncTime();

  // 显示启动画面
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
  // 如果在配网模式，处理 Web 服务器请求
  if (!wifiConfigured) {
    server.handleClient();
    return;
  }

  // 定时同步时间
  if (millis() - lastTimeSync > TIME_SYNC_INTERVAL) {
    syncTime();
    lastTimeSync = millis();
  }

  // 定时读取传感器
  if (millis() - lastSensorRead > SENSOR_READ_INTERVAL) {
    readSensor();
    lastSensorRead = millis();
  }

  // 模式切换
  if (millis() - modeSwitchTime > MODE_SWITCH_INTERVAL) {
    currentMode = (DisplayMode)((currentMode + 1) % 4);
    modeSwitchTime = millis();
    display->fillScreen(0);
  }

  // 根据当前模式显示内容
  switch (currentMode) {
    case MODE_CLOCK:
      displayClock();
      break;
    case MODE_SPECTRUM:
      displaySpectrum();
      break;
    case MODE_TETRIS:
      displayTetris();
      break;
    case MODE_COLOR_CLOCK:
      displayColorClock();
      break;
  }

  delay(50);  // 控制刷新率
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

  // 创建配置热点
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32-LED-Clock", "12345678");
  
  IPAddress apIP = WiFi.softAPIP();
  Serial.printf("AP IP address: %s\n", apIP.toString().c_str());
  Serial.println(F("Connect to WiFi: ESP32-LED-Clock"));
  Serial.println(F("Password: 12345678"));
  Serial.println(F("Open browser and visit: 192.168.4.1"));

  // 设置 Web 服务器路由
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
    body {
      font-family: Arial, sans-serif;
      background: #1a1a2e;
      color: #eee;
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      margin: 0;
      padding: 20px;
      box-sizing: border-box;
    }
    .container {
      background: #16213e;
      padding: 30px;
      border-radius: 15px;
      box-shadow: 0 8px 32px rgba(0,0,0,0.3);
      max-width: 400px;
      width: 100%;
    }
    h1 {
      text-align: center;
      color: #0f3460;
      margin-bottom: 30px;
      font-size: 24px;
    }
    h1 span {
      color: #e94560;
    }
    .form-group {
      margin-bottom: 20px;
    }
    label {
      display: block;
      margin-bottom: 8px;
      color: #aaa;
      font-size: 14px;
    }
    input[type="text"],
    input[type="password"] {
      width: 100%;
      padding: 12px;
      border: 2px solid #0f3460;
      background: #1a1a2e;
      color: #eee;
      border-radius: 8px;
      font-size: 16px;
      box-sizing: border-box;
      transition: border-color 0.3s;
    }
    input[type="text"]:focus,
    input[type="password"]:focus {
      outline: none;
      border-color: #e94560;
    }
    .btn {
      width: 100%;
      padding: 14px;
      background: #e94560;
      color: white;
      border: none;
      border-radius: 8px;
      font-size: 18px;
      cursor: pointer;
      transition: background 0.3s;
    }
    .btn:hover {
      background: #c73650;
    }
    .info {
      text-align: center;
      margin-top: 20px;
      color: #666;
      font-size: 12px;
    }
    .scan-btn {
      display: block;
      width: 100%;
      padding: 10px;
      margin-bottom: 20px;
      background: #0f3460;
      color: #eee;
      border: none;
      border-radius: 8px;
      font-size: 14px;
      cursor: pointer;
      transition: background 0.3s;
    }
    .scan-btn:hover {
      background: #1a4a8a;
    }
    #scanResult {
      margin-top: 10px;
      max-height: 150px;
      overflow-y: auto;
    }
    .wifi-item {
      padding: 8px 12px;
      margin: 4px 0;
      background: #1a1a2e;
      border: 1px solid #0f3460;
      border-radius: 5px;
      cursor: pointer;
      font-size: 14px;
      transition: background 0.2s;
    }
    .wifi-item:hover {
      background: #0f3460;
    }
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
    <div class="info">
      配置成功后，设备将自动重启并连接 WiFi<br>
      如需重新配置，按住 BOOT 键重新上电
    </div>
  </div>

  <script>
    function scanWiFi() {
      var btn = event.target;
      btn.textContent = "⏳ 扫描中...";
      btn.disabled = true;
      
      fetch('/scan')
        .then(response => response.json())
        .then(data => {
          var html = '';
          data.forEach(function(network) {
            html += '<div class="wifi-item" onclick="selectWiFi(\'' + network.ssid + '\')">';
            html += '📶 ' + network.ssid;
            if (network.rssi > -50) html += ' ████';
            else if (network.rssi > -70) html += ' ███';
            else if (network.rssi > -80) html += ' ██';
            else html += ' █';
            html += '</div>';
          });
          document.getElementById('scanResult').innerHTML = html;
          btn.textContent = "📶 重新扫描";
          btn.disabled = false;
        })
        .catch(error => {
          document.getElementById('scanResult').innerHTML = '<div style="color:red;">扫描失败，请重试</div>';
          btn.textContent = "📶 扫描 WiFi";
          btn.disabled = false;
        });
    }

    function selectWiFi(ssid) {
      document.getElementById('ssid').value = ssid;
      document.getElementById('password').focus();
    }

    function validateForm() {
      var ssid = document.getElementById('ssid').value.trim();
      if (ssid.length === 0) {
        alert('请输入 WiFi 名称');
        return false;
      }
      return true;
    }
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
    server.send(200, "text/html", 
      "<html><body style='font-family:Arial;text-align:center;padding:50px;background:#1a1a2e;color:#eee;'>"
      "<h2 style='color:#e94560;'>❌ 错误</h2>"
      "<p>WiFi 名称不能为空！</p>"
      "<a href='/' style='color:#0f3460;'>返回重试</a>"
      "</body></html>");
    return;
  }

  // 保存 WiFi 配置
  saveWiFiConfig(ssid, password);
  
  Serial.printf("Saving WiFi config - SSID: %s, Password: %s\n", ssid.c_str(), password.c_str());

  // 显示保存成功页面
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>配置成功</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: #1a1a2e;
      color: #eee;
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      margin: 0;
      padding: 20px;
      box-sizing: border-box;
    }
    .container {
      background: #16213e;
      padding: 30px;
      border-radius: 15px;
      box-shadow: 0 8px 32px rgba(0,0,0,0.3);
      max-width: 400px;
      width: 100%;
      text-align: center;
    }
    .success-icon {
      font-size: 64px;
      margin-bottom: 20px;
    }
    h2 {
      color: #e94560;
      margin-bottom: 20px;
    }
    .info {
      color: #aaa;
      font-size: 14px;
      line-height: 1.6;
    }
    .ssid-display {
      background: #1a1a2e;
      padding: 10px;
      border-radius: 8px;
      margin: 15px 0;
      color: #0f3460;
      font-weight: bold;
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="success-icon">✅</div>
    <h2>配置成功！</h2>
    <div class="ssid-display">)rawliteral";
  html += "WiFi: " + ssid;
  html += R"rawliteral(</div>
    <div class="info">
      设备正在尝试连接 WiFi...<br>
      连接成功后，时钟将自动显示时间<br><br>
      如需重新配置，按住 BOOT 键重新上电
    </div>
  </div>
</body>
</html>
  )rawliteral";

  server.send(200, "text/html", html);
  delay(1000);

  // 尝试连接 WiFi
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
    
    // 重启设备
    Serial.println(F("Restarting..."));
    ESP.restart();
  } else {
    Serial.println("\nWiFi connection failed!");
    Serial.println(F("Check your WiFi credentials and try again."));
    Serial.println(F("Restarting to config mode..."));
    
    display->fillScreen(0);
    display->setCursor(0, 0);
    display->setTextColor(display->color565(255, 0, 0));
    display->println("WiFi FAIL!");
    display->setTextColor(display->color565(255, 255, 0));
    display->println("Check config");
    display->println("Restarting...");
    delay(3000);
    
    // 清除错误配置，重新进入配网模式
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

// ==================== 时钟模式 ====================

void displayClock() {
  if (!getLocalTime(&timeinfo)) return;
  
  display->fillScreen(0);
  
  // 显示时间 (大字体)
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  
  display->setCursor(0, 0);
  display->setTextSize(2);
  display->setTextColor(display->color565(0, 255, 255));
  display->println(timeStr);
  
  // 显示日期
  char dateStr[20];
  sprintf(dateStr, "%04d-%02d-%02d", 
          timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
  
  display->setCursor(0, 16);
  display->setTextSize(1);
  display->setTextColor(display->color565(255, 255, 0));
  display->println(dateStr);
  
  // 显示温湿度
  char tempStr[16];
  sprintf(tempStr, "T:%.1fC H:%.0f%%", temperature, humidity);
  
  display->setCursor(0, 24);
  display->setTextSize(1);
  display->setTextColor(display->color565(0, 255, 0));
  display->println(tempStr);
}

// ==================== 频谱模式（模拟） ====================

void displaySpectrum() {
  display->fillScreen(0);
  
  // 模拟频谱显示
  const int numBars = 16;
  const int barWidth = PANEL_RES_X / numBars;
  
  for (int i = 0; i < numBars; i++) {
    // 使用模拟值生成频谱条
    int barHeight = (sin(millis() / 100.0 + i * 0.5) * 0.5 + 0.5) * PANEL_RES_Y;
    barHeight = constrain(barHeight, 1, PANEL_RES_Y);
    
    uint16_t color = getRainbowColor(i * 16);
    
    for (int y = PANEL_RES_Y - barHeight; y < PANEL_RES_Y; y++) {
      for (int x = i * barWidth; x < (i + 1) * barWidth; x++) {
        display->drawPixel(x, y, color);
      }
    }
  }
}

// ==================== 俄罗斯方块时钟模式（简化版） ====================

void displayTetris() {
  display->fillScreen(0);
  
  // 显示时间
  if (!getLocalTime(&timeinfo)) return;
  
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  
  // 用俄罗斯方块风格的方块显示时间
  display->setCursor(4, 4);
  display->setTextSize(2);
  display->setTextColor(getRainbowColor(millis() / 50));
  display->println(timeStr);
  
  // 底部显示装饰方块
  for (int i = 0; i < PANEL_RES_X; i += 4) {
    int y = PANEL_RES_Y - 4 - (sin(millis() / 200.0 + i * 0.1) * 4);
    display->fillRect(i, y, 3, 3, getRainbowColor(i * 4 + millis() / 100));
  }
}

// ==================== 变色时钟模式 ====================

void displayColorClock() {
  if (!getLocalTime(&timeinfo)) return;
  
  display->fillScreen(0);
  
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  
  // 时间颜色随时间变化
  uint16_t color = getRainbowColor(millis() / 30);
  
  display->setCursor(0, 4);
  display->setTextSize(2);
  display->setTextColor(color);
  display->println(timeStr);
  
  // 显示日期
  char dateStr[20];
  sprintf(dateStr, "%04d-%02d-%02d", 
          timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
  
  display->setCursor(0, 20);
  display->setTextSize(1);
  display->setTextColor(getRainbowColor(millis() / 50 + 128));
  display->println(dateStr);
}

// ==================== 辅助函数 ====================

uint16_t getRainbowColor(int index) {
  int r, g, b;
  index = index % 256;
  
  if (index < 85) {
    r = index * 3;
    g = 255 - index * 3;
    b = 0;
  } else if (index < 170) {
    index -= 85;
    r = 255 - index * 3;
    g = 0;
    b = index * 3;
  } else {
    index -= 170;
    r = 0;
    g = index * 3;
    b = 255 - index * 3;
  }
  
  return display->color565(r, g, b);
}
