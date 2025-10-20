/*
 * M5StickC Plus 1.1 Light Unit Sample
 *
 * このサンプルはM5StickC Plus 1.1にLight Unitを接続して
 * 光センサー（フォトレジスタ）の値を読み取り表示します。
 *
 * 接続:
 * - Light UnitをM5StickC Plus 1.1のGrove端子（HAT端子）に接続
 * - GPIO33 (ADC): アナログ入力
 *
 * ボタン操作:
 * - ボタンA (M5ボタン): 最小値/最大値をリセット
 * - ボタンB (側面ボタン): 表示モード切り替え
 */

#include <M5StickCPlus.h>

// M5StickC Plus 1.1 のGrove端子アナログピン
#define LIGHT_PIN 33  // GPIO33 (ADC1_CH5)

// 表示モード
enum DisplayMode {
  MODE_VALUE,      // 生の値表示
  MODE_PERCENT,    // パーセント表示
  MODE_BAR,        // バーグラフ表示
  MODE_MAX
};

DisplayMode currentMode = MODE_VALUE;

// 最小値・最大値トラッキング
int minValue = 4095;
int maxValue = 0;

// 移動平均フィルタ用
const int SAMPLE_SIZE = 10;
int samples[SAMPLE_SIZE];
int sampleIndex = 0;
long sampleSum = 0;

void setup() {
  M5.begin();

  // アナログピンの設定
  pinMode(LIGHT_PIN, INPUT);
  analogSetAttenuation(ADC_11db);  // 0-3.3V範囲

  // サンプル配列の初期化
  for (int i = 0; i < SAMPLE_SIZE; i++) {
    samples[i] = 0;
  }

  // 画面の初期化
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.println("Light Unit");
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(10, 40);
  M5.Lcd.println("Btn A: Reset Min/Max");
  M5.Lcd.setCursor(10, 60);
  M5.Lcd.println("Btn B: Mode Change");

  Serial.begin(115200);
  Serial.println("M5StickC Plus 1.1 Light Unit Sample Started");

  delay(2000);
  M5.Lcd.fillScreen(BLACK);
}

void loop() {
  M5.update();

  // アナログ値読み取り（12bit: 0-4095）
  int rawValue = analogRead(LIGHT_PIN);

  // 移動平均フィルタ
  sampleSum = sampleSum - samples[sampleIndex] + rawValue;
  samples[sampleIndex] = rawValue;
  sampleIndex = (sampleIndex + 1) % SAMPLE_SIZE;
  int smoothedValue = sampleSum / SAMPLE_SIZE;

  // 最小値・最大値の更新
  if (smoothedValue < minValue) minValue = smoothedValue;
  if (smoothedValue > maxValue) maxValue = smoothedValue;

  // パーセント計算（値が大きいほど明るい）
  float percent = (smoothedValue / 4095.0) * 100.0;

  // 電圧計算
  float voltage = (smoothedValue / 4095.0) * 3.3;

  // 明るさレベル判定
  String lightLevel = getLightLevel(smoothedValue);

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
  displayLightData(smoothedValue, percent, voltage, lightLevel);

  // シリアル出力
  if (millis() % 500 < 50) {  // 約500msごとに出力
    printLightData(smoothedValue, percent, voltage, lightLevel);
  }

  delay(50);
}

String getLightLevel(int value) {
  if (value < 500) return "Very Dark";
  else if (value < 1000) return "Dark";
  else if (value < 2000) return "Dim";
  else if (value < 3000) return "Normal";
  else if (value < 3500) return "Bright";
  else return "Very Bright";
}

void displayLightData(int smoothedValue, float percent, float voltage, String lightLevel) {
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 10);

  switch (currentMode) {
    case MODE_VALUE:
      M5.Lcd.println("Light Value");
      M5.Lcd.setTextSize(3);
      M5.Lcd.setCursor(10, 40);
      M5.Lcd.printf("%4d    ", smoothedValue);

      M5.Lcd.setTextSize(1);
      M5.Lcd.setCursor(10, 75);
      M5.Lcd.printf("Level: %s       ", lightLevel.c_str());
      M5.Lcd.setCursor(10, 90);
      M5.Lcd.printf("Voltage: %.2fV  ", voltage);
      break;

    case MODE_PERCENT:
      M5.Lcd.println("Brightness");
      M5.Lcd.setTextSize(3);
      M5.Lcd.setCursor(10, 40);
      M5.Lcd.printf("%5.1f%%  ", percent);

      M5.Lcd.setTextSize(1);
      M5.Lcd.setCursor(10, 75);
      M5.Lcd.printf("Level: %s       ", lightLevel.c_str());
      M5.Lcd.setCursor(10, 90);
      M5.Lcd.printf("Raw: %d    ", smoothedValue);
      break;

    case MODE_BAR:
      M5.Lcd.println("Bar Graph");

      // バーグラフ表示
      int barWidth = (int)((smoothedValue / 4095.0) * 220);
      M5.Lcd.fillRect(10, 40, 220, 30, BLACK);
      M5.Lcd.drawRect(10, 40, 220, 30, WHITE);

      // 色分け
      uint16_t barColor;
      if (smoothedValue < 1000) barColor = BLUE;
      else if (smoothedValue < 2500) barColor = GREEN;
      else barColor = YELLOW;

      M5.Lcd.fillRect(12, 42, barWidth - 4, 26, barColor);

      M5.Lcd.setTextSize(1);
      M5.Lcd.setCursor(10, 80);
      M5.Lcd.printf("Value: %d (%.1f%%)  ", smoothedValue, percent);
      M5.Lcd.setCursor(10, 95);
      M5.Lcd.printf("Level: %s       ", lightLevel.c_str());
      break;
  }

  // 最小値・最大値表示
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(10, 110);
  M5.Lcd.printf("Min:%4d Max:%4d", minValue, maxValue);

  // モード表示
  M5.Lcd.setCursor(10, 125);
  M5.Lcd.printf("Mode: %d/%d", currentMode + 1, MODE_MAX);
}

void printLightData(int smoothedValue, float percent, float voltage, String lightLevel) {
  Serial.println("--- Light Unit Data ---");
  Serial.printf("Raw Value: %d (0-4095)\n", smoothedValue);
  Serial.printf("Voltage:   %.2f V\n", voltage);
  Serial.printf("Percent:   %.1f %%\n", percent);
  Serial.printf("Level:     %s\n", lightLevel.c_str());
  Serial.printf("Min: %d, Max: %d\n", minValue, maxValue);
  Serial.println();
}
