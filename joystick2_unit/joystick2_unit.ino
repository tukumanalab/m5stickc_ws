/*
 * M5StickC Plus 1.1 Joystick2 Unit Sample
 *
 * M5StickC Plus 1.1にJoystick2 Unitを接続してジョイスティックの
 * X/Y座標とボタンの状態を読み取り表示します。
 *
 * 接続:
 * - Joystick2 UnitをM5StickC Plus 1.1のGrove端子（HAT端子）に接続
 * - I2C通信（SDA: GPIO32, SCL: GPIO33）
 * - I2Cアドレス: 0x63
 *
 * ボタン操作:
 * - ボタンA (M5ボタン): 中央値のキャリブレーション
 * - ボタンB (側面ボタン): 表示モード切り替え
 * - Joystickボタン: ボタン状態の確認
 */

#include <M5StickCPlus.h>
#include <Wire.h>

// M5StickC Plus 1.1 Grove Port pins
#define GROVE_SDA 32
#define GROVE_SCL 33

// Joystick2 Unit I2C設定
#define JOYSTICK2_ADDR 0x63

// レジスタアドレス
#define JOYSTICK2_ADC_VALUE_8BITS_REG 0x10   // 8ビットADC値（X, Y）
#define JOYSTICK2_BUTTON_REG 0x20             // ボタン状態
#define JOYSTICK2_RGB_REG 0x30                // RGB LED制御

// 表示モード
enum DisplayMode {
  MODE_VALUES,      // 数値表示
  MODE_GRAPH,       // グラフ表示
  MODE_CROSSHAIR,   // クロスヘア表示
  MODE_MAX
};

DisplayMode currentMode = MODE_VALUES;

// ジョイスティック値
int16_t joyX = 0;
int16_t joyY = 0;
bool buttonPressed = false;

// キャリブレーション用の中央値
int16_t centerX = 128;
int16_t centerY = 128;

// 前回の状態（画面更新の最適化用）
int16_t prevJoyX = 0;
int16_t prevJoyY = 0;
bool prevButtonPressed = false;
DisplayMode prevMode = MODE_VALUES;

void setup() {
  M5.begin();
  Wire.begin(GROVE_SDA, GROVE_SCL);  // Grove Port: SDA=GPIO32, SCL=GPIO33

  Serial.begin(115200);
  Serial.println("M5StickC Plus 1.1 - Joystick2 Unit Test");
  Serial.printf("I2C: SDA=GPIO%d, SCL=GPIO%d\n", GROVE_SDA, GROVE_SCL);

  // 画面の初期化
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);

  // 初期画面
  displayHeader();

  // I2C接続確認
  Wire.beginTransmission(JOYSTICK2_ADDR);
  if (Wire.endTransmission() == 0) {
    Serial.println("Joystick2 Unit found!");
    setRGBColor(0, 255, 0);  // 緑色で起動を通知
    delay(500);
  } else {
    Serial.println("Joystick2 Unit not found!");
    M5.Lcd.setCursor(10, 40);
    M5.Lcd.setTextColor(RED);
    M5.Lcd.println("Device not found!");
    M5.Lcd.println("Check connection");
    while (1) delay(100);
  }

  // 初期キャリブレーション
  readJoystickValues();
  centerX = joyX;
  centerY = joyY;

  setRGBColor(0, 0, 255);  // 青色で準備完了
}

