//#include "buttonreader.hpp"

#include "Mahjong.h"
/* Parts used:
  4x 02210
  1x 01027
  3x 01691
  1x 02266
  1x 00973
  1x 00199
*/

#define DEBUG true
#define displayAddress 0x27
#define displayHeight 2
#define displayWidth 16

int pot = 0;

// Player definitions
Player::Player(): score(0), riichi(false){}
bool Player::callRiichi(){
    if (score>=10 && riichi == false){
      score -= 10;
      pot += 10;
      riichi = true;
    } return riichi;
  }

// Game definitions
  Game::Game() : kiriage(1), hand(1) {} //default constructor
  
  Wind Game::getRoundWind(){return (Wind)((hand / 4u)+1);}
  byte Game::getKyoku() {return hand % 4u;}

  int Game::calculateBasePoints(const byte &han, const byte &fu) { //Calculates base points from han and fu
    if (han >=13) {
      return floor(han/13) * 8000;
    }
    if (han>=5) {
      switch (han) {
        case 5:
          return 2000;
          break;
        case 6:
        case 7:
          return 3000;
          break;
        case 8:
        case 9:
        case 10:
          return 4000;
          break;
        case 11:
        case 12:
          return 5000;
          break;
        default:
          break;
      }
    }
    uint basePoints = fu * 2^(2 + han);
    return (basePoints == 1920 && kiriage)? 2000 : basePoints;
  }


  Game& Game::ron(const byte &winner, const int &basePoints, const byte &opponent) {
    byte kyoku = getKyoku();
    bool dealerWin = (winner == kyoku);
    int x = (round(basePoints * (winner == kyoku ? 6 : 4) / 100) + honba * 3);
    players[opponent].score -= x;
    players[winner].score += x + pot;
    dealerWin ? ++honba : honba = 0;
    pot = 0;
    return *this;
  }

  Game& Game::tsumo(const byte &winner, const int &basePoints) {
    byte kyoku = getKyoku();
    bool dealerWin = (winner == kyoku);
    int y = basePoints * (winner == kyoku ? 2 : 1);
    int x = 0;
    for (byte i = 0; i < 4; i++) {
      if (i == winner) {break;}
      int temp = (i == kyoku ? round(y / 50) * 100 : round(y / 100)+ honba);
      players[i].score -= temp;
      x += temp;
    }
    players[winner].score += x + pot;
    dealerWin ? ++honba : honba = 0;
    pot = 0;
    return *this;
  }

const byte keypadRows = 4;
const byte keypadCols = 4;

const char keypadKeys[keypadRows][keypadCols] = {
  {'1', '2', '3', 'N'},
  {'4', '5', '6', 'E'},
  {'7', '8', '9', 'S'},
  {'*', '0', '#', 'W'}
};
const char numToKeypad[] = {};
byte rowPins[keypadRows] = {4,5,6,7};
byte colPins[keypadCols] = {8,9,10,11};

Keypad numpad = Keypad((char*)keypadKeys, rowPins, colPins, keypadRows, keypadCols);

//bitmaps for kanji characters, same order as Wind enum, 2 bytes per char cause mainDisplay char is 5x8
const PROGMEM uint8_t windKanji[8][8] = {
  {
    B00001,
    B00001,
    B00111,
    B00001,
    B00011,
    B00101,
    B00001,
    B00000
  },
  {
    B01000,
    B01000,
    B01010,
    B01100,
    B01000,
    B01010,
    B01110,
    B00000
  },
  {
    B00000,
    B00111,
    B00010,
    B00011,
    B00010,
    B00001,
    B00110,
    B00000
  },
  {
    B10000,
    B11110,
    B10100,
    B11100,
    B10100,
    B11000,
    B10110,
    B00000
  },
  {
    B00000,
    B00111,
    B00000,
    B00111,
    B00101,
    B00111,
    B00100,
    B00000
  },
  {
    B10000,
    B11110,
    B10000,
    B11110,
    B01010,
    B11110,
    B10010,
    B00000
  },
  {
    B00111,
    B00001,
    B00111,
    B00101,
    B00110,
    B00100,
    B00111,
    B00000
  },
  {
    B11110,
    B01000,
    B11110,
    B01010,
    B01110,
    B00010,
    B11110,
    B00000
  },
};
hd44780_I2Cexp mainDisplay;


state cState; //current state
bool stateChanged; //changed state since last cycle

Game game; //the current game


void setup() {
  Serial.begin(9600);
  numpad.setHoldTime(29);
  mainDisplay.begin(displayWidth,displayHeight);
  mainDisplay.lineWrap();
  for (byte i = 0; i < 8; i++)
  {
    mainDisplay.createChar(i,windKanji[i]);
  }
  for (byte i = 0; i < 4; i++)
  {
    game.players[i].scoreDisplay.begin(0x70 + i);
  }
  
  

  cState = configMenu;
}

