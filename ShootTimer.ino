// include the library code:
#include <LiquidCrystal.h>
#include <EEPROMex.h>
#include <EEPROMVar.h>
#include "pitches.h"
#include "constants.h"

// initialize lcd with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

#include "specialChar.h"

////////////////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIALBES
////////////////////////////////////////////////////////////////////////////////////////

unsigned int menuIndex = 4;

//Configuration values
unsigned int cfgThreshold = 5;
unsigned int cfgMode = MODE_NORMAL;
unsigned int cfgParTime = 10;
unsigned int cfgMin = 2;
unsigned int cfgMax = 6;

//Icon configuration
byte icn_first;
byte icn_last;
byte icn_count;
byte icn_split;

//Shot times
unsigned long shots[100];
unsigned int  shotCount;
unsigned int  shotIndex;

//Menu labels
char* menuOptions[] = {
  "Delay max",
  "Delay min",
  "Par time",
  "Mode",
  "Threshold",
};

////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------
// displayTime
//----------------------------------------------------
// Display format ###.##
void displayTime(unsigned long time, unsigned int col, unsigned int row)
{
  unsigned int seconds;
  unsigned int cents;

  seconds = (unsigned int)(time / 1000L);
  cents   = (unsigned int)(time % 1000L) / 10;

  lcd.setCursor(col, row);
  lcd.print(seconds);
  lcd.print(".");
  lcd.print(cents);
}

//----------------------------------------------------
// displayCount
//----------------------------------------------------
//Format ##/##
void displayCount(unsigned int total, unsigned int idx, unsigned int col, unsigned int row)
{
  lcd.setCursor(col, row);
  lcd.print(idx);
  lcd.print("/");
  lcd.print(total);
}

//----------------------------------------------------
// displayText
//----------------------------------------------------
// Display format *
void displayText(char* c, unsigned int col, unsigned int row)
{
  lcd.setCursor(col, row);
  lcd.print(c);
}

//----------------------------------------------------
// displayText
//----------------------------------------------------
// Display format *
void displayNum(unsigned int num, unsigned int col, unsigned int row)
{
  lcd.setCursor(col, row);
  lcd.print(num);
}
//----------------------------------------------------
// displayChar
//----------------------------------------------------
// Display format *
void displayChar(unsigned int c, unsigned int col, unsigned int row)
{
  lcd.setCursor(col, row);
  lcd.write((byte) c);
}

//----------------------------------------------------
// configIcons
//----------------------------------------------------
void configIcons(byte first, byte count, byte last, byte split)
{
  icn_first = first;
  icn_count = count;
  icn_last  = last;
  icn_split = split;

  displayChar(icn_first, 0, 1);
  displayChar(icn_count, 0, 0);
  displayChar(icn_last,  7, 1);
  displayChar(icn_split, 7, 0);
}

//----------------------------------------------------
// updateDisplay times
//----------------------------------------------------
void updateDisplay(unsigned long first, unsigned long last, unsigned long split, unsigned int count, unsigned int index)
{
  lcd.clear();
  displayChar(icn_first, 0, 1);
  displayChar(icn_count, 0, 0);
  displayChar(icn_last,  7, 1);
  displayChar(icn_split, 7, 0);

  if (cfgMode == MODE_PAR)
  {
    displayChar(CHAR_CLOCK, 15, 0);
  }

  displayTime(first, 1, 1);
  displayCount(count, index, 1, 0);
  displayTime(last, 8, 1);
  displayTime(split, 8, 0);

}

//----------------------------------------------------
// readKey
//----------------------------------------------------
//Returns key value or null
unsigned int readKey()
{
  unsigned int value = analogRead(KEY_PIN);
  unsigned int key;

  if (value < 50)
  {
    key = KEY_RIGHT;
  }
  else if (value < 200)
  {
    key = KEY_UP;
  }
  else if (value < 350)
  {
    key = KEY_DOWN;
  }
  else if (value < 550)
  {
    key = KEY_LEFT;
  }
  else if (value < 810)
  {
    key = KEY_SELECT;
  }
  else {
    return KEY_NONE;
  }
  tone(TONE_PIN, NOTE_C4, 50);
  delay(KEY_DELAY);
  return key;
}

