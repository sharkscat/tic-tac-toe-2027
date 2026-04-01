#include <Adafruit_NeoPixel.h>
#define PIN 2
#define MATRIX_WIDTH 8
#define MATRIX_HEIGHT 8
#define NUM_PIXELS (MATRIX_WIDTH * MATRIX_HEIGHT)
#define JOYX A0
#define JOYY A1
#define BTN_PIN 4
Adafruit_NeoPixel strip(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

uint16_t getPixelIndex(int x, int y) {
  int flippedY = (MATRIX_HEIGHT - 1) - y;
  if (flippedY % 2 == 0) {
    return flippedY * MATRIX_WIDTH + (MATRIX_WIDTH - 1 - x);
  } else {
    return flippedY * MATRIX_WIDTH + x;
  }
}

//specifically for space invaders
uint16_t getPixelIndexSI(int x, int y) {
  // rotate 90 degrees clockwise
  int rotX = y;
  int rotY = (MATRIX_WIDTH - 1) - x;
  return getPixelIndex(rotX, rotY);
}

//guess who just learned how to use a struct :o
struct Enemy {
  int x, y;       // position
  int type;       // 1 = 1x1 red (4HP), 2 = 2x1 yellow (8HP)
  int hp;
  bool alive;
};
Enemy enemies[6];


//globals ttt/global
int currentPlayer = 1;
int board[3][3];
int playerToCheck;
int cursorX = 0;
int cursorY = 0;
unsigned long lastMoveTime = 0;
bool btnLastState = HIGH;
bool gameOn = true;
unsigned long btnPressTime = 0;
bool holdTriggered = false;
int btnPressX = cursorX;
int btnPressY = cursorY;

//menu globals

int pressCount = 0;
unsigned long lastPressTime = 0;
int powerPressCount = 0;
unsigned long lastPowerPressTime = 0;
int games[] = {1, 2, 3, 4};  // game IDs
int numGames = 4;
int menuIndex = 0;  // currently selected game


//globals for SI
// game state
int gameState = 0;  // 1=tic tac toe, 2=space invaders, 0=menu

// space invaders player
int playerX = 2;
const int playerY = 6;

// space invaders inventory
int inventorySlots[3] = {0, 0, 0};  // 0=empty, 1=cyan, 2=purple
int inventory = 0;  // tracks how many stored

// space invaders power up
bool powerUpActive = false;
unsigned long powerUpEndTime = 0;
bool powerUpOnBoard = false;
int powerUpX = 0;
int powerUpY = 0;
unsigned long lastPowerUpFall = 0;
int powerUpType = 0;    // 0=none, 1=cyan firerate, 2=purple damage
int damageStacks = 0;   // tracks purple stacks
int shotDamage = 1;     // current shot damage
bool purpleActive = false;
bool btnJustReleased = false;
unsigned long purpleEndTime = 0;
// space invaders shooting
unsigned long lastShotTime = 0;
int fireRate = 300;

// space invaders level
int currentLevel = 1;
int loopCount = 0;
bool isBossLevel = false;
int bossPixelHP[2][4];  // 2 rows, 4 cols, each starts at 10
int baseEnemySpeed = 800;
// boss
int bossX = 2;
int bossY = 0;
int bossDir = 1;
unsigned long lastBossMove = 0;

int shotX = -1;  // -1 means no active shot
int shotY = -1;
unsigned long lastShotMove = 0;
int enemyDir = 1;        // 1 = moving right, -1 = moving left
unsigned long lastEnemyMove = 0;

// snake globals
int snakeX[64];
int snakeY[64];
int snakeLength = 3;
int snakeDirX = 1;
int snakeDirY = 0;
int foodX = 0;
int foodY = 0;
unsigned long lastSnakeMove = 0;
int snakeSpeed = 300;

// connect 4 globals
int c4Board[7][6];  // [row][col] 7 rows, 6 cols
int c4Player = 1;
int c4CursorCol = 0;
unsigned long lastC4Blink = 0;
bool c4BlinkState = false;
unsigned long lastC4Move = 0;

//easter egg time
int konamiCode[] = {0, 0, 1, 1, 2, 3, 2, 3};
int inputBuffer[8] = {-1,-1,-1,-1,-1,-1,-1,-1};
int inputIndex = 0;

//draws the white grid for the game
void drawGrid() {
  for (int i = 0; i < 8; i++) {
    strip.setPixelColor(getPixelIndex(2, i), strip.Color(20, 20, 20));
    strip.setPixelColor(getPixelIndex(5, i), strip.Color(20, 20, 20));
    strip.setPixelColor(getPixelIndex(i, 2), strip.Color(20, 20, 20));
    strip.setPixelColor(getPixelIndex(i, 5), strip.Color(20, 20, 20));
  }
}
void spawnFood() {
  bool valid = false;
  while (!valid) {
    foodX = random(0, 8);
    foodY = random(0, 8);
    valid = true;
    for (int i = 0; i < snakeLength; i++) {
      if (snakeX[i] == foodX && snakeY[i] == foodY) {
        valid = false;
        break;
      }
    }
  }
}

void initSnake() {
  snakeLength = 3;
  snakeDirX = 1;
  snakeDirY = 0;
  snakeSpeed = 300;  
  snakeX[0] = 4; snakeY[0] = 4;
  snakeX[1] = 3; snakeY[1] = 4;
  snakeX[2] = 2; snakeY[2] = 4;
  spawnFood();
}

void initC4() {
  memset(c4Board, 0, sizeof(c4Board));
  c4Player = 1;
  c4CursorCol = 0;
}

void drawC4() {
  strip.clear();

  // draw border walls
  for (int row = 0; row < 8; row++) {
    strip.setPixelColor(getPixelIndexSI(0, row), strip.Color(20, 20, 20));
    strip.setPixelColor(getPixelIndexSI(7, row), strip.Color(20, 20, 20));
  }

  // draw play area
  for (int row = 0; row < 7; row++) {
    for (int col = 0; col < 6; col++) {
      if (c4Board[row][col] == 1)
        strip.setPixelColor(getPixelIndexSI(col + 1, row + 1), strip.Color(100, 0, 0));
      else if (c4Board[row][col] == 2)
        strip.setPixelColor(getPixelIndexSI(col + 1, row + 1), strip.Color(100, 100, 0));
    }
  }


  uint32_t selectorColor = (c4Player == 1) ? strip.Color(100, 0, 0) : strip.Color(100, 100, 0);
  strip.setPixelColor(getPixelIndexSI(c4CursorCol + 1, 0), selectorColor);

  strip.show();
}

bool checkC4Win(int player) {
  // horizontal
  for (int r = 0; r < 7; r++)
    for (int c = 0; c <= 2; c++) {
      if (c4Board[r][c]==player && c4Board[r][c+1]==player && 
          c4Board[r][c+2]==player && c4Board[r][c+3]==player) return true;
    }
  // vertical
  for (int r = 0; r <= 3; r++)
    for (int c = 0; c < 6; c++) {
      if (c4Board[r][c]==player && c4Board[r+1][c]==player && 
          c4Board[r+2][c]==player && c4Board[r+3][c]==player) return true;
    }
  // diagonal down-right
  for (int r = 0; r <= 3; r++)
    for (int c = 0; c <= 2; c++) {
      if (c4Board[r][c]==player && c4Board[r+1][c+1]==player && 
          c4Board[r+2][c+2]==player && c4Board[r+3][c+3]==player) return true;
    }
  // diagonal down-left
  for (int r = 0; r <= 3; r++)
    for (int c = 3; c < 6; c++) {
      if (c4Board[r][c]==player && c4Board[r+1][c-1]==player && 
          c4Board[r+2][c-2]==player && c4Board[r+3][c-3]==player) return true;
    }
  return false;
}



void resetBoard() {
  memset(board, 0, sizeof(board));
  cursorX = 0;
  cursorY = 0;
  currentPlayer = 1;
  
}

void setCellColor(int cellX, int cellY, uint32_t color) {
  int pixelX = cellX * 3;
  int pixelY = cellY * 3;
  for (int row = 0; row < 2; row++) {
    for (int col = 0; col < 2; col++) {
      uint16_t idx = getPixelIndex(pixelX + col, pixelY + row);
      strip.setPixelColor(idx, color);
    }
  }
}


void spawnEnemies() {
  // clear all enemies first
  if (isBossLevel) return;
  for (int i = 0; i < 6; i++) {
    enemies[i].alive = false;
  }

  // randomize mix of 1x1 and 2x1 enemies
  for (int i = 0; i < 6; i++) {
    enemies[i].alive = true;
    enemies[i].x = i;  // spread across columns
    enemies[i].y = 0;  // start at top
    enemies[i].type = random(1, 3);  // randomly 1 or 2
    enemies[i].hp = (enemies[i].type == 1) ? 2 : 4;
  }
}
void drawInventory() {
  for (int i = 0; i < 3; i++) {
    if (inventorySlots[i] == 1)
      strip.setPixelColor(getPixelIndexSI(5 + i, 7), strip.Color(0, 80, 80));   // cyan
    else if (inventorySlots[i] == 2)
      strip.setPixelColor(getPixelIndexSI(5 + i, 7), strip.Color(80, 0, 80));   // purple
    else
      strip.setPixelColor(getPixelIndexSI(5 + i, 7), strip.Color(0, 0, 0));     // empty
  }
}
void setup() {
  pinMode(BTN_PIN, INPUT_PULLUP);
  strip.begin();
  strip.clear();
  strip.setBrightness(15);
  delay(500);
  strip.show();
  Serial.begin(9600);
  btnLastState = digitalRead(BTN_PIN);
  initBoss();
  spawnEnemies();
  initSnake();
  initC4();
}


void showSMPTE() {
  uint32_t bars[8] = {
    strip.Color(80, 80, 80),  // white
    strip.Color(80, 80, 0),   // yellow
    strip.Color(0, 80, 80),   // cyan
    strip.Color(0, 80, 0),    // green
    strip.Color(80, 0, 80),   // magenta
    strip.Color(80, 0, 0),    // red
    strip.Color(0, 0, 80),    // blue
    strip.Color(80, 80, 80)   // white repeat
  };
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      strip.setPixelColor(getPixelIndex(col, row), bars[row]);
    }
  }
  strip.show();
  delay(5000);
  strip.clear();
  strip.show();
  delay(100);

}
void recordInput(int dir) {
  inputBuffer[inputIndex % 8] = dir;
  inputIndex++;
  bool match = true;
  for (int i = 0; i < 8; i++) {
    if (inputBuffer[(inputIndex + i) % 8] != konamiCode[i]) {
      match = false;
      break;
    }
  }
  Serial.print("Match check: ");
  Serial.println(match ? "TRUE" : "FALSE");
  if (match) showSMPTE();
}


