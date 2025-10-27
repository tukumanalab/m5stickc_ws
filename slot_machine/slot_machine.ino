#include <M5StickCPlus.h>

// 0: 停止中, 1: 全て回転中, 2: 1つ目が停止, 3: 2つ目が停止, 4: 3つ目が停止
int stopState = 0;

// スロットの出目
int slot1 = 0;
int slot2 = 0;
int slot3 = 0;

// slot2の更新タイミング
unsigned long lastSlot2UpdateTime = 0;
int slot2Interval = 200; // slot2の更新間隔（ミリ秒）

bool flashState = false; // 点滅状態の管理
bool winAnimationPlayed = false; // 勝利アニメーションが再生されたか

// 勝利アニメーション
void playWinAnimation() {
  // メロディの定義
  int melody[] = {262, 330, 392, 523, 0}; // C4, E4, G4, C5, pause
  int durations[] = {100, 100, 100, 200, 100};

  M5.Lcd.fillScreen(BLACK);

  for (int i = 0; i < 5; i++) {
    // ノートを再生
    if (melody[i] > 0) {
      M5.Beep.tone(melody[i]);
    } else {
      M5.Beep.mute();
    }

    // 花火を描画
    for (int j = 0; j < 50; j++) {
      int x = random(M5.Lcd.width());
      int y = random(M5.Lcd.height());
      uint16_t color = random(0xFFFF);
      M5.Lcd.drawPixel(x, y, color);
    }

    delay(durations[i]);
  }
  M5.Beep.mute();
  delay(1000); // 少し待つ
  M5.Lcd.fillScreen(BLACK); // 画面をクリアして数字を再表示
}

void setup() {
  // M5StickCの初期化
  M5.begin();
  // LCDの初期化
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(5);
  M5.Lcd.setCursor(40, 40);
  M5.Lcd.println("SLOT");
}

void loop() {
  // M5StickCの状態を更新
  M5.update();

  // ボタンAが押されたら状態を遷移
  if (M5.BtnA.wasPressed()) {
    stopState++;
    if (stopState > 4) {
      stopState = 1; // 全てのスロットが停止したら、次は全て回転から
      winAnimationPlayed = false; // アニメーションフラグをリセット
    }
  }

  // 状態に応じてスロットを更新
  if (stopState >= 1) { // 回転開始
    if (stopState < 2) { // 1つ目も回転
      slot1 = random(1, 10);
    }
    if (stopState < 3) { // 2つ目も回転
      if (millis() - lastSlot2UpdateTime > slot2Interval) {
        lastSlot2UpdateTime = millis();
        slot2++;
        if (slot2 > 9) {
          slot2 = 1;
        }
      }
    }
    if (stopState < 4) { // 3つ目も回転
      slot3 = random(1, 10);
    }
  }

  // 勝利判定とアニメーション
  if (stopState == 4 && slot1 == slot2 && slot2 == slot3 && !winAnimationPlayed) {
    playWinAnimation();
    winAnimationPlayed = true;
  }

  // 画面に表示
  if (stopState > 0) {
    uint16_t bgColor = BLACK;
    // リーチ状態の判定
    if (stopState == 3 && slot1 == slot2) {
      flashState = !flashState;
      bgColor = flashState ? YELLOW : BLACK;
    } else {
      flashState = false;
    }

    // 描画エリアをクリア
    M5.Lcd.fillRect(0, 40, M5.Lcd.width(), 40, bgColor);

    // 1つ目のスロット（赤）
    M5.Lcd.setCursor(40, 40);
    M5.Lcd.setTextColor(RED, bgColor);
    M5.Lcd.printf("%d", slot1);

    // 2つ目のスロット（青）
    M5.Lcd.setCursor(90, 40);
    M5.Lcd.setTextColor(BLUE, bgColor);
    M5.Lcd.printf("%d", slot2);

    // 3つ目のスロット（緑）
    M5.Lcd.setCursor(140, 40);
    M5.Lcd.setTextColor(GREEN, bgColor);
    M5.Lcd.printf("%d", slot3);
  }
  
  // 100ミリ秒待つ
  delay(100);
}
