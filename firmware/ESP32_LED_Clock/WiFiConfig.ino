// ==================== WiFi 配置功能（从借鉴代码 WIFI.ino 整合） ====================

// 自动连接已保存的 WiFi
bool autoConfig() {
  WiFi.begin();
  for (int i = 0; i < 20; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("AutoConfig Success");
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      return true;
    } else {
      Serial.print("AutoConfig Waiting......");
      Serial.println(WiFi.status());
      delay(1000);
    }
  }
  delay(1000);
  Serial.println("AutoConfig Faild!");
  return false;
}

// SmartConfig 配网
void smartConfig() {
  int i = 0;
  WiFi.mode(WIFI_STA);
  Serial.println("\r\nWait for Smartconfig");
  WiFi.beginSmartConfig();
  for (i = 0; i < 30; i++) {
    Serial.print(".");
    if (WiFi.smartConfigDone()) {
      Serial.println("SmartConfig Success");
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      WiFi.setAutoConnect(true);
      ESP.restart();
      break;
    }
    delay(2000);
  }
}

// 初始化 AP 模式
void initSoftAP() {
  IPAddress apIP(192, 168, 4, 1);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  if (WiFi.softAP("DACLOCK")) {
    Serial.println("ESP-32S SoftAP is right.");
    Serial.print("Soft-AP IP address = ");
    Serial.println(WiFi.softAPIP());
    drawText(WiFi.softAPIP().toString().c_str(), 0, 20);
    Serial.println(String("MAC address = ") + WiFi.softAPmacAddress().c_str());
  } else {
    Serial.println("WiFiAP Failed");
    delay(1000);
    Serial.println("restart now...");
    ESP.restart();
  }
}

// 初始化 Web 服务器（配网页面）
void initWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/configwifi", HTTP_POST, handleConfigWifi);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("WebServer started!");
}

// 扫描 WiFi
bool scanWiFi() {
  Serial.println("scan start");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
    scanNetworksID = "no networks found";
    return false;
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      scanNetworksID += "<P>" + WiFi.SSID(i) + "</P>";
      delay(10);
    }
    return true;
  }
}

// 连接 WiFi（带超时）
void connectToWiFi(int timeOut_s) {
  Serial.println("进入connectToWiFi()函数");
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true);
  if (wifi_ssid != "") {
    Serial.println("用web配置信息连接.");
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
    wifi_ssid = "";
    wifi_pass = "";
  } else {
    Serial.println("用nvs保存的信息连接.");
    WiFi.begin();
  }
  int Connect_time = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    Connect_time++;
    if (Connect_time > 4 * timeOut_s) {
      Serial.println("");
      Serial.println("WIFI autoconnect fail, start AP for webconfig now...");
      wifiConfig();
      return;
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    server.stop();
  }
}

// WiFi 配置模式
void wifiConfig() {
  initSoftAP();
  initWebServer();
  scanWiFi();
}

// 处理根目录请求
void handleRoot() {
  if (server.hasArg("selectSSID")) {
    server.send(200, "text/html", ROOT_HTML + scanNetworksID + "</body></html>");
  } else {
    server.send(200, "text/html", ROOT_HTML + scanNetworksID + "</body></html>");
  }
}

// 处理 WiFi 配置提交
void handleConfigWifi() {
  if (server.hasArg("ssid")) {
    Serial.print("got ssid:");
    wifi_ssid = server.arg("ssid");
    Serial.println(wifi_ssid);
  } else {
    Serial.println("error, not found ssid");
    server.send(200, "text/html", "<meta charset='UTF-8'>error, not found ssid");
    return;
  }
  if (server.hasArg("pass")) {
    Serial.print("got password:");
    wifi_pass = server.arg("pass");
    Serial.println(wifi_pass);
  } else {
    Serial.println("error, not found password");
    server.send(200, "text/html", "<meta charset='UTF-8'>error, not found password");
    return;
  }
  server.send(200, "text/html", "<meta charset='UTF-8'>SSID：" + wifi_ssid + "<br />password:" + wifi_pass + "<br />已取得WiFi信息,正在尝试连接,请手动关闭此页面。");
  delay(2000);
  WiFi.softAPdisconnect(true);
  server.close();
  WiFi.softAPdisconnect();
  Serial.println("WiFi Connect SSID:" + wifi_ssid + "  PASS:" + wifi_pass);
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("开始调用连接函数connectToWiFi()..");
    connectToWiFi(15);
  } else {
    Serial.println("提交的配置信息自动连接成功..");
  }
}

// 404 处理
void handleNotFound() {
  handleRoot();
}