void ticTacToeLoop(unsigned long now, int jx, int jy, bool btnState) {

  if (btnJustReleased && !holdTriggered) {
  if (board[cursorY][cursorX] == 0) {
    board[cursorY][cursorX] = currentPlayer;
    currentPlayer = 3 - currentPlayer;
    btnJustReleased = false;
  }
}

  if (now - lastMoveTime > 250) {
    if (jx < 400) {
      cursorX = min(cursorX + 1, 2);
      lastMoveTime = now;
      recordInput(1);
    } else if (jx > 624) {
      cursorX = max(cursorX - 1, 0);
      lastMoveTime = now;
      recordInput(0);
    } else if (jy < 400) {
      cursorY = min(cursorY + 1, 2);
      lastMoveTime = now;
      recordInput(3);
    } else if (jy > 624) {
      cursorY = max(cursorY - 1, 0);
      lastMoveTime = now;
      recordInput(2);
    }
  }

  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      if (col == cursorX && row == cursorY) {
        if (currentPlayer == 1)
          setCellColor(col, row, strip.Color(0, 80, 0));
        else
          setCellColor(col, row, strip.Color(80, 60, 0));
      } else if (board[row][col] == 1) {
        setCellColor(col, row, strip.Color(100, 0, 0));
      } else if (board[row][col] == 2) {
        setCellColor(col, row, strip.Color(0, 0, 100));
      } else {
        setCellColor(col, row, strip.Color(0, 0, 0));
      }
    }
  }

  for (playerToCheck = 1; playerToCheck <= 2; playerToCheck++) {
    bool rowWin = true;
    bool colWin = true;
    bool diagWin1 = true;
    bool diagWin2 = true;

    for (int i = 0; i < 3; i++)
      if (board[cursorY][i] != playerToCheck) rowWin = false;
    for (int i = 0; i < 3; i++)
      if (board[i][cursorX] != playerToCheck) colWin = false;
    for (int i = 0; i < 3; i++)
      if (board[i][i] != playerToCheck) diagWin1 = false;
    for (int i = 0; i < 3; i++)
      if (board[i][2 - i] != playerToCheck) diagWin2 = false;

    if (rowWin || colWin || diagWin1 || diagWin2) {
      for (int i = 0; i < NUM_PIXELS; i++) {
        if (playerToCheck == 1) strip.setPixelColor(i, strip.Color(100, 0, 0));
        else strip.setPixelColor(i, strip.Color(0, 0, 100));
        strip.show();
        delay(30);
      }
      delay(200);
      for (int i = 0; i < NUM_PIXELS; i++) strip.setPixelColor(i, 0);
      strip.show();
      delay(300);
      for (int i = 0; i < NUM_PIXELS; i++) {
        if (playerToCheck == 1) strip.setPixelColor(i, strip.Color(100, 0, 0));
        else strip.setPixelColor(i, strip.Color(0, 0, 100));
      }
      strip.show();
      delay(300);
      for (int i = 0; i < NUM_PIXELS; i++) strip.setPixelColor(i, 0);
      strip.show();
      resetBoard();
    }
  }

  {
    bool boardFull = true;
    for (int row = 0; row < 3; row++)
      for (int col = 0; col < 3; col++)
        if (board[row][col] == 0) boardFull = false;
    if (boardFull) {
      for (int flash = 0; flash < 2; flash++) {
        for (int i = 0; i < NUM_PIXELS; i++)
          strip.setPixelColor(i, strip.Color(50, 50, 50));
        strip.show();
        delay(300);
        for (int i = 0; i < NUM_PIXELS; i++) strip.setPixelColor(i, 0);
        strip.show();
        delay(300);
      }
      resetBoard();
    }
  }

  drawGrid();
  strip.show();
}
void initBoss() {
  bossX = 2;
  bossY = 0;
  bossDir = 1;
  for (int r = 0; r < 2; r++)
    for (int c = 0; c < 4; c++)
      bossPixelHP[r][c] = 20;
}

