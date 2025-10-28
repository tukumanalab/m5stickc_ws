/*
 * M5StickC Plus 1.1 Puyo Puyo Game
 *
 * Classic falling puzzle game with color matching and chain reactions.
 * Uses Joystick2 Unit for control.
 *
 * Connection:
 * - Joystick2 Unit to M5StickC Plus 1.1 Grove port
 * - I2C (SDA: GPIO32, SCL: GPIO33)
 * - I2C Address: 0x63
 *
 * Controls:
 * - Joystick Left/Right: Move puyo pair
 * - Joystick Button: Rotate puyo pair clockwise
 * - Joystick Down (hard): Hard drop
 * - Button A: Reset game (when game over)
 */

#include <M5StickCPlus.h>
#include <Wire.h>

// Joystick2 Unit I2C settings
#define GROVE_SDA 32
#define GROVE_SCL 33
#define JOYSTICK2_ADDR 0x63
#define JOYSTICK2_ADC_VALUE_8BITS_REG 0x10
#define JOYSTICK2_BUTTON_REG 0x20

// Joystick values
int joyX = 128;
int joyY = 128;
bool joyButton = false;
bool lastJoyButton = false;
bool lastJoyDown = false;
int centerX = 128;
int centerY = 128;

// Game settings
const int PUYO_SIZE = 10;
const int BOARD_WIDTH = 6;
const int BOARD_HEIGHT = 12;
const int BOARD_X = 10;
const int BOARD_Y = 40;

// Game board (0=empty, 1-4=puyo colors)
int board[BOARD_HEIGHT][BOARD_WIDTH] = {0};

// Puyo colors
const uint16_t puyoColors[5] = {
  BLACK,      // 0: Empty
  TFT_RED,    // 1: Red
  TFT_BLUE,   // 2: Blue
  TFT_GREEN,  // 3: Green
  TFT_YELLOW  // 4: Yellow
};

// Current falling puyo pair
struct PuyoPair {
  int col;        // Column position
  int row;        // Row position (axis puyo)
  int color1;     // Axis puyo color
  int color2;     // Attached puyo color
  int rotation;   // 0=up, 1=right, 2=down, 3=left
} currentPair;

// Game state
int score = 0;
int chainCount = 0;
bool gameOver = false;
bool gameOverDisplayed = false;

// Timing
unsigned long lastFall = 0;
unsigned long fallDelay = 500;
unsigned long lastMove = 0;
unsigned long moveDelay = 150;
unsigned long lastButtonCheck = 0;
unsigned long buttonDelay = 200;

void setup() {
  M5.begin();
  M5.Lcd.setRotation(0);  // Portrait mode

  // Initialize I2C for Joystick2
  Wire.begin(GROVE_SDA, GROVE_SCL);

  // Initialize Speaker
  M5.Beep.begin();

  randomSeed(analogRead(0));

  M5.Lcd.fillScreen(BLACK);
  drawBoard();
  displayScore();
  newPuyoPair();

  // Calibrate joystick
  readJoystick();
  centerX = joyX;
  centerY = joyY;

  Serial.begin(115200);
  Serial.println("Puyo Puyo Game Start!");
}

