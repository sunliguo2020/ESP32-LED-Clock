// ==================== OLED 驱动和显示函数（从借鉴代码 OLEDDriver.ino 整合） ====================

// 初始化 OLED
void initOLED() {
  // Hub75 面板已由 MatrixPanel_I2S_DMA 初始化
}

// 清空 OLED
void clearOLED() {
  display->fillScreen(0);
}

// 绘制文本
void drawText(const char* text, int x, int y) {
  display->setCursor(x, y);
  display->setTextColor(display->color565(255, 255, 255));
  display->print(text);
}

// 清空画布
void cleanTab() {
  display->fillRect(0, 0, 64, 64, 0);
}

// 填充画布
void fillScreenTab() {
  // 由 DMA 驱动自动刷新
}

// 绘制像素点
void fillTab(int x, int y, uint16_t c) {
  if (x >= 0 && x < 128 && y >= 0 && y < 64) {
    display->drawPixel(x, y, c);
  }
}

// 绘制水平线
void drawHLine(int x, int y, int len, uint16_t c) {
  display->drawFastHLine(x, y, len, c);
}

// 绘制垂直线
void drawLine(int x, int y, int len, uint16_t c) {
  display->drawFastVLine(x, y, len, c);
}

// 绘制位图（单色）
void drawBit(int x, int y, const uint16_t* data, int w, int h, uint16_t c) {
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      if (data[j * w + i] != 0) {
        fillTab(x + i, y + j, c);
      }
    }
  }
}

// 绘制彩色位图
void drawColorBit(int x, int y, const uint16_t* data, int w, int h) {
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      if (data[j * w + i] != 0) {
        fillTab(x + i, y + j, data[j * w + i]);
      }
    }
  }
}

// 绘制小位图
void drawSmBit(int x, int y, const uint16_t* data, int w, int h, uint16_t c) {
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      if (data[j * w + i] != 0) {
        fillTab(x + i, y + j, c);
      }
    }
  }
}

// 绘制字符（ASCII）
void drawChars(int x, int y, const char* str, uint16_t c) {
  display->setCursor(x, y);
  display->setTextColor(c);
  display->print(str);
}

// 绘制汉字（使用字库）
void drawHanziS(int x, int y, const char* str, uint16_t c) {
  // 使用 Arduino_GB2312_library.h 中的字库
  // 这里简化处理，使用系统字体
  display->setCursor(x, y);
  display->setTextColor(c);
  display->print(str);
}

// 显示数字（大字体）
void displayNumbers(int num, int x, int y, uint16_t c) {
  char buf[4];
  sprintf(buf, "%d", num);
  display->setCursor(x, y);
  display->setTextColor(c);
  display->setTextSize(2);
  display->print(buf);
  display->setTextSize(1);
}

// 显示数字（中字体）
void displayNumbers2(int num, int x, int y, uint16_t c) {
  char buf[4];
  sprintf(buf, "%d", num);
  display->setCursor(x, y);
  display->setTextColor(c);
  display->print(buf);
}

// 显示小数字
void disSmallNumbers(int num, int x, int y, uint16_t c) {
  char buf[4];
  sprintf(buf, "%d", num);
  display->setCursor(x, y);
  display->setTextColor(c);
  display->setTextSize(1);
  display->print(buf);
}

// 显示 30 号字体数字
void dis30Numbers(int num, int x, int y, uint16_t c) {
  char buf[4];
  sprintf(buf, "%d", num);
  display->setCursor(x, y);
  display->setTextColor(c);
  display->setTextSize(2);
  display->print(buf);
  display->setTextSize(1);
}
