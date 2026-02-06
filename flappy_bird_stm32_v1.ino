// #include <U8g2lib.h> 
#include <EEPROM.h>
#include<string.h>

#include "tonedet.h"
#include "bird.h"
#include "title.h"

#define PIN_BL PB4 //backlight
#define PIN_BTN_L PB5 //button
#define PIN_BTN_R PB6 //button-reset
#define PIN_BTN_MD PB7 //button-gamemode-setting
#define PIN_BUZZER PB8 //buzzer

#define COLOR_SHIT 0b0111101100100000 // bird use this color
#define COLOR_LEMON T_GREEN//0b1010111111100101 // pipe use this color
#define COLOR_BACKGROUND T_BLUE//0
#define COLOR_FONT 0b1110010011001100//0xFFFF

#define DEBUG false

// ****************** New Screen *********************
/*
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define TFT_CS PB11
#define TFT_RST PB10
#define TFT_DC PA3
#define TFT_MOSI PA7
#define TFT_SCLK PA5

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
*/
// ****************** New Screen *********************

// ****************** New Library Test ******************
#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();
// ****************** New Library Test ******************

boolean isSplashScreen = true, gameOver, prevTap = false, prevMd = false, gamePause = false;

//Bird
int yPos, xPos;
int prevYPos, prevXPos;
int score, highScore;
int birdSpeed = 80;
int birds_generate_interval = 10000;
int difficulty = 1;
int prevDifficulty = difficulty;

//Obstacles
typedef struct { int x; int y;} Bird;
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 128
#define MAX_BIRDS 5
#define pipe_width 25
#define bird_width 10
#define bird_height 10
#define pipe_start_y 40
#define pipe_width 15
Bird birds[MAX_BIRDS];
Bird prevBirds[MAX_BIRDS];
int gap_size = 60;
int curBirds = 0;
int bird_delta = 1;
int lastScoreBird;
int FSM = 1;
// ****************
// 1 => Entry page
// 2 => Entry page to Home page 
// 3 => Home page
// 4 => Home page to Game page
// 5 => Game page
// 6 => GameOver
// 7 => GamePause
// ****************

char scoreBuf[4], highestScoreBuf[4];
char prevScoreBuf[4], prevHighestScoreBuf[4];
char freqBuffer[4], prevFreqBuffer[4];
long tick = 0, tickObstacle = 0, tickObstacleUpdate = 0;
long last_millis;
long tickToneDetect = 0;
long frameTick = 0;

void generateBirds() {
  if (millis() - tickObstacle > birds_generate_interval) {
    prevBirds[curBirds] = birds[curBirds];
    int pad = 12, startPos = random(pad, SCREEN_HEIGHT - bird_height - pad);
    birds[curBirds].x = SCREEN_WIDTH;
    birds[curBirds].y = startPos;
    if ((curBirds + 1) < MAX_BIRDS) {
      curBirds++;
    } else {
      curBirds = 0; 
    }
    tickObstacle = millis();
  }
}

void setGameOver() {
  gameOver = true;
  if (score > highScore) {
    EEPROMWriteInt(0, score);
    highScore = score;
  }
  tone(PIN_BUZZER, 225, 50);
  FSM = 2;
}

void updateBirds() {
   // update pos
   if (millis() - tickObstacleUpdate > birdSpeed / 2) {
     for (int i = 0; i < MAX_BIRDS; i++) {
        prevBirds[i] = birds[i];
        birds[i].x -= bird_delta;
        
        // Check If bird overlaps obstacle
        if(!DEBUG){
          boolean betweenBird = birds[i].x + bird_width > xPos && birds[i].x < xPos + pipe_width;
          if(betweenBird && !(birds[i].y > yPos && birds[i].y + bird_height < yPos + gap_size)){
            setGameOver();
          }else{
            if(betweenBird && i != lastScoreBird){
              score++;
              lastScoreBird = i;
            }
          }
        }
     } 
     tickObstacleUpdate = millis();
   }
}

