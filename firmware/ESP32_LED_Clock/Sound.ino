// ==================== 蜂鸣器音乐功能（从借鉴代码 sound.ino 整合） ====================

// 整点报时音乐
void common_play() {
  // 使用 BUZZER_PIN 引脚控制蜂鸣器
  pinMode(BUZZER_PIN, OUTPUT);
  
  // 播放简单旋律
  int melody[] = {262, 294, 330, 349, 392, 440, 494, 523};
  int noteDurations[] = {100, 100, 100, 100, 100, 100, 100, 200};
  
  for (int thisNote = 0; thisNote < 8; thisNote++) {
    int noteDuration = noteDurations[thisNote];
    tone(BUZZER_PIN, melody[thisNote], noteDuration);
    delay(noteDuration * 1.3);
    noTone(BUZZER_PIN);
  }
}

// 播放简单音调
void playTone(int frequency, int duration) {
  tone(BUZZER_PIN, frequency, duration);
  delay(duration);
  noTone(BUZZER_PIN);
}
