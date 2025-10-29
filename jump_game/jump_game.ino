/*
 * M5StickC Plus 1.1 Jump Game
 *
 * A simple runner game where a square block jumps over spikes.
 * Reach the goal to win!
 *
 * Controls:
 * - Button A: Jump
 * - Button B: Jump (alternative)
 *
 * Objective:
 * - Avoid the spikes by jumping over them
 * - Reach the goal (distance 1000) to win
 * - If you hit a spike, game over
 */

#include <M5StickCPlus.h>

// Game settings
const int GROUND_Y = 120;  // Adjusted for M5StickC Plus screen height (135px)
const int PLAYER_SIZE = 12;
const int PLAYER_X = 30;  // Fixed X position of player
const int GRAVITY = 2;
const int JUMP_POWER = 15;
const int SCROLL_SPEED = 3;
const int GOAL_DISTANCE = 1000;

// Player state
int playerY = GROUND_Y - PLAYER_SIZE;
int playerVelocityY = 0;
bool isJumping = false;
bool isOnGround = true;

// Game state
int scrollOffset = 0;
int distance = 0;
bool gameOver = false;
bool gameWon = false;
bool gameOverDisplayed = false;

// Spike obstacles (position relative to scroll)
struct Spike {
  int x;
  bool active;
};

const int MAX_SPIKES = 10;
Spike spikes[MAX_SPIKES];
int nextSpikeIndex = 0;

// Timing
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 30;

// Sprite for smooth rendering (double buffering)
TFT_eSprite sprite = TFT_eSprite(&M5.Lcd);

void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);  // Landscape mode

  // Create sprite for double buffering
  sprite.createSprite(240, 135);
  sprite.setSwapBytes(true);

  // Initialize Speaker
  M5.Beep.begin();

  Serial.begin(115200);
  Serial.println("Jump Game Start!");

  // Initialize spikes
  for (int i = 0; i < MAX_SPIKES; i++) {
    spikes[i].active = false;
  }

  // Create initial spikes
  createSpike(150);
  createSpike(300);
  createSpike(450);
  createSpike(600);
  createSpike(750);

  drawGame();
}

void loop() {
  M5.update();

  if (gameOver || gameWon) {
    if (!gameOverDisplayed) {
      displayGameEnd();
      gameOverDisplayed = true;
    }

    // Restart on button press
    if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed()) {
      resetGame();
    }
    delay(100);
    return;
  }

  // Handle jump - check every frame for responsive controls
  if ((M5.BtnA.wasPressed() || M5.BtnB.wasPressed()) && isOnGround) {
    jump();
  }

  // Update game at fixed intervals
  if (millis() - lastUpdate > UPDATE_INTERVAL) {
    // Update player physics
    updatePlayer();

    // Update scroll
    scrollOffset += SCROLL_SPEED;
    distance = scrollOffset;

    // Check for win
    if (distance >= GOAL_DISTANCE) {
      gameWon = true;
      M5.Beep.tone(1000, 100);
      delay(100);
      M5.Beep.tone(1200, 100);
      delay(100);
      M5.Beep.tone(1500, 200);
      M5.Beep.mute();
    }

    // Update spikes
    updateSpikes();

    // Check collision
    if (checkCollision()) {
      gameOver = true;
      M5.Beep.tone(200, 300);
      delay(300);
      M5.Beep.mute();
    }

    // Draw game
    drawGame();

    lastUpdate = millis();
  }

  delay(10);
}

void jump() {
  playerVelocityY = -JUMP_POWER;
  isJumping = true;
  isOnGround = false;

  // Jump sound
  M5.Beep.tone(800, 50);
  delay(50);
  M5.Beep.mute();
}

void updatePlayer() {
  // Apply gravity
  if (!isOnGround) {
    playerVelocityY += GRAVITY;
    playerY += playerVelocityY;

    // Check ground collision
    if (playerY >= GROUND_Y - PLAYER_SIZE) {
      playerY = GROUND_Y - PLAYER_SIZE;
      playerVelocityY = 0;
      isJumping = false;
      isOnGround = true;
    }
  }
}

void updateSpikes() {
  // Generate new spikes
  for (int i = 0; i < MAX_SPIKES; i++) {
    if (spikes[i].active) {
      // Remove spikes that are off screen
      int screenX = spikes[i].x - scrollOffset;
      if (screenX < -20) {
        spikes[i].active = false;
      }
    }
  }

  // Create new spike if needed
  int furthestSpikeX = 0;
  for (int i = 0; i < MAX_SPIKES; i++) {
    if (spikes[i].active && spikes[i].x > furthestSpikeX) {
      furthestSpikeX = spikes[i].x;
    }
  }

  if (furthestSpikeX < scrollOffset + 300 && distance < GOAL_DISTANCE - 100) {
    createSpike(furthestSpikeX + random(100, 200));
  }
}

void createSpike(int worldX) {
  for (int i = 0; i < MAX_SPIKES; i++) {
    if (!spikes[i].active) {
      spikes[i].x = worldX;
      spikes[i].active = true;
      break;
    }
  }
}

