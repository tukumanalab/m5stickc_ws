/*
 * M5StickC Plus 1.1 Angle Unit Sample
 *
 * このサンプルはM5StickC Plus 1.1にAngle Unitを接続して
 * ポテンシオメーターの角度（アナログ値）を読み取り表示します。
 *
 * 接続:
 * - Angle UnitをM5StickC Plus 1.1のGrove端子（HAT端子）に接続
 * - GPIO36 (ADC): アナログ入力
 *
 * ボタン操作:
 * - ボタンA (M5ボタン): 最小値/最大値をリセット
 * - ボタンB (側面ボタン): 表示モード切り替え
 */

#include <M5StickCPlus.h>

// M5StickC Plus 1.1 のGrove端子アナログピン
#define ANGLE_PIN 33  // GPIO36 (ADC1_CH0)

// 表示モード
enum DisplayMode {
  MODE_VALUE,      // 生の値表示
  MODE_PERCENT,    // パーセント表示
  MODE_ANGLE,      // 角度表示（0-270度）
  MODE_MAX
};

DisplayMode currentMode = MODE_VALUE;

// 最小値・最大値トラッキング
int minValue = 4095;
int maxValue = 0;

void setup() {
  M5.begin();

  // アナログピンの設定
  pinMode(ANGLE_PIN, INPUT);
  analogSetAttenuation(ADC_11db);  // 0-3.3V範囲

  // 画面の初期化
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.println("Angle Unit");
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(10, 40);
  M5.Lcd.println("Btn A: Reset Min/Max");
  M5.Lcd.setCursor(10, 60);
  M5.Lcd.println("Btn B: Mode Change");

  Serial.begin(115200);
  Serial.println("M5StickC Plus 1.1 Angle Unit Sample Started");

  delay(2000);
  M5.Lcd.fillScreen(BLACK);
}

void loop() {
  M5.update();

  // アナログ値読み取り（12bit: 0-4095）
  int rawValue = analogRead(ANGLE_PIN);

  // 最小値・最大値の更新
  if (rawValue < minValue) minValue = rawValue;
  if (rawValue > maxValue) maxValue = rawValue;

  // パーセント計算
  float percent = (rawValue / 4095.0) * 100.0;

  // 角度計算（0-270度と仮定）
  float angle = (rawValue / 4095.0) * 270.0;

  // 電圧計算
  float voltage = (rawValue / 4095.0) * 3.3;

  // ボタンA: 最小値/最大値リセット
  if (M5.BtnA.wasPressed()) {
    minValue = 4095;
    maxValue = 0;
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("Reset!");
    delay(500);
    M5.Lcd.fillScreen(BLACK);
    Serial.println("Min/Max values reset");
  }

  // ボタンB: 表示モード切り替え
  if (M5.BtnB.wasPressed()) {
    currentMode = (DisplayMode)((currentMode + 1) % MODE_MAX);
    M5.Lcd.fillScreen(BLACK);
    Serial.print("Mode changed to: ");
    Serial.println(currentMode);
  }

  // 表示更新
  displayAngleData(rawValue, percent, angle, voltage);

  // シリアル出力
  if (millis() % 500 < 50) {  // 約500msごとに出力
    printAngleData(rawValue, percent, angle, voltage);
  }

  delay(50);
}

void displayAngleData(int rawValue, float percent, float angle, float voltage) {
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 10);

  switch (currentMode) {
    case MODE_VALUE:
      M5.Lcd.println("Raw Value");
      M5.Lcd.setTextSize(3);
      M5.Lcd.setCursor(10, 40);
      M5.Lcd.printf("%4d    ", rawValue);

      M5.Lcd.setTextSize(1);
      M5.Lcd.setCursor(10, 80);
      M5.Lcd.printf("Voltage: %.2fV  ", voltage);
      break;

    case MODE_PERCENT:
      M5.Lcd.println("Percentage");
      M5.Lcd.setTextSize(3);
      M5.Lcd.setCursor(10, 40);
      M5.Lcd.printf("%5.1f%%  ", percent);

      M5.Lcd.setTextSize(1);
      M5.Lcd.setCursor(10, 80);
      M5.Lcd.printf("Raw: %d    ", rawValue);
      break;

    case MODE_ANGLE:
      M5.Lcd.println("Angle");
      M5.Lcd.setTextSize(3);
      M5.Lcd.setCursor(10, 40);
      M5.Lcd.printf("%5.1f", angle);
      M5.Lcd.setTextSize(2);
      M5.Lcd.print("o");  // 度記号の代わり

      M5.Lcd.setTextSize(1);
      M5.Lcd.setCursor(10, 80);
      M5.Lcd.printf("Raw: %d    ", rawValue);
      break;
  }

  // 最小値・最大値表示
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(10, 100);
  M5.Lcd.printf("Min:%4d Max:%4d", minValue, maxValue);

  // モード表示
  M5.Lcd.setCursor(10, 120);
  M5.Lcd.printf("Mode: %d/%d", currentMode + 1, MODE_MAX);
}

void printAngleData(int rawValue, float percent, float angle, float voltage) {
  Serial.println("--- Angle Unit Data ---");
  Serial.printf("Raw Value: %d (0-4095)\n", rawValue);
  Serial.printf("Voltage:   %.2f V\n", voltage);
  Serial.printf("Percent:   %.1f %%\n", percent);
  Serial.printf("Angle:     %.1f deg (0-270)\n", angle);
  Serial.printf("Min: %d, Max: %d\n", minValue, maxValue);
  Serial.println();
}