void drawBoss() {
  for (int r = 0; r < 2; r++) {
    for (int c = 0; c < 4; c++) {
      if (bossPixelHP[r][c] > 0) {
        strip.setPixelColor(getPixelIndexSI(bossX + c, bossY + r), strip.Color(100, 40, 0));
      }
    }
  }
}

void spaceInvadersLoop(unsigned long now, int jx, int jy, bool btnState) {
  strip.clear();

  // player movement
 if (now - lastMoveTime > 150) {
  if (jy > 624) {
    playerX = min(playerX + 1, 7);
    lastMoveTime = now;
  } else if (jy < 400) {
    playerX = max(playerX - 1, 0);
    lastMoveTime = now;
  }
}

  // draw player
  strip.setPixelColor(getPixelIndexSI(playerX, playerY), strip.Color(0, 100, 0));

// fire shot when joystick held up
if (jx > 624 && shotX == -1 && now - lastShotTime > fireRate) {
  shotX = playerX;
  shotY = playerY;
  lastShotTime = now;
}

// move shot upward
if (shotX != -1 && now - lastShotMove > 50) {
  shotY--;
  lastShotMove = now;
  if (shotY < 0) {
    shotX = -1;  // shot left the board, clear it
    shotY = -1;
  }
}

// draw shot
if (shotX != -1) {
  strip.setPixelColor(getPixelIndexSI(shotX, shotY), strip.Color(100, 100, 100));
}
// move enemies
if (now - lastEnemyMove > baseEnemySpeed) {
  lastEnemyMove = now;
  // remove enemies that reach bottom
// game over check - enemy reached row 5
for (int i = 0; i < 6; i++) {
  if (enemies[i].alive && enemies[i].y >= 6) {
    // game over animation
    for (int flash = 0; flash < 2; flash++) {
      for (int j = 0; j < NUM_PIXELS; j++)
        strip.setPixelColor(j, strip.Color(100, 0, 0));
      strip.show();
      delay(300);
      for (int j = 0; j < NUM_PIXELS; j++)
        strip.setPixelColor(j, 0);
      strip.show();
      delay(300);
    }
    // reset to level 1
    currentLevel = 1;
    baseEnemySpeed = 800;
    shotX = -1;
    shotY = -1;
    playerX = 2;
    spawnEnemies();
    return;
  }
}

// safety net - remove enemies past row 7
for (int i = 0; i < 6; i++) {
  if (enemies[i].alive && enemies[i].y > 7) {
    enemies[i].alive = false;
  }
}
  // check if any enemy hits a wall
  bool hitWall = false;
  for (int i = 0; i < 6; i++) {
    if (!enemies[i].alive) continue;
    if (enemies[i].type == 1) {
      if ((enemyDir == 1 && enemies[i].x >= 7) || (enemyDir == -1 && enemies[i].x <= 0)) {
        hitWall = true;
        break;
      }
    } else {
      if ((enemyDir == 1 && enemies[i].x >= 6) || (enemyDir == -1 && enemies[i].x <= 0)) {
        hitWall = true;
        break;
      }
    }
  }

  if (hitWall) {
    enemyDir = -enemyDir;
    for (int i = 0; i < 6; i++) {
      if (enemies[i].alive) enemies[i].y++;
    }
  } else {
    for (int i = 0; i < 6; i++) {
      if (enemies[i].alive) enemies[i].x += enemyDir;
    }
  }
}


// shot collision detection
if (shotX != -1) {
  for (int i = 0; i < 6; i++) {
    if (!enemies[i].alive) continue;
    if (shotX == enemies[i].x && shotY == enemies[i].y) {
      enemies[i].hp-= shotDamage;
      shotX = -1;
      shotY = -1;
      if (enemies[i].hp <= 0) {
        enemies[i].alive = false;
        // 25% chance power up drop
        if (random(100) < 25) {
         powerUpOnBoard = true;
         powerUpX = enemies[i].x;
         powerUpY = enemies[i].y;
         powerUpType = random(1, 3);  // 1=cyan, 2=purple
         lastPowerUpFall = now;
        }
      }
      break;
    }
  }
}
// draw enemies
for (int i = 0; i < 6; i++) {
  if (!enemies[i].alive) continue;
  if (enemies[i].type == 1) {
    strip.setPixelColor(getPixelIndexSI(enemies[i].x, enemies[i].y), strip.Color(100, 0, 0));
  } else {
    strip.setPixelColor(getPixelIndexSI(enemies[i].x, enemies[i].y), strip.Color(100, 100, 0));
  }
}
if (isBossLevel) {
  // move boss at half enemy speed
  if (now - lastBossMove > baseEnemySpeed * 2) {
    lastBossMove = now;

    bool hitWall = (bossDir == 1 && bossX >= 4) || (bossDir == -1 && bossX <= 0);
    if (hitWall) {
      bossDir = -bossDir;
      bossY++;
    } else {
      bossX += bossDir;
    }

    // game over check
    if (bossY >= 5) {
      for (int flash = 0; flash < 2; flash++) {
        for (int j = 0; j < NUM_PIXELS; j++)
          strip.setPixelColor(j, strip.Color(100, 0, 0));
        strip.show();
        delay(300);
        for (int j = 0; j < NUM_PIXELS; j++)
          strip.setPixelColor(j, 0);
        strip.show();
        delay(300);
      }
      currentLevel = 1;
      baseEnemySpeed = 800;
      shotX = -1;
      shotY = -1;
      playerX = 2;
      initBoss();
      isBossLevel = false;
      spawnEnemies();
      return;
    }
  }

  // boss shot collision
  if (shotX != -1) {
    for (int r = 0; r < 2; r++) {
      for (int c = 0; c < 4; c++) {
        if (bossPixelHP[r][c] > 0 && shotX == bossX + c && shotY == bossY + r) {
          bossPixelHP[r][c] -= shotDamage;
          shotX = -1;
          shotY = -1;
          break;
        }
      }
    }
  }

  // boss defeated check
  bool bossDefeated = true;
for (int r = 0; r < 2; r++)
  for (int c = 0; c < 4; c++)
    if (bossPixelHP[r][c] > 0) bossDefeated = false;

if (bossDefeated) {

  for (int i = 0; i < NUM_PIXELS; i++)
    strip.setPixelColor(i, strip.Color(100, 40, 0));
  strip.show();
  delay(500);
  loopCount++;
  baseEnemySpeed = max(150, 800 - (loopCount * 100) - (currentLevel * 10));
  currentLevel = 1;
  isBossLevel = false;
  initBoss();
  spawnEnemies();
  return;
}

  drawBoss();
}

// check if all enemies dead = level clear
if (!isBossLevel) {
  bool allDead = true;
  for (int i = 0; i < 6; i++) {
    if (enemies[i].alive) {
      allDead = false;
      break;
    }
  }

  if (allDead) {
  currentLevel++;
  if (currentLevel >= 10) {
    isBossLevel = true;
    currentLevel = 10;
    // don't spawn regular enemies for boss level
  } else {
    baseEnemySpeed = max(200, 800 - (loopCount * 150) - 20);
    spawnEnemies();  // only spawn for regular levels
  }
  for (int i = 0; i < NUM_PIXELS; i++)
    strip.setPixelColor(i, strip.Color(0, 80, 0));
  strip.show();
  delay(500);
  shotX = -1;
  shotY = -1;
  enemyDir = 1;
 }
}





// power up falling
if (powerUpOnBoard && now - lastPowerUpFall > 500) {
  lastPowerUpFall = now;
  powerUpY++;
  if (powerUpY > 7) {
    powerUpOnBoard = false;  // fell off board
  }
}

// power up collection
if (powerUpOnBoard && powerUpX == playerX && powerUpY == playerY) {
  if (inventory < 3) {
    inventorySlots[inventory] = powerUpType;
    inventory++;
  }
  powerUpOnBoard = false;
}

// draw power up
if (powerUpOnBoard) {
  if (powerUpType == 1)
    strip.setPixelColor(getPixelIndexSI(powerUpX, powerUpY), strip.Color(0, 80, 80));  // cyan
  else
    strip.setPixelColor(getPixelIndexSI(powerUpX, powerUpY), strip.Color(80, 0, 80));  // purple
}

// draw inventory
drawInventory();


if (btnJustReleased && !holdTriggered) {
  if (inventory > 0) {
    int usedType = inventorySlots[0];
    for (int i = 0; i < 2; i++) inventorySlots[i] = inventorySlots[i + 1];
    inventorySlots[2] = 0;
    inventory--;
    
    if (usedType == 1) {
      powerUpActive = true;
      fireRate = 50;
      powerUpEndTime = now + 5000;
    } else if (usedType == 2) {
      purpleActive = true;
      if (damageStacks < 3) damageStacks++;
      shotDamage = 1 * (damageStacks + 1);
      purpleEndTime = now + 5000;
    }
  }
}

// expire power ups
if (powerUpActive && now > powerUpEndTime) {
  powerUpActive = false;
  fireRate = 300;
}
if (purpleActive && now > purpleEndTime) {
  purpleActive = false;
  damageStacks = 0;
  shotDamage = 1;
}


  strip.show();
}