void loop() {
  if (gameOver) {
    if (!gameOverDisplayed) {
      M5.Lcd.fillRect(5, 80, 125, 50, WHITE);
      M5.Lcd.setCursor(15, 90, 4);
      M5.Lcd.setTextColor(RED);
      M5.Lcd.print("GAME");
      M5.Lcd.setCursor(15, 115, 4);
      M5.Lcd.print("OVER");
      gameOverDisplayed = true;
    }
    M5.update();
    if (M5.BtnA.wasPressed()) {
      resetGame();
    }
    delay(100);
    return;
  }

  M5.update();

  // Read joystick
  readJoystick();

  // Handle left/right movement
  int relX = joyX - centerX;
  if (abs(relX) > 30 && millis() - lastMove > moveDelay) {
    int newCol = currentPair.col;
    if (relX < -30) {
      newCol = currentPair.col + 1;  // Right
    } else if (relX > 30) {
      newCol = currentPair.col - 1;  // Left
    }

    if (newCol != currentPair.col && canPlacePair(newCol, currentPair.row, currentPair.rotation)) {
      erasePuyoPair();
      currentPair.col = newCol;
      drawPuyoPair();
      lastMove = millis();
    }
  }

  // Handle rotation
  if (joyButton && !lastJoyButton && millis() - lastButtonCheck > buttonDelay) {
    int newRotation = (currentPair.rotation + 1) % 4;
    if (canPlacePair(currentPair.col, currentPair.row, newRotation)) {
      erasePuyoPair();
      currentPair.rotation = newRotation;
      drawPuyoPair();
    }
    lastButtonCheck = millis();
  }
  lastJoyButton = joyButton;

  // Handle M5 buttons for rotation
  if ((M5.BtnA.wasPressed() || M5.BtnB.wasPressed()) && millis() - lastButtonCheck > buttonDelay) {
    int newRotation = (currentPair.rotation + 1) % 4;
    if (canPlacePair(currentPair.col, currentPair.row, newRotation)) {
      erasePuyoPair();
      currentPair.rotation = newRotation;
      drawPuyoPair();
    }
    lastButtonCheck = millis();
  }

  // Handle hard drop
  int relY = joyY - centerY;
  bool joyDown = (relY < -100);

  if (joyDown && !lastJoyDown) {
    erasePuyoPair();
    while (canPlacePair(currentPair.col, currentPair.row + 1, currentPair.rotation)) {
      currentPair.row++;
    }
    drawPuyoPair();
    lockPuyoPair();
    processClearing();
    newPuyoPair();
    if (!canPlacePair(currentPair.col, currentPair.row, currentPair.rotation)) {
      gameOver = true;
      M5.Beep.tone(200, 200);
      delay(200);
      M5.Beep.tone(150, 300);
      M5.Beep.mute();
    }
    lastFall = millis();
    lastJoyDown = joyDown;
    delay(100);
    return;
  }
  lastJoyDown = joyDown;

  // Handle falling
  if (millis() - lastFall > fallDelay) {
    if (canPlacePair(currentPair.col, currentPair.row + 1, currentPair.rotation)) {
      erasePuyoPair();
      currentPair.row++;
      drawPuyoPair();
    } else {
      lockPuyoPair();
      processClearing();
      newPuyoPair();
      if (!canPlacePair(currentPair.col, currentPair.row, currentPair.rotation)) {
        gameOver = true;
        M5.Beep.tone(200, 200);
        delay(200);
        M5.Beep.tone(150, 300);
        M5.Beep.mute();
      }
    }
    lastFall = millis();
  }

  delay(20);
}

// Read Joystick2 Unit
void readJoystick() {
  Wire.beginTransmission(JOYSTICK2_ADDR);
  Wire.write(JOYSTICK2_ADC_VALUE_8BITS_REG);
  Wire.endTransmission(false);

  Wire.requestFrom(JOYSTICK2_ADDR, 2);
  if (Wire.available() >= 2) {
    joyX = Wire.read();
    joyY = Wire.read();
  }

  Wire.beginTransmission(JOYSTICK2_ADDR);
  Wire.write(JOYSTICK2_BUTTON_REG);
  Wire.endTransmission(false);

  Wire.requestFrom(JOYSTICK2_ADDR, 1);
  if (Wire.available() >= 1) {
    joyButton = (Wire.read() == 0);
  }
}

// Draw game board
void drawBoard() {
  // Draw border
  M5.Lcd.drawRect(BOARD_X - 1, BOARD_Y - 1,
                  BOARD_WIDTH * PUYO_SIZE + 2,
                  BOARD_HEIGHT * PUYO_SIZE + 2, WHITE);

  // Draw board contents
  for (int y = 0; y < BOARD_HEIGHT; y++) {
    for (int x = 0; x < BOARD_WIDTH; x++) {
      drawPuyo(x, y, board[y][x]);
    }
  }
}

// Draw a single puyo
void drawPuyo(int col, int row, int color) {
  int screenX = BOARD_X + col * PUYO_SIZE;
  int screenY = BOARD_Y + row * PUYO_SIZE;

  if (color == 0) {
    M5.Lcd.fillRect(screenX, screenY, PUYO_SIZE - 1, PUYO_SIZE - 1, BLACK);
  } else {
    M5.Lcd.fillRect(screenX, screenY, PUYO_SIZE - 1, PUYO_SIZE - 1, puyoColors[color]);
    // Draw eye-like highlight
    M5.Lcd.fillCircle(screenX + 3, screenY + 3, 1, WHITE);
  }
}

// Get attached puyo position based on rotation
void getAttachedPosition(int col, int row, int rotation, int* attachedCol, int* attachedRow) {
  switch (rotation) {
    case 0: // Up
      *attachedCol = col;
      *attachedRow = row - 1;
      break;
    case 1: // Right
      *attachedCol = col + 1;
      *attachedRow = row;
      break;
    case 2: // Down
      *attachedCol = col;
      *attachedRow = row + 1;
      break;
    case 3: // Left
      *attachedCol = col - 1;
      *attachedRow = row;
      break;
  }
}

// Check if puyo pair can be placed
bool canPlacePair(int col, int row, int rotation) {
  // Check axis puyo
  if (col < 0 || col >= BOARD_WIDTH || row < 0 || row >= BOARD_HEIGHT) {
    return false;
  }
  if (board[row][col] != 0) {
    return false;
  }

  // Check attached puyo
  int attachedCol, attachedRow;
  getAttachedPosition(col, row, rotation, &attachedCol, &attachedRow);

  if (attachedCol < 0 || attachedCol >= BOARD_WIDTH || attachedRow < 0 || attachedRow >= BOARD_HEIGHT) {
    return false;
  }
  if (board[attachedRow][attachedCol] != 0) {
    return false;
  }

  return true;
}

