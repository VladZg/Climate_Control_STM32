#include <Adafruit_GFX.h>   // include adafruit GFX library
#include "KS0108_GLCD.h"    // include KS0108 GLCD library
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <avr/sleep.h>

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

// KS0108 GLCD library initialization according to the following connection:
// KS0108_GLCD(DI, RW, E, DB0, DB1, DB2, DB3, DB4, DB5, DB6, DB7, CS1, CS2, RES);
// KS0108_GLCD display = KS0108_GLCD(A0, A1, A2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define BALL_RADIUS 3
#define PADDLE_WIDTH 2
#define PADDLE_HEIGHT 20
#define PADDLE_SPEED 2
#define BALL_SPEED 2
#define WALL_THICKNESS 2
#define DELTA_Y 4

#define N_MAX_ROUNDS 15

const int finishGameButtonPin  = 3;     
const int finishRoundButtonPin = 2;     

const int userLButtonUpPin   = 8; 
const int userLButtonDownPin = 7; 
const int userRButtonUpPin   = 5;      
const int userRButtonDownPin = 6;      

volatile int finishGameButtonState   = 0; 
volatile int finishRoundButtonState  = 0; 

volatile int userLButtonUpState   = 0; // Переменная для хранения состояния кнопки
volatile int userLButtonDownState = 0; // Переменная для хранения состояния кнопки
volatile int userRButtonUpState   = 0; // Переменная для хранения состояния кнопки
volatile int userRButtonDownState = 0; // Переменная для хранения состояния кнопки

#define BUZZER_PIN 13

unsigned long startTime = 0;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

struct Ball {
    int x;
    int y;
    int speedX;
    int speedY;
    int r;
};

struct Platform {
    int x;
    int y;
    int speedY;
    int height;
    int width;
};

struct Game {
  int score1;
  int score2;
  Ball ball;
  Platform platfLeft;
  Platform platfRight;
  int isPlaying;
  int counter;
  float freq;
  unsigned long gameStartTime;
  unsigned long roundStartTime;
} game;

void setup()
{                
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(finishGameButtonPin , INPUT);
  pinMode(finishRoundButtonPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(finishGameButtonPin ), finishGameButtonISR , CHANGE);
  attachInterrupt(digitalPinToInterrupt(finishRoundButtonPin), finishRoundButtonISR, CHANGE);

  pinMode(userLButtonUpPin  , INPUT); 
  pinMode(userLButtonDownPin, INPUT); 
  pinMode(userRButtonUpPin  , INPUT); 
  pinMode(userRButtonDownPin, INPUT); 

  srand(time(NULL));

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.display();

  StartMessage();
  game.score1 = 0;
  game.score2 = 0;
  game.freq = 0.5;
  game.gameStartTime = millis();
}

// main loop
void loop()
{
  for (int nRounds = 0; nRounds < N_MAX_ROUNDS; nRounds++)
  {
    startRound();  

    while (game.isPlaying)
    {
        display.clearDisplay();
        drawWall();
        drawBall();
        drawPaddles();
        drawCounts();
        updateBall();
        updatePaddle();
        // updateTimer();
        display.display();    
        delay(game.freq);  
    }

    ScoreMessage(game.score1, game.score2);
  }

  FinalMessage();
  sleepMode();
}

void updateBall()
{
  //Ball movement
  if (game.ball.x + game.ball.speedX < game.platfLeft.x + game.ball.r) {
      game.ball.x = game.platfLeft.x + game.platfLeft.width + game.ball.r;
      game.ball.y = game.ball.y + game.ball.speedY;
      if ((game.ball.y - game.ball.r < game.platfLeft.y + game.platfLeft.height) && (game.ball.y + game.ball.r > game.platfLeft.y)) {
          game.counter++;
          tone(BUZZER_PIN, 3250, 20);
          // updateTimer();
          game.ball.speedX = game.ball.speedX * (-1);
          complicateGame();
      }
      else {
          game.ball.x += game.ball.speedX;
          game.score2++;
          game.isPlaying = 0;
          tone(BUZZER_PIN, 1000, 100);
      }
  }
  else if (game.ball.x + game.ball.speedX + game.ball.r > game.platfRight.x) {
      game.ball.x = game.platfRight.x - game.ball.r;
      game.ball.y = game.ball.y + game.ball.speedY;
      if ((game.ball.y - game.ball.r < game.platfRight.y + game.platfRight.height) && (game.ball.y + game.ball.r > game.platfRight.y)) {
          game.counter++;
          tone(BUZZER_PIN, 3500, 20);
          // updateTimer();
          game.ball.speedX = game.ball.speedX * (-1);
          complicateGame();
      }
      else {
          game.ball.x += game.ball.speedX;
          game.score1++;
          game.isPlaying = 0;
          tone(BUZZER_PIN, 1000, 100);
      }
  }
  else if (game.ball.y + game.ball.speedY <= WALL_THICKNESS + game.ball.r) {
      game.ball.speedY = game.ball.speedY * (-1);
      game.ball.y = WALL_THICKNESS + game.ball.r;
      game.ball.x += game.ball.speedX;
  }
  else if (game.ball.y + game.ball.speedY >= SCREEN_HEIGHT - WALL_THICKNESS - game.ball.r) {
      game.ball.speedY = game.ball.speedY * (-1);
      game.ball.y = SCREEN_HEIGHT - WALL_THICKNESS - game.ball.r;
      game.ball.x += game.ball.speedX;
  }
  else {
      game.ball.x += game.ball.speedX;
      game.ball.y += game.ball.speedY;
  }
}