void snakeLoop(unsigned long now, int jx, int jy, bool btnState) {
  // direction control - prevent reversing
  if (now - lastMoveTime > 150) {
  if (jy > 624 && snakeDirX != -1) { snakeDirX = 1; snakeDirY = 0; lastMoveTime = now; }
  else if (jy < 400 && snakeDirX != 1) { snakeDirX = -1; snakeDirY = 0; lastMoveTime = now; }
  else if (jx > 624 && snakeDirY != 1) { snakeDirX = 0; snakeDirY = -1; lastMoveTime = now; }
  else if (jx < 400 && snakeDirY != -1) { snakeDirX = 0; snakeDirY = 1; lastMoveTime = now; }
}

  // move snake
  if (now - lastSnakeMove > snakeSpeed) {
    lastSnakeMove = now;

    int newX = snakeX[0] + snakeDirX;
    int newY = snakeY[0] + snakeDirY;

    // wall collision
    if (newX < 0 || newX > 7 || newY < 0 || newY > 7) {
      // game over
      for (int flash = 0; flash < 2; flash++) {
        for (int i = 0; i < NUM_PIXELS; i++)
          strip.setPixelColor(i, strip.Color(100, 0, 0));
        strip.show();
        delay(300);
        for (int i = 0; i < NUM_PIXELS; i++) strip.setPixelColor(i, 0);
        strip.show();
        delay(300);
      }
      initSnake();
      return;
    }

    // self collision
    for (int i = 0; i < snakeLength; i++) {
      if (newX == snakeX[i] && newY == snakeY[i]) {
        for (int flash = 0; flash < 2; flash++) {
          for (int j = 0; j < NUM_PIXELS; j++)
            strip.setPixelColor(j, strip.Color(100, 0, 0));
          strip.show();
          delay(300);
          for (int j = 0; j < NUM_PIXELS; j++) strip.setPixelColor(j, 0);
          strip.show();
          delay(300);
        }
        initSnake();
        return;
      }
    }

    // check food
    bool ate = (newX == foodX && newY == foodY);

    // shift body
    if (!ate) {
      for (int i = snakeLength - 1; i > 0; i--) {
        snakeX[i] = snakeX[i-1];
        snakeY[i] = snakeY[i-1];
      }
    } else {
      for (int i = snakeLength; i > 0; i--) {
        snakeX[i] = snakeX[i-1];
        snakeY[i] = snakeY[i-1];
      }
      snakeLength++;
      snakeSpeed = max(100, snakeSpeed - 10);  // speed up slightly

      // win check
      if (snakeLength >= 64) {
        for (int i = snakeLength - 1; i >= 0; i--) {
          strip.setPixelColor(getPixelIndexSI(snakeX[i], snakeY[i]), strip.Color(0, 100, 0));
          strip.show();
          delay(30);
        }
        for (int flash = 0; flash < 1; flash++) {
          for (int j = 0; j < NUM_PIXELS; j++)
            strip.setPixelColor(j, strip.Color(0, 100, 0));
          strip.show();
          delay(500);
          for (int j = 0; j < NUM_PIXELS; j++) strip.setPixelColor(j, 0);
          strip.show();
          delay(300);
        }
        initSnake();
        return;
      }
      spawnFood();
    }

    snakeX[0] = newX;
    snakeY[0] = newY;
  }

  // draw
  strip.clear();
  strip.setPixelColor(getPixelIndexSI(foodX, foodY), strip.Color(100, 0, 0));
  for (int i = 1; i < snakeLength; i++)
    strip.setPixelColor(getPixelIndexSI(snakeX[i], snakeY[i]), strip.Color(0, 0, 80));
  strip.setPixelColor(getPixelIndexSI(snakeX[0], snakeY[0]), strip.Color(100, 100, 255));
  strip.show();
}


