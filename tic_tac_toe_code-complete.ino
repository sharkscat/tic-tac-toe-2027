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
int btnPressX = 0;
int btnPressY = 0;

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

void setup() {
  pinMode(BTN_PIN, INPUT_PULLUP);
  strip.begin();
  strip.clear();
  strip.setBrightness(25);
  delay(500);
  strip.show();
  Serial.begin(9600);
  btnLastState = digitalRead(BTN_PIN);
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



void loop() {

  //button stuff, lets button be used to place a piece and turn off/on the board.
 unsigned long now = millis();
  bool btnState = digitalRead(BTN_PIN);

if (btnState == LOW && btnLastState == HIGH) {
  btnPressTime = now;
  holdTriggered = false;
  btnPressX = cursorX;
  btnPressY = cursorY;
}

if (btnState == LOW && !holdTriggered && (now - btnPressTime >= 1350)) {
  holdTriggered = true;
  gameOn = !gameOn;
  if (!gameOn) {
    strip.clear();
    strip.show();
  }
}

if (btnState == HIGH && btnLastState == LOW) {
  if (!holdTriggered && cursorX == btnPressX && cursorY == btnPressY) {
    if (gameOn && board[cursorY][cursorX] == 0) {
      board[cursorY][cursorX] = currentPlayer;
      currentPlayer = 3 - currentPlayer;
    }
  }
}

btnLastState = btnState;


//if game is on then run game, otherwise don't run
  if (gameOn){
  int jx = analogRead(JOYX);
  int jy = analogRead(JOYY);
 



  if (now - lastMoveTime > 250) {
  if (jx < 400) {
  cursorX = min(cursorX + 1, 2);
  lastMoveTime = now;
  recordInput(1); // right
} else if (jx > 624) {
  cursorX = max(cursorX - 1, 0);
  lastMoveTime = now;
  recordInput(0); // left
} else if (jy < 400) {
  cursorY = min(cursorY + 1, 2);
  lastMoveTime = now;
  recordInput(3); // down
} else if (jy > 624) {
  cursorY = max(cursorY - 1, 0);
  lastMoveTime = now;
  recordInput(2); // up
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
      for (int flash = 0; flash < 3; flash++) {
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
  else {
  strip.clear();
  strip.show();
}
}

