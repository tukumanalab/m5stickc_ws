/*
 * M5StickC Plus 1.1 RGB LED Unit Sample
 *
 * このサンプルはM5StickC Plus 1.1にRGB LED Unitを接続して
 * カラフルなLED制御を行います。
 *
 * 接続:
 * - RGB LED UnitをM5StickC Plus 1.1のGrove端子（HAT端子）に接続
 * - GPIO32: データ信号ピン（黄色ケーブル）
 *
 * ボタン操作:
 * - ボタンA (M5ボタン): エフェクトモード切り替え
 * - ボタンB (側面ボタン): 明るさ調整
 *
 * 必要なライブラリ:
 * - Adafruit NeoPixel (ライブラリマネージャーからインストール)
 */

#include <M5StickCPlus.h>
#include <Adafruit_NeoPixel.h>

// RGB LED設定
#define LED_PIN 32        // GPIO32 (M5StickC Plus 1.1のGrove端子)
#define NUM_LEDS 3        // LED数（Unit RGBは通常3個）
#define LED_TYPE NEO_GRB  // SK6812/WS2812のカラー順序

// NeoPixelオブジェクト
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, LED_TYPE + NEO_KHZ800);

// エフェクトモード
enum EffectMode {
  MODE_RAINBOW,        // レインボー
  MODE_PULSE,          // パルス（呼吸）
  MODE_COLOR_CYCLE,    // カラーサイクル
  MODE_WAVE,           // ウェーブ
  MODE_SPARKLE,        // スパークル
  MODE_MAX
};

EffectMode currentMode = MODE_RAINBOW;

// 明るさレベル（0-255）
const int BRIGHTNESS_LEVELS = 4;
int brightnessIndex = 2;  // 中間の明るさ
uint8_t brightnessValues[BRIGHTNESS_LEVELS] = {32, 64, 128, 255};

// アニメーション用変数
unsigned long lastUpdate = 0;
uint16_t animationStep = 0;

void setup() {
  M5.begin();

  // NeoPixel初期化
  strip.begin();
  strip.setBrightness(brightnessValues[brightnessIndex]);
  strip.show();  // すべてのLEDをオフ

  // 画面の初期化
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.println("RGB LED Unit");
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(10, 40);
  M5.Lcd.println("Btn A: Mode Change");
  M5.Lcd.setCursor(10, 60);
  M5.Lcd.println("Btn B: Brightness");

  Serial.begin(115200);
  Serial.println("M5StickC Plus 1.1 RGB LED Unit Sample Started");

  delay(2000);
  M5.Lcd.fillScreen(BLACK);
}

void loop() {
  M5.update();

  // ボタンA: エフェクトモード切り替え
  if (M5.BtnA.wasPressed()) {
    currentMode = (EffectMode)((currentMode + 1) % MODE_MAX);
    animationStep = 0;
    M5.Lcd.fillScreen(BLACK);
    Serial.print("Mode changed to: ");
    Serial.println(getModeString(currentMode));
  }

  // ボタンB: 明るさ調整
  if (M5.BtnB.wasPressed()) {
    brightnessIndex = (brightnessIndex + 1) % BRIGHTNESS_LEVELS;
    strip.setBrightness(brightnessValues[brightnessIndex]);
    Serial.print("Brightness: ");
    Serial.println(brightnessValues[brightnessIndex]);
  }

  // エフェクト更新（約50ms間隔）
  if (millis() - lastUpdate > 50) {
    updateEffect();
    lastUpdate = millis();
  }

  // 画面表示更新
  displayStatus();

  delay(10);
}

void updateEffect() {
  switch (currentMode) {
    case MODE_RAINBOW:
      rainbowEffect();
      break;
    case MODE_PULSE:
      pulseEffect();
      break;
    case MODE_COLOR_CYCLE:
      colorCycleEffect();
      break;
    case MODE_WAVE:
      waveEffect();
      break;
    case MODE_SPARKLE:
      sparkleEffect();
      break;
  }
  strip.show();
  animationStep++;
}

// レインボーエフェクト
void rainbowEffect() {
  for (int i = 0; i < NUM_LEDS; i++) {
    uint16_t hue = (animationStep * 256 / NUM_LEDS + i * 65536L / NUM_LEDS) % 65536;
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(hue)));
  }
}

// パルスエフェクト（呼吸）
void pulseEffect() {
  float brightness = (sin(animationStep * 0.05) + 1.0) / 2.0;
  uint8_t value = (uint8_t)(brightness * 255);
  uint32_t color = strip.Color(value, 0, value);  // 紫色
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, color);
  }
}

// カラーサイクルエフェクト
void colorCycleEffect() {
  uint32_t colors[] = {
    strip.Color(255, 0, 0),    // 赤
    strip.Color(0, 255, 0),    // 緑
    strip.Color(0, 0, 255),    // 青
    strip.Color(255, 255, 0),  // 黄
    strip.Color(0, 255, 255),  // シアン
    strip.Color(255, 0, 255)   // マゼンタ
  };
  int colorIndex = (animationStep / 20) % 6;
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, colors[colorIndex]);
  }
}

// ウェーブエフェクト
void waveEffect() {
  for (int i = 0; i < NUM_LEDS; i++) {
    float brightness = (sin((animationStep + i * 30) * 0.1) + 1.0) / 2.0;
    uint8_t value = (uint8_t)(brightness * 255);
    strip.setPixelColor(i, strip.Color(0, value, value));  // シアン系
  }
}

// スパークルエフェクト
void sparkleEffect() {
  // ランダムにLEDを点灯
  if (animationStep % 5 == 0) {
    for (int i = 0; i < NUM_LEDS; i++) {
      if (random(10) < 3) {  // 30%の確率で点灯
        strip.setPixelColor(i, strip.Color(255, 255, 255));
      } else {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
      }
    }
  }
}

String getModeString(EffectMode mode) {
  switch (mode) {
    case MODE_RAINBOW: return "Rainbow";
    case MODE_PULSE: return "Pulse";
    case MODE_COLOR_CYCLE: return "Color Cycle";
    case MODE_WAVE: return "Wave";
    case MODE_SPARKLE: return "Sparkle";
    default: return "Unknown";
  }
}

void displayStatus() {
  // タイトル
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 5);
  M5.Lcd.println("RGB LED");

  // 現在のモード
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(10, 35);
  M5.Lcd.print("Mode: ");
  M5.Lcd.println(getModeString(currentMode) + "          ");

  // 明るさ
  M5.Lcd.setCursor(10, 55);
  M5.Lcd.printf("Brightness: %d/255   ", brightnessValues[brightnessIndex]);
  M5.Lcd.setCursor(10, 70);
  M5.Lcd.printf("Level: %d/%d     ", brightnessIndex + 1, BRIGHTNESS_LEVELS);

  // LED数
  M5.Lcd.setCursor(10, 90);
  M5.Lcd.printf("LEDs: %d      ", NUM_LEDS);

  // 操作説明
  M5.Lcd.setCursor(10, 110);
  M5.Lcd.println("A:Mode B:Bright");
}
