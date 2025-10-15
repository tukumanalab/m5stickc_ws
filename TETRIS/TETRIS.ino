#include <M5StickCPlus.h>

// ANGLE unit pin
int sensorPin = 33;
int last_sensorValue = 2048;  // center value
int cur_sensorValue = 2048;

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

void setup() {
  M5.begin();
  M5.Lcd.setRotation(0);  // Portrait mode (0 or 2)
  pinMode(sensorPin, INPUT);

  // Initialize IMU
  M5.IMU.Init();

  // Initialize Speaker
  M5.Beep.begin();

  randomSeed(analogRead(0));

  M5.Lcd.fillScreen(BLACK);
  drawBoard();
  nextPiece = random(7);
  newPiece();
  displayScore();
  displayNextPiece();
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
      M5.Beep.tone(200, 200);
      delay(200);
      M5.Beep.tone(150, 300);
      M5.Beep.mute();
    }
    lastFall = millis();
    delay(300); // Debounce shake
    return; // Skip rest of loop after drop
  }

  // Handle ANGLE unit for left/right movement
  cur_sensorValue = analogRead(sensorPin);

  // Map ANGLE value to board position
  // Calculate the actual width of current piece
  int pieceWidth = 0;
  int pieceMinX = 4, pieceMaxX = 0;
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (tetrominoes[currentPiece][currentRotation][y][x]) {
        if (x < pieceMinX) pieceMinX = x;
        if (x > pieceMaxX) pieceMaxX = x;
      }
    }
  }
  pieceWidth = pieceMaxX - pieceMinX + 1;

  // ANGLE 0 -> right edge
  // ANGLE 2048 (center) -> middle
  // ANGLE 4095 (max) -> left edge
  int maxX = BOARD_WIDTH - pieceWidth;
  int targetX = map(cur_sensorValue, 0, 4095, maxX, 0) - pieceMinX;
  targetX = constrain(targetX, -pieceMinX, BOARD_WIDTH - 1 - pieceMaxX);

  if (targetX != currentX && millis() - lastMove > 50) {
    if (canMove(targetX, currentY, currentRotation)) {
      erasePiece();
      currentX = targetX;
      drawPiece();
      lastMove = millis();
    }
  }

  // Handle A button for rotation
  if (M5.BtnA.wasPressed() && millis() - lastButtonCheck > buttonDelay) {
    int newRotation = (currentRotation + 1) % 4;
    if (canMove(currentX, currentY, newRotation)) {
      erasePiece();
      currentRotation = newRotation;
      drawPiece();
    }
    lastButtonCheck = millis();
  }

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
        M5.Beep.tone(200, 200);
        delay(200);
        M5.Beep.tone(150, 300);
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
      y++; // Check this line again
    }
  }

  if (linesCleared > 0) {
    score += linesCleared;
    displayScore();
    drawBoard();

    // Play line clear sound
    for (int i = 0; i < linesCleared; i++) {
      M5.Beep.tone(800 + i * 200, 100);
      delay(100);
    }
    M5.Beep.mute();
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
