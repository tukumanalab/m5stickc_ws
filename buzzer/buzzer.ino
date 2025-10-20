/*
 * M5StickC Plus 1.1 Buzzer Sample
 *
 * このサンプルはM5StickC Plus 1.1の内蔵ブザーを鳴らすプログラムです。
 * ブザーはGPIO2に接続されています。
 *
 * ボタン操作:
 * - ボタンA (M5ボタン): ドレミファソラシドを演奏
 * - ボタンB (側面ボタン): アラーム音を鳴らす
 */

#include <M5StickCPlus.h>

// M5StickC Plus 1.1 のブザーピン
#define BUZZER_PIN 2

// 音階の周波数定義 (Hz)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

// LEDCチャンネル設定
#define LEDC_CHANNEL 0
#define LEDC_RESOLUTION 8

// 音量設定（デューティ比: 0-255、8bit解像度の場合）
// 値が大きいほど音量が大きくなります（推奨: 128-255）
#define BUZZER_VOLUME 100

void setup() {
  M5.begin();

  // ブザーピンの初期化（新しいAPI）
  ledcAttach(BUZZER_PIN, 1000, LEDC_RESOLUTION);

  // 画面の初期化
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.println("Buzzer Sample");
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(10, 40);
  M5.Lcd.println("Btn A: Play Scale");
  M5.Lcd.setCursor(10, 60);
  M5.Lcd.println("Btn B: Alarm");

  // 起動音を鳴らす
  tone(NOTE_C5, 100);
  delay(100);
  noTone();

  Serial.begin(115200);
  Serial.println("M5StickC Plus 1.1 Buzzer Sample Started");
}

void loop() {
  M5.update();

  // ボタンAが押されたら音階を演奏
  if (M5.BtnA.wasPressed()) {
    M5.Lcd.fillRect(0, 80, 240, 20, BLACK);
    M5.Lcd.setCursor(10, 80);
    M5.Lcd.println("Playing scale...");
    Serial.println("Button A pressed - Playing scale");

    playScale();

    M5.Lcd.fillRect(0, 80, 240, 20, BLACK);
    M5.Lcd.setCursor(10, 80);
    M5.Lcd.println("Done!");
  }

  // ボタンBが押されたらアラーム音
  if (M5.BtnB.wasPressed()) {
    M5.Lcd.fillRect(0, 80, 240, 20, BLACK);
    M5.Lcd.setCursor(10, 80);
    M5.Lcd.println("Alarm!");
    Serial.println("Button B pressed - Playing alarm");

    playAlarm();

    M5.Lcd.fillRect(0, 80, 240, 20, BLACK);
    M5.Lcd.setCursor(10, 80);
    M5.Lcd.println("Done!");
  }

  delay(10);
}

// 指定した周波数と時間でブザーを鳴らす
void tone(int frequency, int duration) {
  ledcChangeFrequency(BUZZER_PIN, frequency, LEDC_RESOLUTION);
  ledcWrite(BUZZER_PIN, BUZZER_VOLUME);  // デューティ比で音量制御
  delay(duration);
}

// ブザーを止める
void noTone() {
  ledcWrite(BUZZER_PIN, 0);  // デューティ比を0にして音を止める
}

// ドレミファソラシドを演奏
void playScale() {
  int notes[] = {NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5};
  int duration = 300; // 各音の長さ (ms)

  for (int i = 0; i < 8; i++) {
    tone(notes[i], duration);
    delay(duration);
    noTone();
    delay(50); // 音の間隔
  }
}

// アラーム音を鳴らす
void playAlarm() {
  for (int i = 0; i < 3; i++) {
    // 高音
    tone(1000, 200);
    delay(200);
    noTone();
    delay(50);

    // 低音
    tone(500, 200);
    delay(200);
    noTone();
    delay(50);
  }
}