// Draw current puyo pair
void drawPuyoPair() {
  int attachedCol, attachedRow;
  getAttachedPosition(currentPair.col, currentPair.row, currentPair.rotation, &attachedCol, &attachedRow);

  drawPuyo(currentPair.col, currentPair.row, currentPair.color1);
  drawPuyo(attachedCol, attachedRow, currentPair.color2);
}

// Erase current puyo pair
void erasePuyoPair() {
  int attachedCol, attachedRow;
  getAttachedPosition(currentPair.col, currentPair.row, currentPair.rotation, &attachedCol, &attachedRow);

  drawPuyo(currentPair.col, currentPair.row, 0);
  drawPuyo(attachedCol, attachedRow, 0);
}

// Lock puyo pair to board
void lockPuyoPair() {
  int attachedCol, attachedRow;
  getAttachedPosition(currentPair.col, currentPair.row, currentPair.rotation, &attachedCol, &attachedRow);

  board[currentPair.row][currentPair.col] = currentPair.color1;
  board[attachedRow][attachedCol] = currentPair.color2;
}

// Create new puyo pair
void newPuyoPair() {
  currentPair.col = BOARD_WIDTH / 2 - 1;
  currentPair.row = 1;
  currentPair.rotation = 0;
  currentPair.color1 = random(1, 5);  // 1-4
  currentPair.color2 = random(1, 5);
  drawPuyoPair();
}

// Process clearing and chains
void processClearing() {
  chainCount = 0;

  while (true) {
    // Apply gravity
    applyGravity();

    // Find and mark connected puyos
    bool cleared[BOARD_HEIGHT][BOARD_WIDTH] = {false};
    bool foundAny = false;

    for (int y = 0; y < BOARD_HEIGHT; y++) {
      for (int x = 0; x < BOARD_WIDTH; x++) {
        if (board[y][x] > 0 && !cleared[y][x]) {
          int count = countConnected(x, y, board[y][x], cleared);
          if (count >= 4) {
            foundAny = true;
          }
        }
      }
    }

    if (!foundAny) break;

    // Clear marked puyos
    int clearedCount = 0;
    for (int y = 0; y < BOARD_HEIGHT; y++) {
      for (int x = 0; x < BOARD_WIDTH; x++) {
        if (cleared[y][x]) {
          board[y][x] = 0;
          clearedCount++;
        }
      }
    }

    chainCount++;
    score += clearedCount * chainCount * 10;

    // Play clear sound
    M5.Beep.tone(800 + chainCount * 200, 100);
    delay(100);
    M5.Beep.mute();

    drawBoard();
    displayScore();
    delay(300);
  }

  if (chainCount > 0) {
    Serial.printf("Chain x%d! Score: %d\n", chainCount, score);
  }
}

// Count connected puyos using flood fill
int countConnected(int x, int y, int color, bool cleared[][BOARD_WIDTH]) {
  if (x < 0 || x >= BOARD_WIDTH || y < 0 || y >= BOARD_HEIGHT) return 0;
  if (board[y][x] != color || cleared[y][x]) return 0;

  cleared[y][x] = true;
  int count = 1;

  count += countConnected(x + 1, y, color, cleared);
  count += countConnected(x - 1, y, color, cleared);
  count += countConnected(x, y + 1, color, cleared);
  count += countConnected(x, y - 1, color, cleared);

  return count;
}

// Apply gravity to make puyos fall
void applyGravity() {
  bool moved = true;
  while (moved) {
    moved = false;
    for (int y = BOARD_HEIGHT - 2; y >= 0; y--) {
      for (int x = 0; x < BOARD_WIDTH; x++) {
        if (board[y][x] > 0 && board[y + 1][x] == 0) {
          board[y + 1][x] = board[y][x];
          board[y][x] = 0;
          moved = true;
        }
      }
    }
    if (moved) {
      drawBoard();
      delay(50);
    }
  }
}

// Display score
void displayScore() {
  int displayX = BOARD_X + BOARD_WIDTH * PUYO_SIZE + 5;
  int displayY = BOARD_Y;

  M5.Lcd.fillRect(displayX - 2, displayY, 50, 30, BLACK);
  M5.Lcd.setCursor(displayX, displayY, 2);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println("Score");
  M5.Lcd.setCursor(displayX, displayY + 15, 2);
  M5.Lcd.println(score);
}

// Reset game
void resetGame() {
  // Clear board
  for (int y = 0; y < BOARD_HEIGHT; y++) {
    for (int x = 0; x < BOARD_WIDTH; x++) {
      board[y][x] = 0;
    }
  }

  score = 0;
  chainCount = 0;
  gameOver = false;
  gameOverDisplayed = false;

  M5.Lcd.fillScreen(BLACK);
  drawBoard();
  displayScore();
  newPuyoPair();
}
