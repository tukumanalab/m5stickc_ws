/*
 * M5StickC Plus 1.1 Infrared Unit Sample
 *
 * このサンプルはM5StickC Plus 1.1にInfrared Unitを接続して
 * 赤外線リモコンの送受信を行います。
 *
 * 接続:
 * - Infrared UnitをM5StickC Plus 1.1のGrove端子（HAT端子）に接続
 * - GPIO32: IR送信ピン
 * - GPIO33: IR受信ピン
 *
 * ボタン操作:
 * - ボタンA (M5ボタン): テストコマンド送信
 * - ボタンB (側面ボタン): 送信コマンド切り替え
 *
 * 必要なライブラリ:
 * - IRremote (ライブラリマネージャーからインストール)
 */

#include <M5StickCPlus.h>
#include <IRremote.hpp>

// IR設定
#define IR_SEND_PIN 32    // GPIO32 (送信)
#define IR_RECEIVE_PIN 33 // GPIO33 (受信)

// NECプロトコルのテストコマンド
#define NEC_ADDRESS 0x00  // アドレス
uint8_t testCommands[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};  // コマンド
int currentCommandIndex = 0;

// 受信データ
unsigned long lastReceiveTime = 0;
uint32_t lastReceivedData = 0;
String lastProtocol = "";

void setup() {
  M5.begin();

  // IR受信初期化
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  // IR送信初期化
  IrSender.begin(IR_SEND_PIN);

  // 画面の初期化
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.println("IR Unit");
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(10, 40);
  M5.Lcd.println("Btn A: Send IR");
  M5.Lcd.setCursor(10, 60);
  M5.Lcd.println("Btn B: Change Cmd");
  M5.Lcd.setCursor(10, 80);
  M5.Lcd.println("Waiting...");

  Serial.begin(115200);
  Serial.println("M5StickC Plus 1.1 Infrared Unit Sample Started");
  Serial.println("IR Receiver ready on GPIO33");
  Serial.println("IR Sender ready on GPIO32");

  delay(2000);
  M5.Lcd.fillScreen(BLACK);
}

void loop() {
  M5.update();

  // ボタンA: IR送信
  if (M5.BtnA.wasPressed()) {
    sendIRCommand();
  }

  // ボタンB: 送信コマンド切り替え
  if (M5.BtnB.wasPressed()) {
    currentCommandIndex = (currentCommandIndex + 1) % (sizeof(testCommands) / sizeof(testCommands[0]));
    M5.Lcd.fillRect(0, 100, 240, 20, BLACK);
    Serial.print("Command changed to: 0x");
    Serial.println(testCommands[currentCommandIndex], HEX);
  }

  // IR受信チェック
  if (IrReceiver.decode()) {
    handleIRReceive();
    IrReceiver.resume();  // 次の受信のための準備
  }

  // 画面表示更新
  displayStatus();

  delay(50);
}

void sendIRCommand() {
  uint8_t command = testCommands[currentCommandIndex];

  // NEC形式で送信
  IrSender.sendNEC(NEC_ADDRESS, command, 0);  // 3番目の引数は繰り返し回数

  M5.Lcd.fillRect(0, 80, 240, 20, BLACK);
  M5.Lcd.setCursor(10, 80);
  M5.Lcd.setTextColor(GREEN, BLACK);
  M5.Lcd.print("Sent!");
  M5.Lcd.setTextColor(WHITE, BLACK);

  Serial.println("=============================");
  Serial.println("IR Command Sent!");
  Serial.print("Address: 0x");
  Serial.println(NEC_ADDRESS, HEX);
  Serial.print("Command: 0x");
  Serial.println(command, HEX);
  Serial.println("=============================");

  delay(500);
}

void handleIRReceive() {
  lastReceiveTime = millis();

  // プロトコル判定
  if (IrReceiver.decodedIRData.protocol == NEC) {
    lastProtocol = "NEC";
    lastReceivedData = IrReceiver.decodedIRData.command;
  } else if (IrReceiver.decodedIRData.protocol == SONY) {
    lastProtocol = "SONY";
    lastReceivedData = IrReceiver.decodedIRData.command;
  } else if (IrReceiver.decodedIRData.protocol == RC5) {
    lastProtocol = "RC5";
    lastReceivedData = IrReceiver.decodedIRData.command;
  } else if (IrReceiver.decodedIRData.protocol == RC6) {
    lastProtocol = "RC6";
    lastReceivedData = IrReceiver.decodedIRData.command;
  } else {
    lastProtocol = "UNKNOWN";
    lastReceivedData = IrReceiver.decodedIRData.decodedRawData;
  }

  Serial.println("=============================");
  Serial.println("IR Signal Received!");
  Serial.print("Protocol: ");
  Serial.println(lastProtocol);
  Serial.print("Address: 0x");
  Serial.println(IrReceiver.decodedIRData.address, HEX);
  Serial.print("Command: 0x");
  Serial.println(IrReceiver.decodedIRData.command, HEX);
  Serial.print("Raw Data: 0x");
  Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);
  Serial.println("=============================");
}

void displayStatus() {
  // タイトル
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 5);
  M5.Lcd.println("IR Unit");

  // 送信コマンド情報
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(10, 35);
  M5.Lcd.println("--- SEND ---");
  M5.Lcd.setCursor(10, 50);
  M5.Lcd.printf("Addr: 0x%02X      ", NEC_ADDRESS);
  M5.Lcd.setCursor(10, 65);
  M5.Lcd.printf("Cmd:  0x%02X (%d/%d)",
                testCommands[currentCommandIndex],
                currentCommandIndex + 1,
                sizeof(testCommands) / sizeof(testCommands[0]));

  // 受信情報
  M5.Lcd.setCursor(10, 85);
  M5.Lcd.println("--- RECEIVE ---");

  if (lastProtocol != "") {
    M5.Lcd.setCursor(10, 100);
    M5.Lcd.printf("Proto: %s      ", lastProtocol.c_str());
    M5.Lcd.setCursor(10, 115);
    M5.Lcd.printf("Data:  0x%02X      ", lastReceivedData);

    // 最後の受信からの経過時間
    unsigned long elapsed = (millis() - lastReceiveTime) / 1000;
    if (elapsed < 60) {
      M5.Lcd.setCursor(150, 100);
      M5.Lcd.printf("%2lus  ", elapsed);
    }
  } else {
    M5.Lcd.setCursor(10, 100);
    M5.Lcd.println("No data          ");
  }
}
