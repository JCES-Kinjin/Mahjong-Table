#pragma once
#ifndef BUTTONREADER_H
#define BUTTONREADER_H

//checks each button's state in sequence until it finds one that is pressed, then returns it
int buttonreader(const byte * buttons,const size_t &size,const int &inputTime){
  bool done=false;
  size_t i;
  while(!done){
    i = 0;
    for(; i < size; ++i){
      if (!digitalRead(buttons[i])){
        done = true;
        while(!digitalRead(buttons[i])){}
        delay(inputTime);
      }
    }
    Serial.println(millis(),DEC);
  }
  return i;
}
//template to allow for calling without explicilty declaring amount of buttons
template <size_t size>
int buttonreader(const byte (&buttons)[size],const int &inputTime){
  return buttonreader(buttons, size, inputTime);
}
#endif