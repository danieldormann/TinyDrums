import java.util.ArrayList;

import ddf.minim.*;
import ddf.minim.analysis.*;
import ddf.minim.effects.*;
import ddf.minim.signals.*;
import ddf.minim.spi.*;
import ddf.minim.ugens.*;

import processing.serial.*;

Serial mSerial;
Minim mMinim;
boolean mFirstRead = true;

String[] mFiles = { "kick", "hh", "clap", "snare", "shaker", "cowbell", "chord" };
int[] mPatterns = { 0, 0, 0, 0, 0, 0, 0 };

ArrayList<AudioPlayer> mPlayers = new ArrayList<AudioPlayer>();

long mLastTickAt = 0;

void setup() {
  size(200, 200);
  mSerial = new Serial(this, Serial.list()[0], 9600); 
  mMinim = new Minim(this);

  for (int i = 0; i < mFiles.length; i++) {
    println(mFiles[i] + ".wav");
    mPlayers.add(mMinim.loadFile(mFiles[i] + ".mp3", 2048));
  }
}

void draw() {
  if (mSerial.available() != 0) {
    String c = mSerial.readStringUntil('\0');

    if (c != null) {
      c = c.substring(0, c.length() - 1);
      println(c);
      if (c.length() == 1) {
        tick(parseInt(c));
      } else if (!mFirstRead && c.length() > 1) {
        int i = parseInt(c.substring(0, 1));
        int p = parseInt(c.substring(1));

        println("new pattern");
        println(i + ": " + p);

        mPatterns[i] = p;
      }

      mFirstRead = false;
    }
  }
  
}

void tick(int n) {
  for (int i = 0; i < mFiles.length; i++) {
    int p = mPatterns[i];
    int b = 1 << n;
    if ((p & b) == b) mPlayers.get(i).play(0);
  }
}

void keyPressed() {
  if (key == 'a') mSerial.write('1');
  if (key == 'd') mSerial.write('2'); 
  if (key == 's') mSerial.write('3');
}