void connect4Loop(unsigned long now, int jx, int jy, bool btnState) {
  // cursor movement
  if (now - lastC4Move > 250) {
    if (jy > 624) {
      c4CursorCol = min(c4CursorCol + 1, 5);
      lastC4Move = now;
    } else if (jy < 400) {
      c4CursorCol = max(c4CursorCol - 1, 0);
      lastC4Move = now;
    }
  }


  // drop piece
  if (btnJustReleased && !holdTriggered) {
    // find lowest empty row in selected column
    int dropRow = -1;
    for (int r = 6; r >= 0; r--) {
      if (c4Board[r][c4CursorCol] == 0) {
        dropRow = r;
        break;
      }
    }
    if (dropRow != -1) {
      c4Board[dropRow][c4CursorCol] = c4Player;

      if (checkC4Win(c4Player)) {
        // win animation
        for (int flash = 0; flash < 2; flash++) {
          for (int i = 0; i < NUM_PIXELS; i++)
            strip.setPixelColor(i, c4Player == 1 ? strip.Color(100,0,0) : strip.Color(100,100,0));
          strip.show();
          delay(300);
          for (int i = 0; i < NUM_PIXELS; i++) strip.setPixelColor(i, 0);
          strip.show();
          delay(300);
        }
        initC4();
        return;
      }
      c4Player = 3 - c4Player;
    }
    btnJustReleased = false;
  }

  drawC4();
}

