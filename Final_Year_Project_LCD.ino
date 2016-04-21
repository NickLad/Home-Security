/*
#####################################################################################
#   File:               Final_Year_Project_LCD                                       
#   Micro controller:   Arduino Mega 2560   
#   Language:           Wiring / C /Processing /Fritzing / Arduino IDE
#  
#   Objectives:         Arduino RFID - Security System and Access Control
#                     
# Operation:            Using the RFID RC522 reader, we can do the personal access control
#                       to an specific environment.
#                       To do this we need to check the card or tag ID
#                       If a valid ID is found it is possible to lit a LED, play a soundand drive a servo-motor
#                       to open the door or gate - if an invalid ID is found, it is possible to do the access rejection
#                       flashing a red LED and playing an alert sound and locking the door.
#                       A LCD (20 columm x 4 rows shos us the system messages
#
#     
#   Author:              Chee Aik Lim 
#   Date:                26/02/16 
#   Place:               Leicester, United Kingdom
#         
#####################################################################################
*/

// libraries
#include <SPI.h>
#include <RFID.h>
#include "pitches.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <Keypad.h>

// RFID definition 
RFID rfid(53,5);

int serNum[5];
int data[5];
int cardRead; // card read 0 = no playTune & 1 = playTune

// Melodies definition: access, welcome and rejection
int access_melody[] = {NOTE_G4,0, NOTE_A4,0, NOTE_B4,0, NOTE_A4,0, NOTE_B4,0, NOTE_C5,0};
int access_noteDurations[] = {8,8,8,8,8,4,8,8,8,8,8,4};
int denied_melody[] = {NOTE_G2,0, NOTE_F2,0, NOTE_D2,0};
int denied_noteDurations[] = {8,8,8,8,8,4};

// 5 data int bytes from cards and tag
int nick[5] = {251,176,30,43,126};
int newuser1[5] = {211,223,163,0,175};
// Add allowed card IDs here

//pins for LED and buzzer
int LED_access = 3;
int LED_intruder = 6;
int speaker = 10;

Servo doorLock;

//set our password
char* ourPassword = "1314";
int currentPosition = 0;

// LCD address and type declaration
LiquidCrystal_I2C lcd(0x27,20,4);
 
//define the keypad
const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns

char keys[ROWS][COLS] = {
   {'1','2','3','A'},
   {'4','5','6','B'},
   {'7','8','9','C'},
   {'*','0','#','D'}
};

byte rowPins[ROWS] = {30, 31, 32, 33}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {34, 35, 36, 37}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
 
void setup()
{
  Serial.begin(9600); // Serial communication initialization
  lcd.init();
  lcd.backlight();
  lcd.clear();
  SPI.begin();  // SPI communication initialization
  rfid.init();   // RFID module initialization
  displayLCDInitial();
  doorLock.attach(9);
  pinMode(LED_access, OUTPUT);
  pinMode(LED_intruder, OUTPUT);
  pinMode(speaker, OUTPUT);
}
 