bool checkCollision() {
  int playerLeft = PLAYER_X;
  int playerRight = PLAYER_X + PLAYER_SIZE;
  int playerTop = playerY;
  int playerBottom = playerY + PLAYER_SIZE;

  for (int i = 0; i < MAX_SPIKES; i++) {
    if (spikes[i].active) {
      int screenX = spikes[i].x - scrollOffset;
      int spikeLeft = screenX;
      int spikeRight = screenX + 15;
      int spikeTop = GROUND_Y - 15;
      int spikeBottom = GROUND_Y;

      // Check collision
      if (playerRight > spikeLeft && playerLeft < spikeRight &&
          playerBottom > spikeTop && playerTop < spikeBottom) {
        return true;
      }
    }
  }

  return false;
}

void drawGame() {
  // Draw to sprite (off-screen buffer) to prevent flickering
  sprite.fillScreen(TFT_DARKGREY);

  // Draw sky
  sprite.fillRect(0, 0, 240, GROUND_Y, TFT_CYAN);

  // Draw ground
  sprite.fillRect(0, GROUND_Y, 240, 40, TFT_GREEN);
  sprite.drawLine(0, GROUND_Y, 240, GROUND_Y, TFT_DARKGREEN);

  // Draw goal indicator
  if (distance >= GOAL_DISTANCE - 200) {
    int goalScreenX = (GOAL_DISTANCE - scrollOffset);
    if (goalScreenX >= 0 && goalScreenX < 240) {
      sprite.fillRect(goalScreenX, GROUND_Y - 50, 20, 50, TFT_YELLOW);
      sprite.drawRect(goalScreenX, GROUND_Y - 50, 20, 50, TFT_ORANGE);
      sprite.setTextColor(TFT_RED);
      sprite.setTextSize(1);
      sprite.setCursor(goalScreenX - 5, GROUND_Y - 60);
      sprite.print("GOAL");
    }
  }

  // Draw spikes
  for (int i = 0; i < MAX_SPIKES; i++) {
    if (spikes[i].active) {
      int screenX = spikes[i].x - scrollOffset;
      if (screenX >= -20 && screenX < 240) {
        drawSpike(screenX, GROUND_Y);
      }
    }
  }

  // Draw player
  sprite.fillRect(PLAYER_X, playerY, PLAYER_SIZE, PLAYER_SIZE, TFT_BLUE);
  sprite.drawRect(PLAYER_X, playerY, PLAYER_SIZE, PLAYER_SIZE, TFT_NAVY);

  // Draw distance/score
  sprite.setTextColor(WHITE);
  sprite.setTextSize(2);
  sprite.setCursor(5, 5);
  sprite.print("Dist:");
  sprite.print(distance);
  sprite.print("/");
  sprite.print(GOAL_DISTANCE);

  // Push sprite to screen (this prevents flickering)
  sprite.pushSprite(0, 0);
}

void drawSpike(int x, int groundY) {
  // Draw triangle spike to sprite
  int spikeHeight = 15;
  int spikeWidth = 15;

  // Triangle points
  int x1 = x;
  int y1 = groundY;
  int x2 = x + spikeWidth / 2;
  int y2 = groundY - spikeHeight;
  int x3 = x + spikeWidth;
  int y3 = groundY;

  // Fill triangle
  sprite.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_RED);
  sprite.drawTriangle(x1, y1, x2, y2, x3, y3, TFT_MAROON);
}

void displayGameEnd() {
  if (gameWon) {
    // Win screen - adjusted for 135px screen height
    M5.Lcd.fillRect(40, 35, 160, 70, TFT_YELLOW);
    M5.Lcd.drawRect(40, 35, 160, 70, TFT_ORANGE);
    M5.Lcd.setTextColor(TFT_GREEN);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setCursor(60, 45);
    M5.Lcd.print("YOU");
    M5.Lcd.setCursor(60, 68);
    M5.Lcd.print("WIN!");

    M5.Lcd.setTextColor(TFT_BLACK);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(50, 92);
    M5.Lcd.print("Press A to Retry");
  } else {
    // Game over screen - adjusted for 135px screen height
    M5.Lcd.fillRect(40, 35, 160, 70, TFT_RED);
    M5.Lcd.drawRect(40, 35, 160, 70, TFT_MAROON);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setCursor(50, 45);
    M5.Lcd.print("GAME");
    M5.Lcd.setCursor(50, 68);
    M5.Lcd.print("OVER");

    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(50, 92);
    M5.Lcd.print("Press A to Retry");
  }
}

void resetGame() {
  playerY = GROUND_Y - PLAYER_SIZE;
  playerVelocityY = 0;
  isJumping = false;
  isOnGround = true;
  scrollOffset = 0;
  distance = 0;
  gameOver = false;
  gameWon = false;
  gameOverDisplayed = false;

  // Reset spikes
  for (int i = 0; i < MAX_SPIKES; i++) {
    spikes[i].active = false;
  }

  // Create initial spikes
  createSpike(150);
  createSpike(300);
  createSpike(450);
  createSpike(600);
  createSpike(750);

  drawGame();
}