void updatePipe(int freq) {
  int freqlow = 200;
  int freqhigh = 600;
  if(millis() - tick > 50){
    if(freq >= freqlow && freq <= freqhigh){
      prevYPos = yPos;
      if(!DEBUG) yPos = map(freq, freqlow, freqhigh, SCREEN_WIDTH - gap_size, 0);
    }
    tick = millis();
  }
}

void resetGame() {
   for (int i = 0; i < MAX_BIRDS; i++) {
    birds[i].x = -10; 
    birds[i].y = 0;
   }

   gameOver = false;
   gamePause = false;
   yPos = pipe_start_y;
   score = 0;
   lastScoreBird = -1;
}

boolean initt = true;

void FiniteStateMachine(boolean tap, boolean md){
  switch(FSM){
    case 1:{
      if(tap || md) FSM = 2;
      else FSM = 1;
      break;
    }
    case 2:{
      tft.fillScreen(COLOR_BACKGROUND);
      FSM = 3;
      initt = true;
      break;
    }
    case 3:{
      if(tap) FSM = 4;
      else if(md){
        difficulty++;
        if(difficulty > 3) difficulty = 1;
        switch(difficulty) {
          case 1:
            birdSpeed = 110;
            birds_generate_interval = 8500;
            gap_size = 65;
            break;
          case 2:
            birdSpeed = 75;
            birds_generate_interval = 5000;
            gap_size = 60;
            break;
          case 3:
            birdSpeed = 50;
            birds_generate_interval = 2500;
            gap_size = 50;
            break;
          default:
            break;
        }
      }
      break;
    }
    case 4:{
      tft.fillScreen(COLOR_BACKGROUND);
      FSM = 5;
      resetGame();
      initt = true;
      break;
    }
    case 5:{
      if(tap) FSM = 7;
      if(md) setGameOver();
      break;
    }
    case 6:{
      tft.fillScreen(0);
      FSM = 3;
      initt = true;
      break;
    }
    case 7:{
      if(tap){
        initt = true;
        FSM = 5;
      }
      break;
    }
    default:{
      break;
    }
  }
}