uint32_t getGameColor(int gameID, bool bright) {
  switch(gameID) {
    case 1: return bright ? strip.Color(80,0,0)   : strip.Color(24,0,0);    // red TTT
    case 2: return bright ? strip.Color(0,80,0)   : strip.Color(0,24,0);    // green SI
    case 3: return bright ? strip.Color(0,0,80)   : strip.Color(0,0,24);    // blue Snake
    case 4: return bright ? strip.Color(80,80,0)  : strip.Color(24,24,0);   // yellow C4
  }
  return 0;
}

//menu
void drawMenu() {
  strip.clear();

  int leftIndex  = (menuIndex - 1 + numGames) % numGames;
  int rightIndex = (menuIndex + 1) % numGames;

  // left slot cols 0-1
  for (int row = 2; row <= 5; row++)
    for (int col = 0; col <= 1; col++)
      strip.setPixelColor(getPixelIndexSI(col, row), getGameColor(games[leftIndex], false));

  // center slot cols 3-4
  for (int row = 2; row <= 5; row++)
    for (int col = 3; col <= 4; col++)
      strip.setPixelColor(getPixelIndexSI(col, row), getGameColor(games[menuIndex], true));

  // right slot cols 6-7
  for (int row = 2; row <= 5; row++)
    for (int col = 6; col <= 7; col++)
      strip.setPixelColor(getPixelIndexSI(col, row), getGameColor(games[rightIndex], false));

  strip.show();
}