void loop () {
  static int debouncetimer = (millis() % 250);
  static bool firstRun = true;
  while((millis() % 250)-debouncetimer < 60);
  debouncetimer = (millis() % 250);
  if (numpad.getKeys() || firstRun){
    firstRun = false;
    switch (cState)
    {
    case configMenu:
      configMenuLoop();
      break;
    case debugMenu:
      debugMenuLoop();
      break;
    case gameMenu:
      gameMenuLoop();
      break;
    case playerMenu:
      playerMenuLoop();
      break;
    case running:
      runningLoop();
      break;
    }
    if (cState == running && stateChanged) runningLoop();
    for (auto &&player : game.players)
    {
      for (byte i = 0; i < 4; i++)
      {
        int n = pow(10,i);
        player.scoreDisplay.writeDigitAscii(3-i,'0'+ ((player.score / n) % 10),i==1);
      }
      
      player.scoreDisplay.writeDisplay();
    }
    
  }
}

void runningLoop(){
  if (stateChanged && cState == running){ //change handler
    mainDisplay.clear();
    writeKanji(game.getRoundWind());
    mainDisplay.print(game.getKyoku());
    mainDisplay.print(" Honba:");
    mainDisplay.print(game.honba);
    mainDisplay.setCursor(0,1);
    mainDisplay.print("Pot: ");
    mainDisplay.print(pot);
    stateChanged = false;
    return;
  }

  //if (numpad.isPressed(''))
  if (numpad.isPressed('N') || numpad.isPressed('E') || numpad.isPressed('S') || numpad.isPressed('W')) {
    cState = playerMenu;
    stateChanged = true;
    playerMenuLoop();
    }
}

void configMenuLoop(){
  static byte menuPosition = 0;
  if (numpad.isPressed('*')) {
    cState = running;
    stateChanged = true;
    return;
  }
  mainDisplay.print("placeholder");
  for (auto &&player : game.players)
  {
    player.score = game.startingPoints;
  }
  
}

void gameMenuLoop(){
  static byte menuPosition = 0;
  

}

void debugMenuLoop(){
  static byte menuPosition = 0;
  

}