void draw(boolean tap, boolean md) {
  switch(FSM){
    case 1:{
      if(initt){
        tft.fillScreen(T_BLUE);

        // Draw Title text
        tft.setTextColor(T_YELLOW);
        tft.drawCentreString("FLAPPY BIRD", 80, 37, 2);
        // tft.setCursor(44, 37);
        // tft.println("FLAPPY BIRD");
        // tft.drawCentreString("BIRD", 80, 50, 2);
        tft.setTextColor(T_RED);
        // tft.setCursor(50, 67);
        // tft.println("PRESS BUTTON");
        // tft.setCursor(60, 75);
        // tft.println("TO START");
        tft.drawCentreString("PRESS BUTTON", 80, 58, 1);
        tft.drawCentreString("TO START", 80, 68, 1);

        // Draw Pipe
        tft.fillRect(0, 0, 14, 56, T_GREEN);
        tft.fillRect(0, 72, 14, 56, T_GREEN);
        tft.fillRect(24, 0, 14, 36, T_GREEN);
        tft.fillRect(24, 54, 14, 74, T_GREEN);
        tft.fillRect(121, 0, 14, 36, T_GREEN);
        tft.fillRect(121, 54, 14, 74, T_GREEN);
        tft.fillRect(146, 0, 14, 56, T_GREEN);
        tft.fillRect(146, 72, 14, 56, T_GREEN);

        initt = false;
      }
      break;
    }
    case 3:{
      // Drawing difficulty
      if(initt || prevDifficulty != difficulty){
        tft.setTextColor(COLOR_BACKGROUND);
        if(prevDifficulty == 1){
          // tft.setCursor(154, 10);
          tft.setCursor(100, 95);
          tft.print("*");  
        }else if(prevDifficulty == 2){
          // tft.setCursor(148, 10);
          tft.setCursor(100, 95);
          tft.print("**");  
        }else if(prevDifficulty == 3){
          // tft.setCursor(142, 10);
          tft.setCursor(100, 95);
          tft.print("***");  
        }
        tft.setTextColor(COLOR_FONT);
        if(difficulty == 1){
          // tft.setCursor(154, 10);
          tft.setCursor(100, 95);
          tft.print("*");  
        }else if(difficulty == 2){
          // tft.setCursor(148, 10);
          tft.setCursor(100, 95);
          tft.print("**");  
        }else if(difficulty == 3){
          // tft.setCursor(142, 10);
          tft.setCursor(100, 95);
          tft.print("***");  
        }
        prevDifficulty = difficulty;
      }
      // Drawing text about frequency
      if(initt || strcmp(freqBuffer, prevFreqBuffer)){
        tft.setCursor(90, 35);
        tft.setTextColor(COLOR_BACKGROUND);
        tft.printf("%s", prevFreqBuffer);
        tft.setCursor(90, 35);
        tft.setTextColor(COLOR_FONT);
        tft.printf("%s", freqBuffer);
      }

      if(get_sound_amp() < 20){
        tft.fillCircle(120, 38, 5, 0b1011010110010110);
      }else if(get_sound_amp() > 300){
        tft.fillCircle(120, 38, 5, 0b1111100000000000);
      }else{
        tft.fillCircle(120, 38, 5, 0b0000011111100000);
      }
      
      if(initt){
        tft.setTextColor(T_YELLOW);
        tft.drawCentreString("> TAP to play <", 80, 10, 2);
        tft.setTextColor(COLOR_FONT);
        tft.setCursor(30, 35);
        tft.print("Now Freq:");
        tft.setCursor(30, 55);
        tft.printf("Highest Score: %s", highestScoreBuf);
        tft.setCursor(30, 75);
        tft.printf("Last Score: %s", scoreBuf);
        tft.setCursor(30, 95);
        tft.printf("Difficulty:");

        // Draw Pipe
        tft.fillRect(0, 0, 14, 56, T_GREEN);
        tft.fillRect(0, 72, 14, 56, T_GREEN);
        tft.fillRect(146, 0, 14, 56, T_GREEN);
        tft.fillRect(146, 72, 14, 56, T_GREEN);

        initt = false;
      }
      break;
    }
    case 5:{
      if(initt){
        tft.setTextColor(COLOR_BACKGROUND);
        tft.setCursor(100, 0);
        tft.print("PAUSE");

        // Draw pipe
        tft.fillRect(xPos, 0, pipe_width, yPos, COLOR_LEMON);
        tft.fillRect(xPos, yPos + gap_size, pipe_width, SCREEN_HEIGHT - (yPos + gap_size), COLOR_LEMON);

        // Draw birds
        for(int t = 0; t < MAX_BIRDS; t++){
          for(int i = 0; i < 10; i++){
            for(int j = 0; j < 10; j++){
              tft.drawPixel(birds[t].x + i, birds[t].y + j, bird_pix[j][i]);
            }
          }
        }

        // Show Score and Frequency
        tft.fillRect(100, 116, 60, 12, COLOR_BACKGROUND);
        tft.setTextColor(COLOR_FONT);
        tft.setCursor(100, 116);
        tft.printf("%s", scoreBuf);
        tft.setCursor(140, 116);
        tft.printf("%s", freqBuffer);

        initt = false;
      }else{
        // Update pipe  
        int posDiff = yPos - prevYPos;
        if(posDiff < gap_size && posDiff > -gap_size){
          if(posDiff > 0){
            tft.fillRect(xPos, prevYPos, pipe_width, posDiff, COLOR_LEMON);
            tft.fillRect(xPos, prevYPos + gap_size, pipe_width, posDiff, COLOR_BACKGROUND);
            // tft.drawPixel(64, 3, 0b1111100000000000);
          }else if(posDiff < 0){
            tft.fillRect(xPos, yPos, pipe_width, -posDiff, COLOR_BACKGROUND);
            tft.fillRect(xPos, yPos + gap_size, pipe_width, -posDiff, COLOR_LEMON);
            // tft.drawPixel(64, 3, 0b0000011111100000);
          }
        }else{
          tft.fillRect(xPos, prevYPos, pipe_width, gap_size, COLOR_LEMON);
          tft.fillRect(xPos, yPos, pipe_width, gap_size, COLOR_BACKGROUND);
          // tft.drawPixel(64, 3, 0b0000000000011111);
        }

        // Update birds
        for(int t = 0; t < MAX_BIRDS; t++){
          if(prevBirds[t].y != birds[t].y){
            tft.fillRect(prevBirds[t].x, prevBirds[t].y, bird_width, bird_height, COLOR_BACKGROUND);
            for(int i = 0; i < 10; i++){
              for(int j = 0; j < 10; j++){
                tft.drawPixel(birds[t].x + i, birds[t].y + j, bird_pix[j][i]);
              }
            }
          }else if(prevBirds[t].x != birds[t].x){
            tft.fillRect(birds[t].x + 10, birds[t].y, bird_delta, 10, COLOR_BACKGROUND);
            for(int i = 0; i < 10; i++){
              tft.drawPixel(birds[t].x, birds[t].y + i, bird_pix[i][0]);
            }
            tft.fillRect(birds[t].x + 4, birds[t].y + 2, 1, 2, B_YELLOW);
            tft.drawPixel(birds[t].x + 3, birds[t].y + 2, B_BLACK);
            tft.drawPixel(birds[t].x + 2, birds[t].y + 2, B_WHITE);
            tft.drawPixel(birds[t].x + 2, birds[t].y + 3, B_BLACK);
            tft.drawPixel(birds[t].x + 4, birds[t].y + 6, B_YELLOW);
            tft.fillRect(birds[t].x + 5, birds[t].y + 5, 1, 3, B_WHITE);
            tft.fillRect(birds[t].x + 9, birds[t].y + 5, 1, 3, B_YELLOW);
          }
        }

        // Update Score and Frequency
        if(strcmp(scoreBuf, prevScoreBuf)){
          tft.setTextColor(COLOR_BACKGROUND);
          tft.setCursor(100, 116);
          tft.printf("%s", prevScoreBuf);
          tft.setTextColor(COLOR_FONT);
          tft.setCursor(100,116);
          tft.printf("%s", scoreBuf);
        } 
        if(strcmp(freqBuffer, prevFreqBuffer)){
          tft.setTextColor(COLOR_BACKGROUND);
          tft.setCursor(140, 116);
          tft.printf("%s", prevFreqBuffer);
          tft.setTextColor(COLOR_FONT);
          tft.setCursor(140, 116);
          tft.printf("%s", freqBuffer);
        }
      }
      break;
    }
    case 7:{
      tft.setTextColor(COLOR_FONT);
      tft.setCursor(100, 0);
      tft.print("PAUSE");
      break;
    }
    case 2:
    case 4:
    case 6:{
      break;
    }
    default:{
      tft.fillScreen(0x0000);
      break;
    }
  }
}