//----------------------------------------------------
// detectShot
//----------------------------------------------------
//Returns millis if threshold is exceeded
unsigned long detectShot(unsigned int threshold)
{
  unsigned long start = millis(); // Start of sample window
  unsigned int  signalMax = 0;
  unsigned int  signalMin = 1024;
  unsigned int  sample;
  double        value;

  // collect data for 50 mS
  while (millis() - start < SIG_WINDOW)
  {
    sample = analogRead(SIG_PIN);
    if (sample < 1024)  // toss out spurious readings
    {
      if (sample > signalMax)
      {
        signalMax = sample;  // save just the max levels
      }
      else if (sample < signalMin)
      {
        signalMin = sample;  // save just the min levels
      }
    }
  }
  value = ((signalMax - signalMin) * 100.0) / 1024;

  if (value > threshold)
  {
    return start;
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------
// startDelay
//----------------------------------------------------
void startDelay(unsigned int low, unsigned int hi)
{
  unsigned long time;

  lcd.clear();

  if (low != hi)
  {
    low = random(low, hi + 1);
  }
  displayText("Count down:", 0, 0);
  displayText(">", 0, 1);

  time = low * 1000L;

  while (time)
  {
    displayTime(time, 1, 1);
    time = time - 200L;
    delay(200);
  }
  displayTime(time, 1, 1);
}

//----------------------------------------------------
// timer
//----------------------------------------------------
//Normal mode
void timer(unsigned int mode)
{
  unsigned long first;
  unsigned long last;
  unsigned long initial;
  unsigned long current;
  unsigned long split;
  unsigned int  key = KEY_NONE;
  unsigned long parTime;
  bool          finish;

  shotCount = 0;
  shotIndex = 0;

  finish = false;
  parTime = cfgParTime * 1000;

  configIcons(CHAR_FIRST, CHAR_NUM, CHAR_PLAY, CHAR_SPLIT);
  updateDisplay(first, last, split, 0, 0);

  tone(TONE_PIN, NOTE_C7, 250);
  delay(255);

  //To discard seconds since Arduino start.
  initial = millis();

  while (key == KEY_NONE && !finish)
  {
    /*Par time mode*/
    if (mode == MODE_PAR) {
      current = millis() - initial;
      if (current > parTime)
      {
        finish = true;
        continue;
      }
    }

    current = detectShot(cfgThreshold);

    if (current)
    {
      current = current - initial;
      shotCount++;
      shotIndex++;
      shots[shotCount] = current;

      if (!first)
      {
        first = current;
      }

      split = current - last;
      last  = current;

      updateDisplay(first, last, split, shotCount, shotIndex);
    }
    key  = readKey();
  }
  if (finish)
  {
    tone(TONE_PIN, NOTE_C7, 250);
    delay(251);
  }
}

//----------------------------------------------------
// navigateShot
//----------------------------------------------------
void navigateShot(unsigned int key)
{
  if (!shotCount)
  {
    updateDisplay(0, 0, 0, 0, 0);
    return;
  }
  if (key == KEY_UP)
  {
    shotIndex = (shotIndex == shotCount) ? 1 : shotIndex + 1;

    updateDisplay(shots[1], shots[shotIndex], shots[shotIndex] - shots[shotIndex - 1], shotCount, shotIndex);
  }
  else if (key == KEY_DOWN)
  {
    shotIndex = (shotIndex == 1) ? shotCount : shotIndex - 1;

    updateDisplay(shots[1], shots[shotIndex], shots[shotIndex] - shots[shotIndex - 1], shotCount, shotIndex);
  }
}

//----------------------------------------------------
// displayMenuOption
//----------------------------------------------------
void displayMenuOption(unsigned int option)
{
  lcd.clear();
  displayChar(CHAR_WRENCH, 0, 0);
  displayText(menuOptions[menuIndex], 1, 0);
  displayText(">", 0, 1);

  switch (option)
  {
    case MNU_THRESHOLD:
      displayNum(cfgThreshold, 1, 1);
      break;
    case MNU_MODE:
      if (cfgMode == MODE_NORMAL)
        displayText("Normal", 1, 1);
      else
        displayText("Par", 1, 1);
      break;
    case MNU_PARTIME:
      displayNum(cfgParTime, 1, 1);
      break;
    case MNU_MIN:
      displayNum(cfgMin, 1, 1);
      break;
    case MNU_MAX:
      displayNum(cfgMax, 1, 1);
      break;
  }
}

//----------------------------------------------------
// menu
//----------------------------------------------------
void menu() {
  int key;
  lcd.clear();

  displayMenuOption(menuIndex);

  while (key != KEY_SELECT)
  {
    key = readKey();

    if (key == KEY_UP)
    {
      menuIndex = (menuIndex == 4) ? 0 : menuIndex + 1;
      displayMenuOption(menuIndex);
    }
    else if (key == KEY_DOWN)
    {
      menuIndex = (menuIndex == 0) ? 4 : menuIndex - 1 ;
      lcd.clear();
      displayMenuOption(menuIndex);
    }
    /*Threshold*/
    else if (key == KEY_RIGHT && menuIndex == MNU_THRESHOLD)
    {
      cfgThreshold = (cfgThreshold == 95) ? 5 : cfgThreshold + 5;
      displayMenuOption(menuIndex);
    }
    else if (key == KEY_LEFT && menuIndex == MNU_THRESHOLD)
    {
      cfgThreshold = (cfgThreshold == 5) ? 95 : cfgThreshold - 5;
      displayMenuOption(menuIndex);
    }
    /*Mode*/
    else if ( (key == KEY_RIGHT || key == KEY_LEFT) && menuIndex == MNU_MODE)
    {
      cfgMode = (cfgMode == MODE_NORMAL) ? MODE_PAR : MODE_NORMAL;
      displayMenuOption(menuIndex);
    }
    /*Par Time */
    else if (key == KEY_RIGHT && menuIndex == MNU_PARTIME)
    {
      cfgParTime = (cfgParTime == 300) ? 1 : cfgParTime + 1;
      displayMenuOption(menuIndex);
    }
    else if (key == KEY_LEFT && menuIndex == MNU_PARTIME)
    {
      cfgParTime = (cfgParTime == 1) ? 300 : cfgParTime - 1;
      displayMenuOption(menuIndex);
    }
    /*Delay min */
    else if (key == KEY_RIGHT && menuIndex == MNU_MIN)
    {
      cfgMin = (cfgMin == 10) ? 1 : cfgMin + 1;
      cfgMin = (cfgMin > cfgMax) ? cfgMax : cfgMin;
      displayMenuOption(menuIndex);
    }
    else if (key == KEY_LEFT && menuIndex == MNU_MIN)
    {
      cfgMin = (cfgMin == 1) ? 10 : cfgMin - 1;
      cfgMin = (cfgMin > cfgMax) ? cfgMax : cfgMin;
      displayMenuOption(menuIndex);
    }
    /*Delay max */
    else if (key == KEY_RIGHT && menuIndex == MNU_MAX)
    {
      cfgMax = (cfgMax == 10) ? 1 : cfgMax + 1;
      cfgMax = (cfgMin > cfgMax) ? cfgMin : cfgMax;
      displayMenuOption(menuIndex);
    }
    else if (key == KEY_LEFT && menuIndex == MNU_MAX)
    {
      cfgMax = (cfgMax == 1) ? 10 : cfgMax - 1;
      cfgMax = (cfgMin > cfgMax) ? cfgMin : cfgMax;
      displayMenuOption(menuIndex);
    }
  }
  safe();
}

//----------------------------------------------------
// safe
//----------------------------------------------------
int ePut(int ee, unsigned int &value)
{
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          EEPROM.write(ee++, *p++);
    return i;
}

//----------------------------------------------------
// safe
//----------------------------------------------------
int eGet(unsigned int ee, unsigned int &value)
{
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          *p++ = EEPROM.read(ee++);
    return i;
}

//----------------------------------------------------
// safe
//----------------------------------------------------
void safe()
{
  EEPROM.updateInt(ADR_THRESHOLD, cfgThreshold);
  EEPROM.updateInt(ADR_MODE, cfgMode);
  EEPROM.updateInt(ADR_PARTIME, cfgParTime);  
  EEPROM.updateInt(ADR_MIN, cfgMin);
  EEPROM.updateInt(ADR_MAX, cfgMax);

}

//----------------------------------------------------
// load
//----------------------------------------------------

void load()
{
  cfgThreshold = EEPROM.readInt(ADR_THRESHOLD);
  cfgMode = EEPROM.readInt(ADR_MODE);
  cfgParTime = EEPROM.readInt(ADR_PARTIME);  
  cfgMin = EEPROM.readInt(ADR_MIN);  
  cfgMax = EEPROM.readInt(ADR_MAX);
}

////////////////////////////////////////////////////////////////////////////////////////
// SETUP
////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
  //Random seed
  randomSeed(analogRead(SIG_PIN));

  // Turn on backlight
  pinMode(LCD_BL_PIN, OUTPUT);
  digitalWrite(LCD_BL_PIN, LCD_BL_ON);

  //Key read pin
  pinMode(KEY_PIN, INPUT);

  //create a new character
  lcd.createChar(CHAR_SPLIT, split);
  lcd.createChar(CHAR_PLAY, play);
  lcd.createChar(CHAR_FLAG, flag);
  lcd.createChar(CHAR_FIRST, first);
  lcd.createChar(CHAR_NUM, num);
  lcd.createChar(CHAR_WRENCH, wrench);
  lcd.createChar(CHAR_CLOCK, clock);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  
  load();
}

////////////////////////////////////////////////////////////////////////////////////////
// MAIN LOOP
////////////////////////////////////////////////////////////////////////////////////////

void loop()
{
  int key = KEY_NONE;

  lcd.clear();
  lcd.print("Stand by...");

  if (cfgMode == MODE_PAR)
  {
    displayChar(CHAR_CLOCK, 15, 0);
  }

  while (key == KEY_NONE)
  {
    key = readKey();
  }

  if (key == KEY_SELECT)
  {
    startDelay(cfgMin, cfgMax);
    timer(cfgMode);

    configIcons(CHAR_FIRST, CHAR_NUM, CHAR_FLAG, CHAR_SPLIT);

    key = KEY_NONE;

    while (key != KEY_SELECT)
    {
      key = readKey();

      if (key == KEY_UP || key == KEY_DOWN)
      {
        navigateShot(key);
      }
    }
  }
  else
  {
    menu();
  }
}

