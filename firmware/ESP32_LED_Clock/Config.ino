// ==================== 配置管理功能（从借鉴代码 DAOLEDCLOCK-SPFF.ino 整合） ====================

// 加载配置
void myconfig() {
  EEPROM.begin(512);
  
  // 读取温度修正值
  temp_mod = EEPROM.read(0);
  if (temp_mod > 127) temp_mod = temp_mod - 256;
  if (temp_mod < -20 || temp_mod > 20) temp_mod = -3;
  
  // 读取湿度修正值
  hum_mod = EEPROM.read(1);
  if (hum_mod > 127) hum_mod = hum_mod - 256;
  if (hum_mod < -20 || hum_mod > 20) hum_mod = 0;
  
  // 读取亮度
  light = EEPROM.read(2);
  if (light < 0 || light > 100) light = 0;
  
  // 读取城市代码
  city = "";
  for (int i = 10; i < 30; i++) {
    char c = EEPROM.read(i);
    if (c != 0 && c != 255) city += c;
  }
  if (city.length() < 5) city = "101011200";  // 默认北京
  
  // 读取天气密钥
  zx_key = "";
  for (int i = 30; i < 70; i++) {
    char c = EEPROM.read(i);
    if (c != 0 && c != 255) zx_key += c;
  }
  if (zx_key.length() < 5) zx_key = "";
  
  // 读取其他配置
  soundon = EEPROM.read(70);
  if (soundon != 0 && soundon != 1) soundon = true;
  
  caidaion = EEPROM.read(71);
  if (caidaion != 0 && caidaion != 1) caidaion = true;
  
  isnightmode = EEPROM.read(72);
  if (isnightmode != 0 && isnightmode != 1) isnightmode = true;
  
  twopannel = EEPROM.read(73);
  if (twopannel != 0 && twopannel != 1) twopannel = true;
  
  starnum = EEPROM.read(74);
  if (starnum < 0 || starnum > 20) starnum = 20;
  
  EEPROM.end();
}

// 保存配置
void saveconfig() {
  EEPROM.begin(512);
  
  EEPROM.write(0, temp_mod);
  EEPROM.write(1, hum_mod);
  EEPROM.write(2, light);
  
  // 保存城市代码
  for (int i = 0; i < city.length() && i < 20; i++) {
    EEPROM.write(10 + i, city[i]);
  }
  EEPROM.write(10 + city.length(), 0);
  
  // 保存天气密钥
  for (int i = 0; i < zx_key.length() && i < 40; i++) {
    EEPROM.write(30 + i, zx_key[i]);
  }
  EEPROM.write(30 + zx_key.length(), 0);
  
  EEPROM.write(70, soundon);
  EEPROM.write(71, caidaion);
  EEPROM.write(72, isnightmode);
  EEPROM.write(73, twopannel);
  EEPROM.write(74, starnum);
  
  EEPROM.commit();
  EEPROM.end();
}

// 读取 DHT11 温湿度
void dht11read() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  
  temperature = t;
  humidity = h;
  
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" *C");
}

// 读取光敏传感器
float sensor_Read() {
  // 使用 ADC 读取光敏电阻
  int sensorValue = analogRead(34);  // GPIO34 (ADC1_CH6)
  float voltage = sensorValue * (3.3 / 4095.0);
  return voltage;
}

// 设置亮度
void setBrightness(float voltage) {
  int brightness = map(voltage * 100, 0, 330, 10, 200);
  brightness = constrain(brightness, 10, 200);
  display->setBrightness8(brightness);
}

// 更新服务器配置
void updateServer() {
  // 启动 Web 服务器
  server.on("/set", handleSet);
  server.on("/reg", handlereg);
  server.on("/upload", HTTP_POST, uploadFinish, handleFileUpload);
  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "404: Not Found");
    }
  });
  server.begin();
  Serial.println("HTTP server started");
}

// 处理设置请求
void handleSet() {
  String json = "{\"code\":200,\"msg\":\"ok\"}";
  server.send(200, "application/json", json);
}

// 处理注册请求
void handlereg() {
  String json = "{\"code\":200,\"msg\":\"ok\"}";
  server.send(200, "application/json", json);
}

// 上传完成
void uploadFinish() {
  server.send(200, "text/html", "Upload OK");
}

// 获取时间
void GetTime() {
  struct tm *p;
  time_t now;
  now = time(nullptr);
  p = localtime(&now);
  
  year1 = p->tm_year + 1900;
  month1 = p->tm_mon + 1;
  day1 = p->tm_mday;
  hou = p->tm_hour;
  minu = p->tm_min;
  sec = p->tm_sec;
  week = p->tm_wday;
  if (week == 0) week = 15;  // 周日特殊处理
  
  sec_ten = sec / 10;
  sec_one = sec % 10;
}

// NTP 时间获取
time_t getNtpTime() {
  IPAddress ntpServerIP;
  while (udp.parsePacket() > 0);  // 丢弃之前的数据包
  Serial.println("Transmit NTP Request");
  
  WiFi.hostByName(NTP_SERVER, ntpServerIP);
  sendNTPpacket(ntpServerIP);
  
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = udp.parsePacket();
    if (size >= 48) {
      Serial.println("Receive NTP Response");
      udp.read(packetBuffer, 48);
      unsigned long secsSince1900;
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + GMT_OFFSET_SEC;
    }
  }
  Serial.println("No NTP Response");
  return 0;
}

// 发送 NTP 请求包
void sendNTPpacket(IPAddress &address) {
  memset(packetBuffer, 0, 48);
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  
  udp.beginPacket(address, 123);
  udp.write(packetBuffer, 48);
  udp.endPacket();
}