void updatePaddle()
{
  if (digitalRead(userLButtonUpPin) == HIGH)
  {
    game.platfLeft.speedY = -PADDLE_SPEED;
  }
  else if (digitalRead(userLButtonDownPin) == HIGH)
  {
    game.platfLeft.speedY = PADDLE_SPEED;
  }
  else
    game.platfLeft.speedY = 0;

  if (digitalRead(userRButtonUpPin) == HIGH)
  {
    game.platfRight.speedY = -PADDLE_SPEED;
  }
  else if (digitalRead(userRButtonDownPin) == HIGH)
  {
    game.platfRight.speedY = PADDLE_SPEED;
  }
  else
    game.platfRight.speedY = 0;

  //Platfrom movement
  if (game.platfLeft.y + game.platfLeft.speedY < WALL_THICKNESS) {
      game.platfLeft.speedY = game.platfLeft.speedY * (-1);
      game.platfLeft.y = WALL_THICKNESS;
  }
  else if (game.platfLeft.y + game.platfLeft.height + game.platfLeft.speedY > SCREEN_HEIGHT-WALL_THICKNESS-1) {
      game.platfLeft.speedY = game.platfLeft.speedY * (-1);
      game.platfLeft.y = SCREEN_HEIGHT-WALL_THICKNESS-1 - game.platfLeft.height;
  }
  else 
      game.platfLeft.y = game.platfLeft.y + game.platfLeft.speedY;

  if (game.platfRight.y + game.platfRight.speedY < WALL_THICKNESS) {
      game.platfRight.speedY = game.platfRight.speedY * (-1);
      game.platfRight.y = WALL_THICKNESS;
  }
  else if (game.platfRight.y + game.platfRight.height + game.platfRight.speedY > SCREEN_HEIGHT-WALL_THICKNESS-1) {
      game.platfRight.speedY = game.platfRight.speedY * (-1);
      game.platfRight.y = SCREEN_HEIGHT-WALL_THICKNESS-1 - game.platfRight.height;
  }
  else 
      game.platfRight.y = game.platfRight.y + game.platfRight.speedY;
}

unsigned long updateTimer()
{
  unsigned long elapsedTime = (millis() - game.roundStartTime) / 1000;
  // display.setTextSize(1);
  // display.setCursor(1, 2);
  // char buffer[50];
  // sprintf(buffer, "%d sec", elapsedTime);
  // display.println(buffer);
  // display.display();
  return elapsedTime;
}

void finishRoundButtonISR() {
  finishRoundButtonState = digitalRead(finishRoundButtonPin); // Считываем состояние кнопки
  
  if (finishRoundButtonState == HIGH)
  {
    game.isPlaying = 0;
    tone(BUZZER_PIN, 5000, 150);
  }
}

void finishGameButtonISR() {
  finishGameButtonState = digitalRead(finishGameButtonPin); // Считываем состояние кнопки

  if (finishGameButtonState == HIGH)
  {
    FinalMessage();
    sleepMode();
  }
}

void drawWall()
{
  display.fillRect(0, 0, SCREEN_WIDTH, WALL_THICKNESS, WHITE);
  // display.fillRect(0, 0, WALL_THICKNESS, SCREEN_HEIGHT, WHITE);
  // display.fillRect(SCREEN_WIDTH - WALL_THICKNESS, 0, WALL_THICKNESS, SCREEN_HEIGHT, WHITE);
  display.fillRect(0, SCREEN_HEIGHT - WALL_THICKNESS, SCREEN_WIDTH, WALL_THICKNESS, WHITE);
}

void drawBall()
{
  display.fillCircle(game.ball.x, game.ball.y, game.ball.r, WHITE);
}

void drawPaddles()
{
  display.fillRect( game.platfLeft.x,  game.platfLeft.y,  game.platfLeft.width,  game.platfLeft.height, WHITE);
  display.fillRect(game.platfRight.x, game.platfRight.y, game.platfRight.width, game.platfRight.height, WHITE);
}