void menuLoop(unsigned long now, int jx, int jy, bool btnState) {
  if (now - lastMoveTime > 300) {
    if (jy < 400) {
      menuIndex = (menuIndex - 1 + numGames) % numGames;
      lastMoveTime = now;
    } else if (jy > 624) {
      menuIndex = (menuIndex + 1) % numGames;
      lastMoveTime = now;
    }
    
  }

  if (btnJustReleased && !holdTriggered) {
    int selectedGame = games[menuIndex];
    if (selectedGame == 1) {
      gameState = 1;
      resetBoard();
      btnLastState = HIGH;
      btnPressX = cursorX;
      btnPressY = cursorY;
      btnJustReleased = false;
    } else if (selectedGame == 2) {
      gameState = 2;
      currentLevel = 1;
      isBossLevel = false;
      baseEnemySpeed = 800;
      shotX = -1;
      shotY = -1;
      playerX = 2;
      inventory = 0;
      inventorySlots[0] = 0;
      inventorySlots[1] = 0;
      inventorySlots[2] = 0;
      loopCount = 0;
      initBoss();
      spawnEnemies();
      btnJustReleased = false;
    } else if (selectedGame == 3) {
      gameState = 3;
         initSnake();
     btnJustReleased = false;
    } else if (selectedGame == 4) {
      gameState = 4;
     initC4();
     btnJustReleased = false;
}
  }

  drawMenu();
}

