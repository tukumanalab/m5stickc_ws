/*
 * M5StickC Plus 1.1 PIR Hat Sample
 *
 * このサンプルはM5StickC Plus 1.1にPIR Hat（人感センサー）を接続して
 * 人の動きを検知し表示します。
 *
 * 接続:
 * - PIR HatをM5StickC Plus 1.1のGrove端子（HAT端子）に接続
 * - GPIO36: PIRセンサー出力（デジタル入力）
 *
 * ボタン操作:
 * - ボタンA (M5ボタン): 検出カウンターをリセット
 * - ボタンB (側面ボタン): 感度設定の表示切り替え
 */

#include <M5StickCPlus.h>

// M5StickC Plus 1.1 のGrove端子デジタルピン
#define PIR_PIN 36  // GPIO36

// 検出状態
bool motionDetected = false;
bool lastMotionState = false;
bool lastDisplayState = false;  // 表示状態の記録
unsigned long lastDetectionTime = 0;
unsigned long detectionCount = 0;
unsigned long sessionStartTime = 0;

// 検出履歴（最近5回分）
const int HISTORY_SIZE = 5;
String detectionHistory[HISTORY_SIZE];
int historyIndex = 0;

void setup() {
  M5.begin();

  // PIRピンの設定
  pinMode(PIR_PIN, INPUT);

  // 画面の初期化
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.println("PIR Hat");
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(10, 40);
  M5.Lcd.println("Btn A: Reset Counter");
  M5.Lcd.setCursor(10, 60);
  M5.Lcd.println("Initializing...");

  Serial.begin(115200);
  Serial.println("M5StickC Plus 1.1 PIR Hat Sample Started");

  // 履歴の初期化
  for (int i = 0; i < HISTORY_SIZE; i++) {
    detectionHistory[i] = "";
  }

  sessionStartTime = millis();

  delay(2000);
  M5.Lcd.fillScreen(BLACK);
  Serial.println("PIR sensor ready. Waiting for motion...");
}

void loop() {
  M5.update();

  // PIRセンサーの読み取り
  motionDetected = digitalRead(PIR_PIN);

  // 立ち上がりエッジ検出（新しい動きを検知）
  if (motionDetected && !lastMotionState) {
    detectionCount++;
    lastDetectionTime = millis();

    // 検出履歴に追加
    unsigned long elapsedSec = (millis() - sessionStartTime) / 1000;
    String timeStr = String(elapsedSec) + "s";
    detectionHistory[historyIndex] = timeStr;
    historyIndex = (historyIndex + 1) % HISTORY_SIZE;

    Serial.println("=============================");
    Serial.println("MOTION DETECTED!");
    Serial.printf("Detection Count: %lu\n", detectionCount);
    Serial.printf("Time: %s\n", timeStr.c_str());
    Serial.println("=============================");

    // ブザーで通知（ブザーがある場合）
    #ifdef BUZZER_PIN
    M5.Beep.tone(2000, 100);
    #endif
  }

  lastMotionState = motionDetected;

  // ボタンA: カウンターリセット
  if (M5.BtnA.wasPressed()) {
    detectionCount = 0;
    sessionStartTime = millis();
    for (int i = 0; i < HISTORY_SIZE; i++) {
      detectionHistory[i] = "";
    }
    historyIndex = 0;
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("Reset!");
    delay(500);
    M5.Lcd.fillScreen(BLACK);
    Serial.println("Counter and history reset");
  }

  // 表示更新
  displayPIRStatus();

  delay(50);
}

void displayPIRStatus() {
  // タイトル
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 5);
  M5.Lcd.println("PIR Sensor");

  // 検出状態表示（大きく）- 状態が変わったときだけクリアして再描画
  if (motionDetected != lastDisplayState) {
    M5.Lcd.fillRect(10, 30, 220, 30, BLACK);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setCursor(10, 30);
    if (motionDetected) {
      M5.Lcd.setTextColor(RED, BLACK);
      M5.Lcd.println("DETECTED");
    } else {
      M5.Lcd.setTextColor(GREEN, BLACK);
      M5.Lcd.println("No Motion");
    }
    M5.Lcd.setTextColor(WHITE, BLACK);
    lastDisplayState = motionDetected;
  }

  // 検出回数
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(10, 65);
  M5.Lcd.printf("Count: %lu       ", detectionCount);

  // 最後の検出からの経過時間
  if (detectionCount > 0) {
    unsigned long elapsedSinceDetection = (millis() - lastDetectionTime) / 1000;
    M5.Lcd.setCursor(10, 80);
    M5.Lcd.printf("Last: %lus ago   ", elapsedSinceDetection);
  }

  // セッション時間
  unsigned long sessionTime = (millis() - sessionStartTime) / 1000;
  M5.Lcd.setCursor(10, 95);
  M5.Lcd.printf("Session: %lus   ", sessionTime);

  // 検出履歴（最近3件）
  M5.Lcd.setCursor(10, 110);
  M5.Lcd.println("Recent:");
  String historyStr = "";
  int count = 0;
  for (int i = HISTORY_SIZE - 1; i >= 0 && count < 3; i--) {
    int idx = (historyIndex - 1 - i + HISTORY_SIZE) % HISTORY_SIZE;
    if (detectionHistory[idx] != "") {
      historyStr += detectionHistory[idx] + " ";
      count++;
    }
  }
  M5.Lcd.setCursor(50, 110);
  M5.Lcd.printf("%s           ", historyStr.c_str());
}