void playerMenuLoop(){
  enum playerMenuPosition : byte {root = 0, ron = 1, tsumo = 2, riichi = 3, ronYaku = 10, ronFu = 11, ronConfirm = 12, tsumoYaku = 20, tsumoFu = 21, tsumoConfirm = 22};
  static bool positionChanged;
  static playerMenuPosition menuPosition = root; //0: root, 1*: ron, 2*: tsumo, 3*: riichi
  static Wind menuOwner;
  static Wind target;
  static byte han;
  static byte fu;
  switch (menuPosition){
    case root:
    {
      if (stateChanged || positionChanged) { // change handler
        if (stateChanged){
          for (byte i = 0; i < 4; i++)
          {
            if (numpad.isPressed(keypadKeys[i][3])){
              menuOwner = Wind(i);
              break;
            }
          }
        }
        mainDisplay.clear();
        writeKanji(menuOwner);
        mainDisplay.print(" 1:Ron 2:Tsumo 3:Riichi");
        //mainDisplay.setCursor(0,1);
        //mainDisplay.print("")
        positionChanged = stateChanged = false;
        return;
      }
      for (byte i = 0; i < 3; i++)
      {
        if(numpad.isPressed('0'+ i)){
          target = menuOwner;
          mainDisplay.clear();
          writeKanji(menuOwner);
          positionChanged = true;
          menuPosition = playerMenuPosition(i);
          break;
        }
      }
      if(numpad.isPressed('*')){
        cState = running;
        menuOwner = North;
        target = North;
        stateChanged = true;
        return;
      }
    } break;

    case ron:
    {
      if (positionChanged){ // change handler
        mainDisplay.setCursor(2,0);
        mainDisplay.print(" Ron who?  ");
        writeKanji(target);
        mainDisplay.setCursor(0,1);
        mainDisplay.print("# to confirm");
        
        positionChanged = false;
        return;
      }
      if (numpad.isPressed('*')){
        menuPosition = root;
        positionChanged = true;
        return;
      }
      if (numpad.isPressed('#')){
        if (target != menuOwner) {
          menuPosition = ronYaku;
          positionChanged = true;
          return;
        }
      }
      for (byte i = 0; i < 4; i++)
      {
        if (numpad.isPressed(keypadKeys[i][3])){
          target = Wind(i);
          positionChanged = true;
        break;
        }
      } return;
    } break;

    case tsumo:
    {
      if (positionChanged){ // change handler
        mainDisplay.setCursor(2,0);
        mainDisplay.print(" Tsumo?");
        mainDisplay.setCursor(0,1);
        mainDisplay.print("# to confirm");
        
        positionChanged = false;
        return;
      }
      if (numpad.isPressed('*')){
        menuPosition = root;
        positionChanged = true;
        return;
      }
      if (numpad.isPressed('#')){
        menuPosition = tsumoYaku;
        positionChanged = true;
        return;
      }
      return;
    } break;

    case riichi:
    {
      if (positionChanged){ // change handler
        mainDisplay.setCursor(2,0);
        if(game.players[menuOwner].riichi) {
          mainDisplay.print(" is already");
          mainDisplay.setCursor(0,1);
          mainDisplay.print("Riichi!");
          menuPosition = root;
          return;
        }
        mainDisplay.print(" Riichi?");
        mainDisplay.setCursor(0,1);
        mainDisplay.print("# to confirm");
        positionChanged = false;
        return;
      }
      if (numpad.isPressed('*')){
        menuPosition = root;
        positionChanged = true;
        return;
      }
      if (numpad.isPressed('#')){
        mainDisplay.setCursor(2,0);
        if (game.players[menuOwner].callRiichi()){
          mainDisplay.print(" has gone");
          mainDisplay.setCursor(0,1);
          mainDisplay.print("RIICHI!!!");
        } else {
          mainDisplay.print(" does not have");
          mainDisplay.setCursor(0,1);
          mainDisplay.print("enough points!");
        }
        delay(1000);
        menuPosition = root;
        cState = running;
        stateChanged = true;
        return;
      }
    } break;

    case ronYaku:
    {
      if (positionChanged) {
        mainDisplay.clear();
        writeKanji(menuOwner);
        mainDisplay.print("R->");
        writeKanji(target);
        mainDisplay.setCursor(0,1);
        mainDisplay.print("Han: ");
        han = 0;
        positionChanged = false;
        return;
      }
      if (numpad.isPressed('*')){
        menuPosition = ron;
        positionChanged = true;
        return;
      }
      if (numpad.isPressed('#') && han > 0){
        menuPosition = ronFu;
        positionChanged = true;
        return;
      }
      char nextNumber = keypadInputByte(han);
      if (nextNumber){
        mainDisplay.print(nextNumber);
      }
      return;
    } break;

    case ronFu:
    {
      if (positionChanged) {
        mainDisplay.clear();
        writeKanji(menuOwner);
        mainDisplay.print("R->");
        writeKanji(target);
        mainDisplay.print("Han: ");
        mainDisplay.print(han);
        mainDisplay.setCursor(0,1);
        mainDisplay.print("Fu: "); 
        fu = 0;
        positionChanged = false;
        return;
      }
      if (numpad.isPressed('*')){
        menuPosition = ronYaku;
        positionChanged = true;
        return;
      }
      if (numpad.isPressed('#') && fu > 0){
        menuPosition = ronConfirm;
        positionChanged = true;
        return;
      }
      char nextNumber = keypadInputByte(fu);
      if (nextNumber){
        mainDisplay.print(nextNumber);
      }
      return;
    } break;

    case ronConfirm: //UNIMPL
    {
      if (positionChanged) {
        mainDisplay.clear();
        writeKanji(menuOwner);
        mainDisplay.print("R->");
        writeKanji(target);
        mainDisplay.print("Han: ");
        mainDisplay.print(han);
        mainDisplay.setCursor(0,1);
        mainDisplay.print("Fu: "); 
        fu = 0;
        positionChanged = false;
        return;
      }
      if (numpad.isPressed('*')){
        menuPosition = ronYaku;
        positionChanged = true;
        return;
      }
      if (numpad.isPressed('#') && fu > 0){
        menuPosition = ronFu;
        positionChanged = true;
        return;
      }
    } break;
    case tsumoYaku:
    {
      if (positionChanged) {
        mainDisplay.clear();
        writeKanji(menuOwner);
        mainDisplay.print("T");
        mainDisplay.setCursor(0,1);
        mainDisplay.print("Han: ");
        han = 0;
        positionChanged = false;
        return;
      }
      if (numpad.isPressed('*')){
        menuPosition = tsumo;
        positionChanged = true;
        return;
      }
      if (numpad.isPressed('#') && han > 0){
        menuPosition = tsumoFu;
        positionChanged = true;
        return;
      }
      char nextNumber = keypadInputByte(han);
      if (nextNumber){
        mainDisplay.print(nextNumber);
      }
      return;
    } break;
    default:
    {
      menuPosition = root;
      positionChanged = true;
    } break;
  }
}

void writeKanji(const Wind &kanji){ //writes 2-character wide kanji
  mainDisplay.write(kanji*2);
  mainDisplay.write(kanji*2+1);
}

char keypadInputByte(byte &temp) {
  if(temp <= 25) { //make sure we don't try to go over 255
    for (byte i = 0; i < 10; i++)
    {
      char c = '0'+ i;
      if (numpad.isPressed(c)) {
        if(temp == 25 && i > 5){ //make sure we don't try to go over 255
          return 0;
        }
        temp = temp * 10 + i;
        return c;
      }
    }
  }
  return 0;
}