void loop() {
  M5.update();

  // ジョイスティック値の読み取り
  readJoystickValues();

  // ボタンA: キャリブレーション
  if (M5.BtnA.wasPressed()) {
    centerX = joyX;
    centerY = joyY;
    M5.Lcd.fillScreen(BLACK);
    displayHeader();
    M5.Lcd.setCursor(10, 40);
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("Calibrated!");
    setRGBColor(0, 255, 0);
    delay(500);
    M5.Lcd.fillScreen(BLACK);
    displayHeader();
    prevMode = MODE_MAX;  // 強制再描画
  }

  // ボタンB: 表示モード切り替え
  if (M5.BtnB.wasPressed()) {
    currentMode = (DisplayMode)((currentMode + 1) % MODE_MAX);
    M5.Lcd.fillScreen(BLACK);
    displayHeader();
    prevMode = MODE_MAX;  // 強制再描画
  }

  // 表示更新（変化があった場合のみ）
  if (joyX != prevJoyX || joyY != prevJoyY ||
      buttonPressed != prevButtonPressed || currentMode != prevMode) {
    displayJoystickInfo();
    prevJoyX = joyX;
    prevJoyY = joyY;
    prevButtonPressed = buttonPressed;
    prevMode = currentMode;
  }

  // LED色の更新（ジョイスティックの位置に応じて）
  updateLEDColor();

  delay(50);
}

// ジョイスティック値の読み取り
void readJoystickValues() {
  Wire.beginTransmission(JOYSTICK2_ADDR);
  Wire.write(JOYSTICK2_ADC_VALUE_8BITS_REG);
  Wire.endTransmission(false);

  Wire.requestFrom(JOYSTICK2_ADDR, 2);
  if (Wire.available() >= 2) {
    joyX = Wire.read();
    joyY = Wire.read();
  }

  // ボタン状態の読み取り
  Wire.beginTransmission(JOYSTICK2_ADDR);
  Wire.write(JOYSTICK2_BUTTON_REG);
  Wire.endTransmission(false);

  Wire.requestFrom(JOYSTICK2_ADDR, 1);
  if (Wire.available() >= 1) {
    buttonPressed = (Wire.read() == 0);  // 0=pressed, 1=released
  }

  // デバッグ出力
  Serial.printf("X:%d Y:%d Button:%d\n", joyX, joyY, buttonPressed);
}

// RGB LED色の設定
void setRGBColor(uint8_t r, uint8_t g, uint8_t b) {
  Wire.beginTransmission(JOYSTICK2_ADDR);
  Wire.write(JOYSTICK2_RGB_REG);
  Wire.write(r);
  Wire.write(g);
  Wire.write(b);
  Wire.endTransmission();
}

// ヘッダー表示
void displayHeader() {
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(10, 5);
  M5.Lcd.println("Joystick2 Unit");

  M5.Lcd.setCursor(10, 15);
  M5.Lcd.setTextColor(YELLOW);

  switch (currentMode) {
    case MODE_VALUES:
      M5.Lcd.println("Mode: Values");
      break;
    case MODE_GRAPH:
      M5.Lcd.println("Mode: Graph");
      break;
    case MODE_CROSSHAIR:
      M5.Lcd.println("Mode: Crosshair");
      break;
  }

  M5.Lcd.setTextColor(DARKGREY);
  M5.Lcd.setCursor(10, 70);
  M5.Lcd.println("A:Calib B:Mode");
}

// ジョイスティック情報の表示
void displayJoystickInfo() {
  // 前回の表示領域をクリア
  M5.Lcd.fillRect(0, 30, 240, 35, BLACK);

  // 相対値の計算（中央を0とする）
  int16_t relX = joyX - centerX;
  int16_t relY = joyY - centerY;

  switch (currentMode) {
    case MODE_VALUES:
      displayValuesMode(relX, relY);
      break;
    case MODE_GRAPH:
      displayGraphMode(relX, relY);
      break;
    case MODE_CROSSHAIR:
      displayCrosshairMode(relX, relY);
      break;
  }
}

// 数値表示モード
void displayValuesMode(int16_t relX, int16_t relY) {
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(10, 30);
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.printf("X: %4d (%+4d)", joyX, relX);

  M5.Lcd.setCursor(10, 42);
  M5.Lcd.setTextColor(MAGENTA);
  M5.Lcd.printf("Y: %4d (%+4d)", joyY, relY);

  M5.Lcd.setCursor(10, 54);
  if (buttonPressed) {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.println("Button: PRESSED");
  } else {
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("Button: Released");
  }
}

