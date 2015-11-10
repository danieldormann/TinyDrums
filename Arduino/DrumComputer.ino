#include <digitalWriteFast.h>
#include <Wire.h>
#include "cmd.h"

#define RGB(r,g,b) (((r&0xF8)<<8)|((g&0xFC)<<3)|((b&0xF8)>>3)) //16bit: 5 red | 6 green | 5 blue
#define I2C_ADDR 0x20

const uint8_t NUM_INSTRUMENTS = 7;
const uint8_t NUM_STEPS = 8;

const char* Names[NUM_INSTRUMENTS] = { "Kick", "HiHat", "Clap", "Snare", "Shaker", "Cowbell", "Chord" };

float mBpm = 90;
uint8_t mCurTick = 0;

bool mRedraw = true;
bool mInstActive = false;
bool mLightUp = false;
bool mEncWasPressed = false;


uint8_t mInst = 0;
uint8_t mPatternPos = 0;
uint8_t mPatterns[NUM_INSTRUMENTS] = { 0, 0, 0, 0, 0, 0, 0 };

unsigned long mLastTickAt = 0;
unsigned long mLastPressAt = 0;

void setup() {

  Serial.begin(9600);

  Wire.begin();

  setBacklight(100);

  // enable rotary controller
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(CMD_CTRL);
  Wire.write(CMD_CTRL_FEATURES);
  Wire.write(FEATURE_ENC);
  Wire.endTransmission();

  Wire.flush();

  delay(1000);
}


void loop() {
  checkInput(); 

  if ((millis() - mLastTickAt) > 60000.0f / mBpm / 4.0f ) {
    mCurTick++;
    if (mCurTick >= NUM_STEPS) mCurTick = 0;

    Serial.print(mCurTick);
    Serial.print('\0');
    mLastTickAt = millis();

    mRedraw = true;
  }

  if (mRedraw) {
    drawElements();
    mRedraw = false;
  }

  delay(50);
}

void checkInput() {
  int8_t rotDir = 0;
  int8_t rotState = 0;

  getRotaryState(rotDir, rotState);
  
  // DEBUG ## overwrite with serial cmds
  if (Serial.available()) {
    char w = Serial.read();
    if ( w == '1' ) rotDir = -1;
    if ( w == '2' ) rotDir =  1;
    if ( w == '3' ) rotState = 1;
  }

  if (rotDir != 0) {
    if (mInstActive) {
      mPatternPos += rotDir;
      if (mPatternPos < 0) mPatternPos = NUM_STEPS;      
      if (mPatternPos > NUM_STEPS) mPatternPos = 0;      
    } else {
      mInst += rotDir;
      if (mInst < 0) mInst = NUM_INSTRUMENTS - 1;
      if (mInst >= NUM_INSTRUMENTS) mInst = 0;
    }  
  }
  
  if (rotState > 0 && millis() - mLastPressAt > 200) {
    if (mInstActive) {
      if (mPatternPos < NUM_STEPS) {
        mPatterns[mInst] ^= indexToByte(mPatternPos); 
      } else {
        propagatePattern();
        mInstActive = false;
      }
    } else {
      mInstActive = true;
    }

    mLastPressAt = millis();
  }

  if (rotDir != 0 || rotState != 0) {
    mRedraw = true;      
  }
}

void propagatePattern() {
  Serial.print(String(mInst) + String(mPatterns[mInst]) + '\0');
}

void setBacklight(const int d) {
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(CMD_LCD_LED);
  Wire.write(d);
  Wire.endTransmission();
 
}

void getRotaryState(int8_t &p, int8_t &s) {
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(CMD_ENC_POS);
  Wire.endTransmission();

  if(Wire.requestFrom(I2C_ADDR, 2) > 0) { 
    p = Wire.read();
    s = Wire.read();
    
    if (p < 0) {
      p = -1;
    } else if (p > 0) {
      p = 1;
    }    
  } else {
    p = 0;
    s = 0;
  }
}

void drawElements() {
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(CMD_LCD_CLEARBG);
  Wire.endTransmission();

  drawText("TinyDrums", 10, 10);
  drawText(Names[mInst], 10, 45);
  drawPattern(10, 80);
  drawRect(10 + (mCurTick * 8) + 4, 70, 2, 7);    

  if (mInstActive) {
    drawRect(10 + (mPatternPos * 8), 90, 8, 3);    
  }
}

void drawText(const char* s, const uint16_t x, const uint16_t y ) {
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(CMD_LCD_DRAWTEXTFG);
  Wire.write(x);
  Wire.write(y);
  Wire.write(0); // dont clear
  Wire.write(strlen(s)); 

  for (uint8_t n = 0; n < strlen(s); n++) {
    Wire.write(s[n]);
  }
  
  Wire.endTransmission();
}

void drawPattern(const uint16_t x, const uint16_t y) {
  uint8_t p = mPatterns[mInst];
  
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(CMD_LCD_DRAWTEXTFG);
  Wire.write(x);
  Wire.write(y);
  Wire.write(0); 
  Wire.write(NUM_STEPS + 1); 

  for (uint8_t n = 0; n < NUM_STEPS; n++) {
    uint8_t m = indexToByte(n);
    Wire.write((p & m) == m ? "X" : "O" );
  }

  Wire.write(">");
  
  Wire.endTransmission();
}


void drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(CMD_LCD_FILLRECTFG);
  Wire.write(x);
  Wire.write(y);
  Wire.write(w);
  Wire.write(h);
  Wire.endTransmission();  
}

int indexToByte(int n) {
  return 1 << n;
}