void setup()
{
  Serial.begin(250000);  

  // manual reset screen
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, LOW);
  delay(10);
  digitalWrite(TFT_RST, HIGH);

  //pinMode setting
  pinMode(PIN_BTN_L, INPUT_PULLUP);
  pinMode(PIN_BTN_R, INPUT_PULLUP);
  pinMode(PIN_BTN_MD, INPUT_PULLUP);
  
  highScore = EEPROMReadInt(0);
  score = 0;
  xPos = (48 / 2) - (bird_width / 2);
  resetGame();
  gameOver = true;
  tickToneDetect = millis();
  last_millis = millis();

  tft.init();
  for(int i = 0; i < 4; i++){
    tft.setRotation(i);
    tft.fillScreen(0);
  }
  tft.setRotation(1);
  tft.fillScreen(0);

// ********* benchmark *********
/*
  delay(1000);
  for(int cnt = 0; cnt < 2000; cnt++){
    for(int i = 0; i < 20; i++){
      tft.drawFastVLine(i, 0, 128, 0xFFF0);
    }
    for(int i = 0; i < 20; i++){
      tft.drawFastVLine(i, 0, 128, 0);
    }
  }

  tft.fillScreen(0x000000);
  delay(1000);

  for(int cnt = 0; cnt < 2000; cnt++){
    tft.fillRect(0, 0, 20, 128, 0xFFF0);
    tft.fillRect(0, 0, 20, 128, 0);
  }
*/
// ********* benchmark *********

  tft.setTextColor(0xFFFF);
  tft.drawCentreString("822 Colde Garage", 80, 40, 2);
  tft.drawCentreString("# NTHU_EE", 80, 60, 1);
  delay(1500);
  tft.fillScreen(0x0000);
}