// グラフ表示モード
void displayGraphMode(int16_t relX, int16_t relY) {
  // X軸バーグラフ
  M5.Lcd.setCursor(10, 32);
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.print("X:");

  int barX = map(joyX, 0, 255, 0, 100);
  M5.Lcd.fillRect(30, 32, 100, 8, BLACK);
  M5.Lcd.fillRect(30, 32, barX, 8, CYAN);
  M5.Lcd.drawRect(30, 32, 100, 8, WHITE);

  // Y軸バーグラフ
  M5.Lcd.setCursor(10, 44);
  M5.Lcd.setTextColor(MAGENTA);
  M5.Lcd.print("Y:");

  int barY = map(joyY, 0, 255, 0, 100);
  M5.Lcd.fillRect(30, 44, 100, 8, BLACK);
  M5.Lcd.fillRect(30, 44, barY, 8, MAGENTA);
  M5.Lcd.drawRect(30, 44, 100, 8, WHITE);

  // ボタン状態
  M5.Lcd.setCursor(10, 56);
  if (buttonPressed) {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.fillCircle(130, 60, 5, RED);
  } else {
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.drawCircle(130, 60, 5, GREEN);
  }
  M5.Lcd.print("BTN");
}

// クロスヘア表示モード
void displayCrosshairMode(int16_t relX, int16_t relY) {
  // 描画領域のクリア
  M5.Lcd.fillRect(0, 25, 135, 45, BLACK);

  // クロスヘア座標系の描画
  int centerScreenX = 67;
  int centerScreenY = 47;
  int radius = 20;

  // 外枠の円
  M5.Lcd.drawCircle(centerScreenX, centerScreenY, radius, DARKGREY);
  M5.Lcd.drawCircle(centerScreenX, centerScreenY, radius/2, DARKGREY);

  // 十字線
  M5.Lcd.drawLine(centerScreenX - radius, centerScreenY,
                  centerScreenX + radius, centerScreenY, DARKGREY);
  M5.Lcd.drawLine(centerScreenX, centerScreenY - radius,
                  centerScreenX, centerScreenY + radius, DARKGREY);

  // ジョイスティック位置の描画
  int posX = centerScreenX + map(relX, -128, 127, -radius, radius);
  int posY = centerScreenY + map(relY, -128, 127, -radius, radius);

  // 位置を円の範囲内に制限
  int dx = posX - centerScreenX;
  int dy = posY - centerScreenY;
  int dist = sqrt(dx*dx + dy*dy);
  if (dist > radius) {
    posX = centerScreenX + (dx * radius) / dist;
    posY = centerScreenY + (dy * radius) / dist;
  }

  // カーソルの描画
  if (buttonPressed) {
    M5.Lcd.fillCircle(posX, posY, 3, RED);
  } else {
    M5.Lcd.fillCircle(posX, posY, 3, GREEN);
  }

  // 数値表示
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(140, 30);
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.printf("X:%+4d", relX);
  M5.Lcd.setCursor(140, 42);
  M5.Lcd.setTextColor(MAGENTA);
  M5.Lcd.printf("Y:%+4d", relY);
}

// LED色の更新（ジョイスティックの位置に応じて）
void updateLEDColor() {
  int16_t relX = joyX - centerX;
  int16_t relY = joyY - centerY;

  if (buttonPressed) {
    // ボタンが押されている場合は緑色
    setRGBColor(0, 255, 0);
  } else {
    // ジョイスティックの位置に応じた色
    uint8_t r = map(constrain(joyX, 0, 255), 0, 255, 0, 255);
    uint8_t g = map(constrain(joyY, 0, 255), 0, 255, 0, 255);
    uint8_t b = 128;  // 青色を固定値で追加

    setRGBColor(r, g, b);
  }
}
