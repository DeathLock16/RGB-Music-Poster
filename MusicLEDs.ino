#include <Adafruit_NeoPixel.h>

#define LPIN 5
#define RPIN 6
#define NUMOFLEDS 50
#define BRIGHTNESS 51

#define RED 16711684
#define BLUE 255

Adafruit_NeoPixel leftStrip = Adafruit_NeoPixel(NUMOFLEDS, LPIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel rightStrip = Adafruit_NeoPixel(NUMOFLEDS, RPIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  leftStrip.begin(); leftStrip.setBrightness(BRIGHTNESS);
  rightStrip.begin(); rightStrip.setBrightness(BRIGHTNESS);
}

int counter = 0;
int leftCounter = 0; int rightCounter = 0;

char identifier;
int value;

void loop() {

    while(Serial.available() < 4) {
      if(counter == 5000) {
      leftStrip.clear(); rightStrip.clear();
      leftStrip.show(); rightStrip.show();
      } else counter++;
    } counter = 0;

    identifier = Serial.read();

    if(identifier == 'L') {
      value = Serial.read();
      if(!(value >= 1 && value <= NUMOFLEDS)) {
        value = Serial.read();
        if(!(value >= 1 && value <= NUMOFLEDS)) {
          if(leftCounter == 10) {
            leftStrip.clear();
            leftStrip.show();
          } else leftCounter++;
        } else {
        leftCounter = 0;
        leftStrip.fill(BLUE, 0, value);
        leftStrip.fill(0, value, 0);
        leftStrip.show();
      }
        
      } else {
        leftCounter = 0;
        leftStrip.fill(BLUE, 0, value);
        leftStrip.fill(0, value, 0);
        leftStrip.show();
      }
    }


      else if(identifier == 'R') {
        value = Serial.read();
        if(!(value >= 1 && value <= NUMOFLEDS)) {
          value = Serial.read();
          if(!(value >= 1 && value <= NUMOFLEDS)) {
            if(rightCounter == 10) {
              rightStrip.clear();
              rightStrip.show();
            } else rightCounter++;
          } else {
        rightCounter = 0;
        rightStrip.fill(RED, 0, value);
        rightStrip.fill(0, value, 0);
        rightStrip.show();
      }
        
      } else {
        rightCounter = 0;
        rightStrip.fill(RED, 0, value);
        rightStrip.fill(0, value, 0);
        rightStrip.show();
      }
    }

    else return;
    
}