boolean wasReseted = false;
float prevfreq = 1.0;
int height = 0;

void loop()
{ 
  Serial.println("CNM");
  float freq = prevfreq;
  if(millis() - tickToneDetect > 100) {
    freq = Tone_det();
    Serial.print("Now Freq: ");
    Serial.println(int(freq));
    tickToneDetect = millis();
  }

  strcpy(prevFreqBuffer, freqBuffer);
  if(freq <= 0) strcpy(freqBuffer, "X");
  else itoa(int(freq), freqBuffer, 10);

// ********* Draw bar graphic *********
/*
  if(millis() - frameTick > 10){
    if(freq > 0){
      int newHeight;
      if(freq > 600) newHeight = 128;
      else newHeight = map(freq, 0, 600, 0, 128);
      int heightDiff = newHeight - height;
      if(heightDiff > 0) tft.fillRect(50, height, 20, heightDiff, 0b1111100000000000);
      else tft.fillRect(50, newHeight, 20, -heightDiff, 0);
      height = newHeight;
    } else {
      tft.fillRect(50, height, 20, -height, 0);
    }

    tft.setCursor(100, 40);
    tft.setTextColor(0);
    tft.println(prevFreqBuffer);
    tft.setCursor(100, 40);
    tft.setTextColor(0xFFF0);
    tft.println(freqBuffer);

    frameTick = millis();
  }
*/
// ********* Draw bar graphic *********

  boolean tap = digitalRead(PIN_BTN_L);
  tap = !tap;
  boolean TappedOnly = tap && !prevTap;
  boolean md = digitalRead(PIN_BTN_MD);
  md = !md;
  boolean mdOnly = md && !prevMd;
  
  boolean rst = digitalRead(PIN_BTN_R);
  rst = !rst;
  if (rst) {
    EEPROMWriteInt(0, 0);
    highScore = 0;
    strcpy(prevHighestScoreBuf, highestScoreBuf);
    itoa(highScore, highestScoreBuf, 10);
    FSM = 2;
    delay(1000);
  }
  
  FiniteStateMachine(TappedOnly, mdOnly);
  draw(TappedOnly, mdOnly);
  
  if (FSM == 5) {
    updatePipe(int(freq));
    generateBirds();
    updateBirds();
  } else if(FSM == 7){
    Serial.println("Game is Paused");
    tick = millis() - (last_millis - tick);
    tickObstacle = millis() - (last_millis - tickObstacle);
    tickObstacleUpdate = millis() - (last_millis - tickObstacleUpdate);
  }
  
  strcpy(prevScoreBuf, scoreBuf);
  itoa(score, scoreBuf, 10);
  strcpy(prevHighestScoreBuf, highestScoreBuf);
  itoa(highScore, highestScoreBuf, 10);
  analogWrite(PIN_BL, 255);
  
  prevTap = tap;
  prevMd = md;
  prevfreq = freq;
  last_millis = millis();
}

//http://forum.arduino.cc/index.php/topic,37470.0.html
//This function will write a 2 byte integer to the eeprom at the specified address and address + 1
void EEPROMWriteInt(int p_address, int p_value)
      {
      byte lowByte = ((p_value >> 0) & 0xFF);
      byte highByte = ((p_value >> 8) & 0xFF);

      EEPROM.write(p_address, lowByte);
      EEPROM.write(p_address + 1, highByte);
      }

//This function will read a 2 byte integer from the eeprom at the specified address and address + 1
unsigned int EEPROMReadInt(int p_address)
      {
      byte lowByte = EEPROM.read(p_address);
      byte highByte = EEPROM.read(p_address + 1);

      return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
      }