void startRound()
{
  game.roundStartTime = millis(); // Запоминаем время запуска
  game.ball = {SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 1, 1, BALL_RADIUS};

  do {
      game.ball.speedX = (rand() % 7 - 3);
      game.ball.speedY = (rand() % 7 - 3);
  } while (game.ball.speedX == 0);

  game.platfLeft  = {WALL_THICKNESS+1             , (SCREEN_HEIGHT-PADDLE_HEIGHT)/2,  PADDLE_SPEED, PADDLE_HEIGHT, PADDLE_WIDTH};
  game.platfRight = {SCREEN_WIDTH-WALL_THICKNESS-1, (SCREEN_HEIGHT-PADDLE_HEIGHT)/2, -PADDLE_SPEED, PADDLE_HEIGHT, PADDLE_WIDTH};
  game.isPlaying  = 1;
  game.counter    = 0;
  game.freq       = 0.5;
}

#define MIN_PADDLE_HEIGHT 6

void complicateGame()
{
  game.freq -= 0.1*game.freq;

  if (game.counter % 2 == 0)
  {
    float alpha_rand = (rand() % 50 + 200) / 100.0 - 1.0;

    game.ball.speedX *= alpha_rand;
    game.ball.speedY *= alpha_rand;
  }

  if (game.counter % 5 == 0)
  {
    float beta_rand = (rand() % 50 + 200) / 100.0 - 1.0;

    if (game.platfLeft.height/beta_rand > MIN_PADDLE_HEIGHT)
    {
      game.platfLeft.height /= beta_rand;
      game.platfRight.height /= beta_rand;
    }
    else
    {
      game.platfLeft.height = MIN_PADDLE_HEIGHT;
      game.platfRight.height = MIN_PADDLE_HEIGHT;
    }
  }
}

void drawCounts()
{
  display.setTextSize(1);
  display.setCursor(44, 6);
  char buffer[50];
  sprintf(buffer, "score: %d", game.counter);
  display.println(buffer);
  display.display();
}

#define C 262
#define D 277
#define E 293
#define F 311
#define G 330
#define A 350
#define B 370

void playMelody() {
  // Последовательность частот и их длительностей для мелодии
  int melody[]        = {C, D, E, F, G, A, B, A, G, F, E, D, C}; // Частоты нот от до
  int noteDurations[] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}; // Длительности нот (в четвертях)

  for (int i = 0; i < 13; i++) {
    int noteDuration = 1000 / noteDurations[i];
    tone(BUZZER_PIN, melody[i], noteDuration);
  }
}

#undef C
#undef D
#undef E
#undef F
#undef G
#undef A
#undef B

void StartMessage()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(4,25);
  display.println("Game \n starts!!!");
  display.display();
  playMelody();
  delay(1000);

  display.clearDisplay();
  display.setTextSize(5);
  display.setCursor(48,25);
  display.println("3!");
  display.display();
  tone(BUZZER_PIN, 2500, 200);
  delay(800);

  display.clearDisplay();
  display.setTextSize(5);
  display.setCursor(48,25);
  display.println("2!");
  display.display();
  tone(BUZZER_PIN, 2500, 200);
  delay(800);

  display.clearDisplay();
  display.setTextSize(5);
  display.setCursor(48,25);
  display.println("1!");
  display.display();
  tone(BUZZER_PIN, 2500, 400);
  delay(600);
}

void ScoreMessage(int score1, int score2)
{
  display.clearDisplay();
  
  display.setTextSize(3);
  display.setCursor(32,20);
  char buffer[50];
  sprintf(buffer, "%d:%d", score1, score2);
  display.println(buffer);

  unsigned long  sec = updateTimer();
  display.setTextSize(1);
  display.setCursor(33,50);
  sprintf(buffer, "(%d sec)", sec);
  display.println(buffer);

  display.display();

  // melody
  tone(BUZZER_PIN, 1000, 100);
  tone(BUZZER_PIN, 1200, 100);
  tone(BUZZER_PIN, 1400, 100);
  tone(BUZZER_PIN, 1600, 100);
  tone(BUZZER_PIN, 1800, 100);
  tone(BUZZER_PIN, 2000, 100);
  
  delay(2000);
}

void FinalMessage()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 20);

  if (game.score1 > game.score2)
    display.println("Left player won!");
  else if (game.score2 > game.score1)
    display.println("Right player won!");
  else
    display.println("No one won!");

  display.display();
  playMelody();
  delay(1500);

  display.clearDisplay();
  display.setTextSize(4);
  display.setCursor(8,25);
  display.println("Bye!");
  display.display();
  playMelody();
  delay(1500);
}

void sleepMode()
{
  display.clearDisplay();
  display.display();
  // Устанавливаем микроконтроллер в режим ожидания
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();
}