#include "buttonreadertimed.hpp"

const byte buttons[] = {1,2,3,4,5,6,7,8,14,15,16};
long t; //timer

void setup(){
  for (auto&& i : buttons){
    pinMode(i, INPUT_PULLUP);
  }
  Serial.begin(9600);
}

void loop(){
  buttonreader(buttons,10);
}
