#include <M5StickCPlus.h>
#include <Wire.h>

// Joystick2 Unit I2C settings
#define GROVE_SDA 32
#define GROVE_SCL 33
#define JOYSTICK2_ADDR 0x63
#define JOYSTICK2_ADC_VALUE_8BITS_REG 0x10
#define JOYSTICK2_BUTTON_REG 0x20

// Buzzer settings for M5StickC Plus v1.1
#define BUZZER_PIN 2
#define LEDC_CHANNEL 0
#define LEDC_RESOLUTION 8
#define BUZZER_VOLUME 128 // 0-255, 128 is a good starting point

// Joystick values
int joyX = 128;  // center value (0-255)
int joyY = 128;  // center value (0-255)
bool joyButton = false;
bool lastJoyButton = false;
bool lastJoyDown = false;  // Track if joystick was down in last frame
int centerX = 128;
int centerY = 128;

// Game settings
const int BLOCK_SIZE = 10;
const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;
const int BOARD_X = 1;
const int BOARD_Y = 30;

// Game board
int board[BOARD_HEIGHT][BOARD_WIDTH] = {0};

// Tetromino shapes (4 rotations for each)
const int tetrominoes[7][4][4][4] = {
  // I piece
  {{{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
   {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
   {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}},
   {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}},
  // O piece
  {{{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
   {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
   {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
   {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}},
  // T piece
  {{{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
   {{0,1,0,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
   {{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}},
   {{0,1,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}},
  // S piece
  {{{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
   {{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}},
   {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},
   {{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}},
  // Z piece
  {{{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
   {{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
   {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},
   {{0,1,0,0},{1,1,0,0},{1,0,0,0},{0,0,0,0}}},
  // J piece
  {{{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
   {{0,1,1,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},
   {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}},
   {{0,1,0,0},{0,1,0,0},{1,1,0,0},{0,0,0,0}}},
  // L piece
  {{{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
   {{0,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,0,0}},
   {{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}},
   {{1,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}}}
};

const uint16_t colors[7] = {
  TFT_CYAN,    // I
  TFT_YELLOW,  // O
  TFT_PURPLE,  // T
  TFT_GREEN,   // S
  TFT_RED,     // Z
  TFT_BLUE,    // J
  TFT_ORANGE   // L
};

// Current piece state
int currentPiece = 0;
int currentRotation = 0;
int currentX = 3;
int currentY = 0;
int nextPiece = 0;
int score = 0;
bool gameOver = false;
bool gameOverDisplayed = false;

// Timing
unsigned long lastFall = 0;
unsigned long fallDelay = 500;
unsigned long lastMove = 0;
unsigned long moveDelay = 150;
unsigned long lastButtonCheck = 0;
unsigned long buttonDelay = 200;

// Custom tone functions using LEDC API
void playTone(int frequency, int duration) {
  if (frequency == 0) {
      delay(duration);
      return;
  }
  ledcChangeFrequency(BUZZER_PIN, frequency, LEDC_RESOLUTION);
  ledcWrite(BUZZER_PIN, BUZZER_VOLUME);
  delay(duration);
  ledcWrite(BUZZER_PIN, 0); // Stop tone
}

void setup() {
  M5.begin();
  M5.Lcd.setRotation(0);  // Portrait mode (0 or 2)

  // Initialize I2C for Joystick2 Unit
  Wire.begin(GROVE_SDA, GROVE_SCL);

  // Initialize IMU
  M5.IMU.Init();

  // Initialize Speaker (using LEDC for v1.1)
  ledcAttach(BUZZER_PIN, 1000, LEDC_RESOLUTION);

  randomSeed(analogRead(0));

  M5.Lcd.fillScreen(BLACK);
  drawBoard();
  nextPiece = random(7);
  newPiece();
  displayScore();
  displayNextPiece();

  // Calibrate joystick center position
  readJoystick();
  centerX = joyX;
  centerY = joyY;
}

void loop() {
  if (gameOver) {
    // Draw game over text only once
    if (!gameOverDisplayed) {
      // Draw white background for game over text (taller)
      M5.Lcd.fillRect(15, 95, 90, 60, WHITE);
      M5.Lcd.setCursor(20, 100, 4);
      M5.Lcd.setTextColor(RED);
      M5.Lcd.print("GAME");
      M5.Lcd.setCursor(20, 130, 4);
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

  // Check for shake (IMU acceleration)
  float accX, accY, accZ;
  M5.IMU.getAccelData(&accX, &accY, &accZ);

  // Calculate total acceleration magnitude
  float totalAccel = sqrt(accX * accX + accY * accY + accZ * accZ);

  // If shake detected (strong deviation from 1G), drop piece immediately
  // Increased threshold to prevent false triggers
  if (totalAccel > 2.2 || totalAccel < 0.3) {
    // Drop piece to bottom
    erasePiece();
    while (canMove(currentX, currentY + 1, currentRotation)) {
      currentY++;
    }
    drawPiece();

    // Lock immediately
    lockPiece();
    clearLines();
    newPiece();
    if (!canMove(currentX, currentY, currentRotation)) {
      gameOver = true;
      gameOverDisplayed = false;
      // Play game over sound
      playTone(200, 200);
      playTone(150, 300);
    }
    lastFall = millis();
    delay(300); // Debounce shake
    return; // Skip rest of loop after drop
  }

  // Read Joystick2 Unit
  readJoystick();

  // Handle joystick X-axis for left/right movement
  int relX = joyX - centerX;  // -128 to +127

  // Only move if joystick is pushed significantly (deadzone)
  if (abs(relX) > 30 && millis() - lastMove > moveDelay) {
    int newX = currentX;
    if (relX < -30) {
      newX = currentX - 1;  // Left
    } else if (relX > 30) {
      newX = currentX + 1;  // Right
    }

    if (newX != currentX && canMove(newX, currentY, currentRotation)) {
      erasePiece();
      currentX = newX;
      drawPiece();
      lastMove = millis();
    }
  }

  // Handle joystick button for rotation
  if (joyButton && !lastJoyButton && millis() - lastButtonCheck > buttonDelay) {
    int newRotation = (currentRotation + 1) % 4;
    if (canMove(currentX, currentY, newRotation)) {
      erasePiece();
      currentRotation = newRotation;
      drawPiece();
    }
    lastButtonCheck = millis();
  }
  lastJoyButton = joyButton;

  // Handle M5StickC Plus buttons (A or B) for rotation
  if ((M5.BtnA.wasPressed() || M5.BtnB.wasPressed()) && millis() - lastButtonCheck > buttonDelay) {
    int newRotation = (currentRotation + 1) % 4;
    if (canMove(currentX, currentY, newRotation)) {
      erasePiece();
      currentRotation = newRotation;
      drawPiece();
    }
    lastButtonCheck = millis();
  }

  // Handle joystick Y-axis for hard drop
  int relY = joyY - centerY;
  bool joyDown = (relY > 100);  // Need to push joystick down very hard for hard drop

  if (joyDown && !lastJoyDown) {
    // Joystick pushed down - perform hard drop
    erasePiece();
    while (canMove(currentX, currentY + 1, currentRotation)) {
      currentY++;
    }
    drawPiece();

    // Lock immediately
    lockPiece();
    clearLines();
    newPiece();
    if (!canMove(currentX, currentY, currentRotation)) {
      gameOver = true;
      gameOverDisplayed = false;
      // Play game over sound
      playTone(200, 200);
      playTone(150, 300);
    }
    lastFall = millis();
    lastJoyDown = joyDown;
    delay(100);  // Debounce
    return;  // Skip rest of loop after hard drop
  }
  lastJoyDown = joyDown;

  // Handle falling
  if (millis() - lastFall > fallDelay) {
    if (canMove(currentX, currentY + 1, currentRotation)) {
      erasePiece();
      currentY++;
      drawPiece();
    } else {
      // Lock piece
      lockPiece();
      clearLines();
      newPiece();
      if (!canMove(currentX, currentY, currentRotation)) {
        gameOver = true;
        // Play game over sound
        playTone(200, 200);
        playTone(150, 300);
      }
    }
    lastFall = millis();
  }

  delay(20);
}

void drawBoard() {
  // Draw border
  M5.Lcd.drawRect(BOARD_X - 1, BOARD_Y - 1,
                  BOARD_WIDTH * BLOCK_SIZE + 2,
                  BOARD_HEIGHT * BLOCK_SIZE + 2, WHITE);

  // Draw board contents
  for (int y = 0; y < BOARD_HEIGHT; y++) {
    for (int x = 0; x < BOARD_WIDTH; x++) {
      if (board[y][x] > 0) {
        M5.Lcd.fillRect(BOARD_X + x * BLOCK_SIZE,
                       BOARD_Y + y * BLOCK_SIZE,
                       BLOCK_SIZE - 1, BLOCK_SIZE - 1,
                       colors[board[y][x] - 1]);
      } else {
        // Clear empty cells
        M5.Lcd.fillRect(BOARD_X + x * BLOCK_SIZE,
                       BOARD_Y + y * BLOCK_SIZE,
                       BLOCK_SIZE - 1, BLOCK_SIZE - 1,
                       BLACK);
      }
    }
  }
}

void drawPiece() {
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (tetrominoes[currentPiece][currentRotation][y][x]) {
        int screenX = BOARD_X + (currentX + x) * BLOCK_SIZE;
        int screenY = BOARD_Y + (currentY + y) * BLOCK_SIZE;
        M5.Lcd.fillRect(screenX, screenY, BLOCK_SIZE - 1, BLOCK_SIZE - 1,
                       colors[currentPiece]);
      }
    }
  }
}

void erasePiece() {
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (tetrominoes[currentPiece][currentRotation][y][x]) {
        int screenX = BOARD_X + (currentX + x) * BLOCK_SIZE;
        int screenY = BOARD_Y + (currentY + y) * BLOCK_SIZE;
        M5.Lcd.fillRect(screenX, screenY, BLOCK_SIZE - 1, BLOCK_SIZE - 1, BLACK);
      }
    }
  }
}

bool canMove(int newX, int newY, int rotation) {
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (tetrominoes[currentPiece][rotation][y][x]) {
        int boardX = newX + x;
        int boardY = newY + y;

        // Check boundaries
        if (boardX < 0 || boardX >= BOARD_WIDTH ||
            boardY >= BOARD_HEIGHT) {
          return false;
        }

        // Check collision with locked pieces
        if (boardY >= 0 && board[boardY][boardX] > 0) {
          return false;
        }
      }
    }
  }
  return true;
}

void lockPiece() {
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (tetrominoes[currentPiece][currentRotation][y][x]) {
        int boardY = currentY + y;
        int boardX = currentX + x;
        if (boardY >= 0 && boardY < BOARD_HEIGHT) {
          board[boardY][boardX] = currentPiece + 1;
        }
      }
    }
  }
}

void clearLines() {
  int linesCleared = 0;
  for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
    bool fullLine = true;
    for (int x = 0; x < BOARD_WIDTH; x++) {
      if (board[y][x] == 0) {
        fullLine = false;
        break;
      }
    }

    if (fullLine) {
      linesCleared++;
      
      // Flash effect
      for (int x = 0; x < BOARD_WIDTH; x++) {
        M5.Lcd.fillRect(BOARD_X + x * BLOCK_SIZE, BOARD_Y + y * BLOCK_SIZE, BLOCK_SIZE - 1, BLOCK_SIZE - 1, TFT_WHITE);
      }
      delay(50);

      // Shift lines down
      for (int yy = y; yy > 0; yy--) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
          board[yy][x] = board[yy - 1][x];
        }
      }
      // Clear top line
      for (int x = 0; x < BOARD_WIDTH; x++) {
        board[0][x] = 0;
      }
      y++;
    }
  }

  if (linesCleared > 0) {
    score += linesCleared;
    displayScore();
    drawBoard();

    // --- Text and Sound Effects ---
    if (linesCleared == 1) {
      // Single
      playTone(1200, 80);
    } else if (linesCleared == 2) {
      // Double
      playTone(1000, 70);
      playTone(1400, 90);
      
      // Display "DOUBLE!" text
      M5.Lcd.setCursor(BOARD_X + BOARD_WIDTH * BLOCK_SIZE + 5, BOARD_Y + 80);
      M5.Lcd.setTextColor(TFT_YELLOW);
      M5.Lcd.setTextSize(2);
      M5.Lcd.print("DOUBLE!");
      delay(1000); // Show text for a moment
      M5.Lcd.fillRect(BOARD_X + BOARD_WIDTH * BLOCK_SIZE + 5, BOARD_Y + 80, 100, 20, BLACK);
    } else if (linesCleared == 3) {
      // Triple
      playTone(1000, 70);
      playTone(1400, 70);
      playTone(1800, 90);

      // Display "TRIPLE!" text
      M5.Lcd.setCursor(BOARD_X + BOARD_WIDTH * BLOCK_SIZE + 5, BOARD_Y + 80);
      M5.Lcd.setTextColor(TFT_CYAN);
      M5.Lcd.setTextSize(2);
      M5.Lcd.print("TRIPLE!");
      delay(1000); // Show text for a moment
      M5.Lcd.fillRect(BOARD_X + BOARD_WIDTH * BLOCK_SIZE + 5, BOARD_Y + 80, 100, 20, BLACK);
    } else if (linesCleared >= 4) {
      // TETRIS!
      // Screen flash effect
      M5.Lcd.fillScreen(TFT_WHITE);
      delay(60);
      
      // Redraw the entire UI after the flash
      drawBoard(); 
      displayScore();
      displayNextPiece();

      // Powerful sound effect (descending arpeggio)
      playTone(2100, 50);
      playTone(1900, 50);
      playTone(1700, 50);
      playTone(1500, 150);
      playTone(800, 200);

      // Display "TETRIS!" text
      M5.Lcd.setCursor(BOARD_X + BOARD_WIDTH * BLOCK_SIZE + 5, BOARD_Y + 80);
      M5.Lcd.setTextColor(TFT_RED);
      M5.Lcd.setTextSize(2);
      M5.Lcd.print("TETRIS!");
      delay(1500); // Longer celebration
      M5.Lcd.fillRect(BOARD_X + BOARD_WIDTH * BLOCK_SIZE + 5, BOARD_Y + 80, 100, 20, BLACK);
    }
  }
}

void newPiece() {
  currentPiece = nextPiece;
  nextPiece = random(7);
  currentRotation = 0;
  currentX = 3;
  currentY = 0;
  drawPiece();
  displayNextPiece();
}

void displayScore() {
  // Display on right side of board
  int displayX = BOARD_X + BOARD_WIDTH * BLOCK_SIZE + 5;
  int displayY = BOARD_Y + 5;

  M5.Lcd.fillRect(displayX - 2, displayY, 32, 15, BLACK);
  M5.Lcd.setCursor(displayX, displayY, 2);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.print(score);
}

void displayNextPiece() {
  // Display next piece on right side, below score
  int displayX = BOARD_X + BOARD_WIDTH * BLOCK_SIZE + 5;
  int displayY = BOARD_Y + 30;

  // Clear next piece area
  M5.Lcd.fillRect(displayX - 2, displayY, 32, 30, BLACK);

  // Draw next piece (small size, 6px blocks)
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (tetrominoes[nextPiece][0][y][x]) {
        M5.Lcd.fillRect(displayX + x * 6, displayY + y * 6, 5, 5, colors[nextPiece]);
      }
    }
  }
}

void resetGame() {
  // Clear board
  for (int y = 0; y < BOARD_HEIGHT; y++) {
    for (int x = 0; x < BOARD_WIDTH; x++) {
      board[y][x] = 0;
    }
  }

  score = 0;
  gameOver = false;
  gameOverDisplayed = false;

  M5.Lcd.fillScreen(BLACK);
  drawBoard();
  nextPiece = random(7);
  newPiece();
  displayScore();
  displayNextPiece();
}

// Read Joystick2 Unit values
void readJoystick() {
  // Read X and Y axis values
  Wire.beginTransmission(JOYSTICK2_ADDR);
  Wire.write(JOYSTICK2_ADC_VALUE_8BITS_REG);
  Wire.endTransmission(false);

  Wire.requestFrom(JOYSTICK2_ADDR, 2);
  if (Wire.available() >= 2) {
    joyX = Wire.read();
    joyY = Wire.read();
  }

  // Read button state
  Wire.beginTransmission(JOYSTICK2_ADDR);
  Wire.write(JOYSTICK2_BUTTON_REG);
  Wire.endTransmission(false);

  Wire.requestFrom(JOYSTICK2_ADDR, 1);
  if (Wire.available() >= 1) {
    joyButton = (Wire.read() == 0);  // 0 = pressed
  }
}