//execute game code
void loop() {
  unsigned long now = millis();
  bool btnState = digitalRead(BTN_PIN);
  int jx = analogRead(JOYX);
  int jy = analogRead(JOYY);

  btnJustReleased = (btnState == HIGH && btnLastState == LOW);

  if (btnState == LOW && btnLastState == HIGH) {
    btnPressTime = now;
    holdTriggered = false;
    btnPressX = cursorX;
    btnPressY = cursorY;
  }

  if (btnState == LOW && !holdTriggered && (now - btnPressTime >= 1000)) {
    holdTriggered = true;
    gameState = 0;
    strip.clear();
    strip.show();
  }

  if (btnJustReleased && !holdTriggered) {
    if (now - lastPowerPressTime < 300) {
      powerPressCount++;
    } else {
      powerPressCount = 1;
    }
    lastPowerPressTime = now;
    if (powerPressCount >= 4) {
      gameOn = !gameOn;
      powerPressCount = 0;
      if (!gameOn) {
        gameState = 0;
        strip.clear();
        strip.show();
      }
    }
  }

  btnLastState = btnState;  // always last

  if (gameOn) {
    if (gameState == 0) menuLoop(now, jx, jy, btnState);
    else if (gameState == 1) ticTacToeLoop(now, jx, jy, btnState);
    else if (gameState == 2) spaceInvadersLoop(now, jx, jy, btnState);
    else if (gameState == 3) snakeLoop(now, jx, jy, btnState);
    else if (gameState == 4) connect4Loop(now, jx, jy, btnState);
  }
}