void loop()
{ 
  int i;
  
  char key = keypad.getKey();

  if(int(key) != 0)
  { 
    lcd.clear();
    lcd.setCursor(0,0);
    displayCodeEntryScreen();
    lcd.setCursor(0,1);
    playTune(300,600);
      
    for(i=0; i<=currentPosition; ++i) 
    {
      lcd.print("*");
    }

    if(key == ourPassword[currentPosition])
    {
      ++currentPosition;
      if(currentPosition == 4)
      { 
        currentPosition = 0;
        cardRead = 1;
        playTone(cardRead);
        unlockDoor();
       
      }
     } else {
       cardRead = 0;
       playTone(cardRead);
       invalidCode();
       currentPosition = 0;
    }
  }
  
  // NAME_card and KEY_card
  boolean nick_card = true; // that is my card 
  boolean newuser1_card = true;
  //put another users here

  if(rfid.isCard()) // valid card found 
  {
     
    if(rfid.readCardSerial()) // reads the card
    {
      delay(1000);
      data[0] = rfid.serNum[0]; // stores the serial number
      data[1] = rfid.serNum[1];
      data[2] = rfid.serNum[2];
      data[3] = rfid.serNum[3];
      data[4] = rfid.serNum[4];
    }
    rfid.halt();
    cardRead = 0;
    displayID_LCD();
    for(int i=0; i<5; i++)
    {
      if(data[i] != nick[i]) nick_card = false; // if it is not a valid card, put false to this user
      if (data[i] != newuser1[i]) newuser1_card = false;
      // Here you can check the another allowed users, just put lines like above with the user name
    }

    if (nick_card) { // if a valid card was found
       cardRead = 1;
       displayAccess("Nick");
       playTone(cardRead);
       
    }
    else if(newuser1_card){
       cardRead = 1;
       displayAccess("Lily");
       playTone(cardRead);
    }
    //another cards analysis, put many blocks like this 
    // as many users you have 
    else { //if a card is not recognized
         digitalWrite(LED_intruder,HIGH);
         displayDenied();
         delay(2000);
         digitalWrite(LED_intruder, LOW);
         playTone(cardRead);
         delay(2000);
         displayLCDInitial();
    }

    if (nick_card || newuser1_card) // add another user using an logical or condition || 
    {
        
        digitalWrite(LED_access,HIGH); // turn on the green LED
        doorLock.write(0);
        delay(6000); // wait 5 senconds
        digitalWrite(LED_access, LOW); // turn off green LED
        doorLock.write(90);
        
    }
    delay(2000);
    rfid.halt();
    lcd.clear();
    displayLCDInitial();
  }

}
//==== Play Tone for acess granted or denied tune =========
void playTone(int Scan) {    
  
  if (Scan == 1) // A valid card
  { 
    for (int i = 0; i < 12; i++)    //loop through the notes
        { // Good card read
          int access_noteDuration = 1000 / access_noteDurations[i];
          tone(speaker, access_melody[i], access_noteDuration);
          int access_pauseBetweenNotes = access_noteDuration * 1.30;
          delay(access_pauseBetweenNotes);
          noTone(speaker);
       }
  }     
    else // An invalid card
       for (int i = 0; i < 6; i++)    //loop through the notes 
       { 
          int denied_noteDuration = 1000 / denied_noteDurations[i];
          tone(speaker, denied_melody[i], denied_noteDuration);
          int denied_pauseBetweenNotes = denied_noteDuration * 1.30;
          delay(denied_pauseBetweenNotes);
          noTone(speaker);
       }
  }

void playTune(long duration, int freq) //playTune when keypad press
{
  duration *=1000;
  int period = (1.0 / freq) * 1000000;
  long elapsed_time = 0;
  while(elapsed_time < duration)
  {
    digitalWrite(speaker, HIGH);
    delayMicroseconds(period / 2);
    digitalWrite(speaker, LOW);
    elapsed_time += (period);
  }
}

//==== Display LCD messages for the users for KeyPad ========
void displayCodeEntryScreen()
{
    lcd.backlight();
    lcd.setCursor(1,0);
    lcd.print("Enter Password:");
}

void unlockDoor()
{ 
  digitalWrite(LED_access, HIGH);
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Access Granted");
  lcd.setCursor(0,1);
  lcd.print("Welcome");

  //add any code to unclock the door here
  doorLock.write(0);
  delay(3000);
  doorLock.write(90);
  digitalWrite(LED_access, LOW);
  lcd.clear();
  displayLCDInitial();
}

void invalidCode()
{ 
  int i = 0;
  
  digitalWrite(LED_intruder, HIGH);
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Access Denied");
  lcd.setCursor(0,1);
  lcd.print("Wrong Password");

  delay(2000);
  digitalWrite(LED_intruder, LOW);
  lcd.clear();
  displayLCDInitial();
  
}

//==== Display LCD messages for the users for RFID ========
void displayLCDInitial()
{
  lcd.backlight();
  lcd.setCursor(2,0);
  lcd.print("Scan Card or");
  lcd.setCursor(2,1);
  lcd.print("Press KeyPad");
  
}

void displayAccess(String user)
{   
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("  Welcome ");
    lcd.print(user);
    lcd.setCursor(2,1);
    lcd.print(" Door Unlock ");
}

void displayDenied()
{   
    lcd.backlight();
    lcd.setCursor(2,0);
    lcd.print("Access Denied");
    lcd.setCursor(0,1);
    
}

void displayID_LCD() 
{
    lcd.backlight();
    lcd.setCursor(1,0);
    lcd.print("Card is found");
    lcd.setCursor(0,1);
    lcd.print("ID = ");
    if(rfid.serNum[0] < 16){
      lcd.print("0");
    }
    lcd.print(rfid.serNum[0],DEC);
  
    if(rfid.serNum[1] < 16){
       lcd.print("0");
    }
    lcd.print(rfid.serNum[1],DEC);
  
    if(rfid.serNum[2] < 16){
      lcd.print("0");
    }
    lcd.print(rfid.serNum[2],DEC);
  
    if(rfid.serNum[3] < 16){
      lcd.print("0");
    }
    lcd.print(rfid.serNum[3],DEC);
  
    if(rfid.serNum[4] < 16){
      lcd.print("0");
    }
    lcd.print(rfid.serNum[4],DEC);
  
    delay(5000); // shows the ID for about 1000ms
    lcd.clear();
    lcd.noBacklight();
    
}




