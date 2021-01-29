#pragma once
#ifndef MAHJONG_H
#define MAHJONG_H

#include <Keypad.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <Adafruit_LEDBackpack.h>

enum Wind : byte {North, East, South, West}; //enum for winds, 1 = east
enum state : byte {running, configMenu, gameMenu, debugMenu, playerMenu}; //enum for which menu one is in

class Player {
public:
  bool riichi : 1;
  Wind wind : 2;
  int score;
  Adafruit_AlphaNum4 scoreDisplay;
  Player();

  bool callRiichi();
};

class Game {
public:
  Game();

  byte hand : 4; //current hand number
  bool kiriage : 1; //Kiriage Mangan

  byte honba = 0; //current honba count, NOT 
  int startingPoints = 250, //each player's starting score in 100s
      goalPoints = 300; //target score in 100s
  
  Player players[4]; //all players

  Wind getRoundWind(); //returns current round Wind
  byte getKyoku();//returns kyoku a.k.a. dealer
  int calculateBasePoints(const byte &han, const byte &fu);


// These functions return the object, to allow chaining of functions (probably won't need it, but I might as well show I know how to do this)
  Game& ron(const byte &winner, const int &basePoints, const byte &opponent);
  Game& tsumo(const byte &winner, const int &basePoints);
};
#endif