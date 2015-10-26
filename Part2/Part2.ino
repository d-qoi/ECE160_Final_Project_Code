/*
 SparkFun Inventor's Kit
 Example sketch 16
 Spark Fun Electronics
 Oct. 7, 2014

 Simond Says is a memory game. Start this game by pressing one of the four buttons. When a button lights up, 
 press the button, reversing the sequence. The sequence will get longer and longer. This game is won after 
 72 rounds.

 Generates random sequence, plays music, and displays button lights.

 Simon tones from Wikipedia
 - A (red, upper left) - 440Hz - 2.272ms - 1.136ms pulse
 - a (green, upper right, an octave higher than A) - 880Hz - 1.136ms,
 0.568ms pulse
 - D (blue, lower left, a perfect fourth higher than the upper left) 
 587.33Hz - 1.702ms - 0.851ms pulse
 - G (yellow, lower right, a perfect fourth higher than the lower left) - 
 784Hz - 1.276ms - 0.638ms pulse

 Simon Says game originally written in C for the PIC16F88.
 Ported for the ATmega168, then ATmega328, then Arduino 1.0.
 Fixes and cleanup by Joshua Neal <joshua[at]trochotron.com>

 This sketch was written by SparkFun Electronics,
 with lots of help from the Arduino community.
 This code is completely free for any use.
 Visit http://www.arduino.cc to learn about the Arduino.
 */

/*************************************************
* Public Constants
*************************************************/
#include "notes.h"
#include <Charlieplex.h>

#define CHOICE_OFF      0 //Used to control LEDs
#define CHOICE_NONE     0 //Used to check buttons
#define CHOICE_RED      (1 << 0) //00011
#define CHOICE_GREEN    (1 << 1) //00110
#define CHOICE_BLUE     (1 << 2) //01100
#define CHOICE_YELLOW   (1 << 3) //11000


// Button pin definitions
#define BUTTON_RED    A0
#define BUTTON_GREEN  A1
#define BUTTON_BLUE   A2
#define BUTTON_YELLOW A3

// Buzzer pin definitions
#define BUZZER1  4
//#define BUZZER2  7

// Define game parameters
#define ROUNDS_TO_WIN      13 //Number of rounds to succesfully remember before you win. 13 is do-able.
#define ENTRY_TIME_LIMIT   3000 //Amount of time to press a button before game times out. 3000ms = 3 sec

#define MODE_MEMORY  1
#define MODE_BATTLE  2
#define MODE_BEEGEES 3
#define MODE_REVERSE 4

//Common LCD pins
//LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

//Charlieplex
byte LED_PIN_ONE = 6;
byte LED_PIN_TWO = 7;
byte LED_PIN_THREE = 8;

byte pins[] = {LED_PIN_ONE, LED_PIN_TWO, LED_PIN_THREE};
Charlieplex charlie = Charlieplex(pins, sizeof(pins));

charliePin LED_RED = {0, 1};
charliePin LED_GREEN = {1, 0};
charliePin LED_YELLOW = {2, 1};
charliePin LED_BLUE = {1, 2};


// Game state variables
byte gameMode = MODE_MEMORY; //By default, let's play the memory game
byte gameBoard[32]; //Contains the combination of buttons as we advance
byte gameRound = 0; //Counts the number of succesful rounds the player has made it through

void setup()
{
  //Setup hardware inputs/outputs. These pins are defined in the hardware_versions header file

  //Enable pull ups on inputs
  pinMode(BUTTON_RED, INPUT_PULLUP);
  pinMode(BUTTON_GREEN, INPUT_PULLUP);
  pinMode(BUTTON_BLUE, INPUT_PULLUP);
  pinMode(BUTTON_YELLOW, INPUT_PULLUP);

  //pinMode(LED_RED, OUTPUT);
  //pinMode(LED_GREEN, OUTPUT);
  //pinMode(LED_BLUE, OUTPUT);
  //pinMode(LED_YELLOW, OUTPUT);

  pinMode(BUZZER1, OUTPUT);
  //pinMode(BUZZER2, OUTPUT);

  //Mode checking
  gameMode = MODE_MEMORY; // By default, we're going to play the memory game

  // Check to see if the lower right button is pressed
  byte buttonchoice = checkButton();

  if (buttonchoice == CHOICE_YELLOW) {
    gameMode = MODE_BEEGEES;
    setLEDs(CHOICE_YELLOW);
    toner(CHOICE_YELLOW,150);

    while(checkButton() != CHOICE_NONE) ;

  }


  // Check to see if upper right button is pressed
  else if (buttonchoice == CHOICE_GREEN)
  {
    gameMode = MODE_BATTLE; //Put game into battle mode

    //Turn on the upper right (green) LED
    setLEDs(CHOICE_GREEN);
    toner(CHOICE_GREEN, 150);

    //setLEDs(CHOICE_RED | CHOICE_BLUE | CHOICE_YELLOW); // Turn on the other LEDs until you release button

    while(checkButton() != CHOICE_NONE) ; // Wait for user to stop pressing button

    //Now do nothing. Battle mode will be serviced in the main routine
  }

  play_winner(); // After setup is complete, say hello to the world
}

void loop()
{
  attractMode(); // Blink lights while waiting for user to press a button

  // Indicate the start of game play
  //setLEDs(CHOICE_RED | CHOICE_GREEN | CHOICE_BLUE | CHOICE_YELLOW); // Turn all LEDs on
  //delay(1000);
  //setLEDs(CHOICE_OFF); // Turn off LEDs
  //delay(250);

  if (gameMode == MODE_MEMORY)
  {
    // Play memory game and handle result
    setLEDs(CHOICE_RED);
    delay(1000);
    setLEDs(CHOICE_OFF);
    delay(250);
    if (play_memory() == true) 
      play_winner(); // Player won, play winner tones
    else 
      play_loser(); // Player lost, play loser tones
  }

  else if (gameMode == MODE_BATTLE)
  {
    setLEDs(CHOICE_GREEN);
    delay(1000);
    setLEDs(CHOICE_OFF);
    delay(250);
    play_battle(); // Play game until someone loses

    play_loser(); // Player lost, play loser tones
  }

  else if(gameMode == MODE_BEEGEES) {
    setLEDs(CHOICE_YELLOW);
    delay(1000);
    setLEDs(CHOICE_OFF);
    delay(250);

    play_beegees();
    play_loser();

  }

  if(gameMode == MODE_REVERSE) {
    setLEDs(CHOICE_BLUE);
    delay(1000);
    setLEDs(CHOICE_OFF);
    delay(250);
  }
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//The following functions are related to game play only

// Play the regular memory game
// Returns 0 if player loses, or 1 if player wins
boolean play_memory(void)
{
  randomSeed(millis()); // Seed the random generator with random amount of millis()

  gameRound = 0; // Reset the game to the beginning

  while (gameRound < ROUNDS_TO_WIN) 
  {
    add_to_moves(); // Add a button to the current moves, then play them back

    playMoves(); // Play back the current game board

    // Then require the player to repeat the sequence.
    for (byte currentMove = 0 ; currentMove < gameRound ; currentMove++)
    {
      byte choice = wait_for_button(); // See what button the user presses

      if (choice == 0) return false; // If wait timed out, player loses

      if (choice != gameBoard[currentMove]) return false; // If the choice is incorect, player loses
    }

    delay(1000); // Player was correct, delay before playing moves
  }

  return true; // Player made it through all the rounds to win!
}

// Play the special 2 player battle mode
// A player begins by pressing a button then handing it to the other player
// That player repeats the button and adds one, then passes back.
// This function returns when someone loses
boolean play_battle(void)
{
  gameRound = 0; // Reset the game frame back to one frame

  while (1) // Loop until someone fails 
  {
    byte newButton = wait_for_button(); // Wait for user to input next move
    gameBoard[gameRound++] = newButton; // Add this new button to the game array

    // Then require the player to repeat the sequence.
    for (byte currentMove = 0 ; currentMove < gameRound ; currentMove++)
    {
      byte choice = wait_for_button();

      if (choice == 0) return false; // If wait timed out, player loses.

      if (choice != gameBoard[currentMove]) return false; // If the choice is incorect, player loses.
    }

    delay(100); // Give the user an extra 100ms to hand the game to the other player
  }

  return true; // We should never get here
}

// Plays the current contents of the game moves
void playMoves(void)
{
  for (byte currentMove = 0 ; currentMove < gameRound ; currentMove++) 
  {
    toner(gameBoard[currentMove], 150);

    // Wait some amount of time between button playback
    // Shorten this to make game harder
    delay(150); // 150 works well. 75 gets fast.
  }
}

// Adds a new random button to the game sequence, by sampling the timer
void add_to_moves(void)
{
  byte newButton = random(0, 4); //min (included), max (exluded)

  // We have to convert this number, 0 to 3, to CHOICEs
  if(newButton == 0) newButton = CHOICE_RED;
  else if(newButton == 1) newButton = CHOICE_GREEN;
  else if(newButton == 2) newButton = CHOICE_BLUE;
  else if(newButton == 3) newButton = CHOICE_YELLOW;

  gameBoard[gameRound++] = newButton; // Add this new button to the game array
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//The following functions control the hardware

// Lights a given LEDs
// Pass in a byte that is made up from CHOICE_RED, CHOICE_YELLOW, etc
//integrate Charlieplex Here.

//Changes:
/*
I, Alex, swapped the digitalWrite with charlie.charlieWrite
All of the other code for this is the same.
When looking at the same lines in the part one, or the example that this is copied from, 

charlie.charlieWrite(LED_RED, HIGH);
is 
digitalWrite(ED_RED, HIGH);

*/


void setLEDs(byte leds)
{
  charlie.clear();
  if ((leds & CHOICE_RED) != 0)
    charlie.charlieWrite(LED_RED, HIGH);
  else
    charlie.charlieWrite(LED_RED, LOW);

  if ((leds & CHOICE_GREEN) != 0)
    charlie.charlieWrite(LED_GREEN, HIGH);
  else
    charlie.charlieWrite(LED_GREEN, LOW);

  if ((leds & CHOICE_BLUE) != 0)
    charlie.charlieWrite(LED_BLUE, HIGH);
  else
    charlie.charlieWrite(LED_BLUE, LOW);

  if ((leds & CHOICE_YELLOW) != 0)
   charlie.charlieWrite(LED_YELLOW, HIGH);
  else
    charlie.charlieWrite(LED_YELLOW, LOW);
}
/*
//This code will no longer function
void setLEDs(byte leds)
{
  if ((leds & CHOICE_RED) != 0)
    digitalWrite(LED_RED, HIGH);
  else
    digitalWrite(LED_RED, LOW);

  if ((leds & CHOICE_GREEN) != 0)
    digitalWrite(LED_GREEN, HIGH);
  else
    digitalWrite(LED_GREEN, LOW);

  if ((leds & CHOICE_BLUE) != 0)
    digitalWrite(LED_BLUE, HIGH);
  else
    digitalWrite(LED_BLUE, LOW);

  if ((leds & CHOICE_YELLOW) != 0)
    digitalWrite(LED_YELLOW, HIGH);
  else
    digitalWrite(LED_YELLOW, LOW);
}
*/

// Wait for a button to be pressed. 
// Returns one of LED colors (LED_RED, etc.) if successful, 0 if timed out
byte wait_for_button(void)
{
  long startTime = millis(); // Remember the time we started the this loop

  while ( (millis() - startTime) < ENTRY_TIME_LIMIT) // Loop until too much time has passed
  {
    byte button = checkButton();

    if (button != CHOICE_NONE)
    { 
      toner(button, 150); // Play the button the user just pressed

      while(checkButton() != CHOICE_NONE) ;  // Now let's wait for user to release button

      delay(10); // This helps with debouncing and accidental double taps

      return button;
    }

  }

  return CHOICE_NONE; // If we get here, we've timed out!
}

// Returns a '1' bit in the position corresponding to CHOICE_RED, CHOICE_GREEN, etc.
byte checkButton(void)
{
  if (digitalRead(BUTTON_RED) == 0) return(CHOICE_RED); 
  else if (digitalRead(BUTTON_GREEN) == 0) return(CHOICE_GREEN); 
  else if (digitalRead(BUTTON_BLUE) == 0) return(CHOICE_BLUE); 
  else if (digitalRead(BUTTON_YELLOW) == 0) return(CHOICE_YELLOW);

  return(CHOICE_NONE); // If no button is pressed, return none
}

// Light an LED and play tone
// Red, upper left:     440Hz - 2.272ms - 1.136ms pulse
// Green, upper right:  880Hz - 1.136ms - 0.568ms pulse
// Blue, lower left:    587.33Hz - 1.702ms - 0.851ms pulse
// Yellow, lower right: 784Hz - 1.276ms - 0.638ms pulse
void toner(byte which, int buzz_length_ms)
{
  setLEDs(which); //Turn on a given LED

  //Play the sound associated with the given LED
  switch(which) 
  {
  case CHOICE_RED:
    buzz_sound(buzz_length_ms, 1130); 
    break;
  case CHOICE_GREEN:
    buzz_sound(buzz_length_ms, 570); 
    break;
  case CHOICE_BLUE:
    buzz_sound(buzz_length_ms, 850); 
    break;
  case CHOICE_YELLOW:
    buzz_sound(buzz_length_ms, 640); 
    break;
  }

  setLEDs(CHOICE_OFF); // Turn off all LEDs
}

// Toggle buzzer every buzz_delay_us, for a duration of buzz_length_ms.
void buzz_sound(int buzz_length_ms, int buzz_delay_us)
{
  // Convert total play time from milliseconds to microseconds
  long buzz_length_us = buzz_length_ms * (long)1000;

  // Loop until the remaining play time is less than a single buzz_delay_us
  while (buzz_length_us > (buzz_delay_us * 2))
  {
    buzz_length_us -= buzz_delay_us * 2; //Decrease the remaining play time

    // Toggle the buzzer at various speeds
    digitalWrite(BUZZER1, LOW);
    //digitalWrite(BUZZER2, HIGH);
    delayMicroseconds(buzz_delay_us);

    digitalWrite(BUZZER1, HIGH);
    //digitalWrite(BUZZER2, LOW);
    delayMicroseconds(buzz_delay_us);
  }
}

// Play the winner sound and lights
void play_winner(void)
{
  setLEDs(CHOICE_GREEN | CHOICE_BLUE);
  winner_sound();
  setLEDs(CHOICE_RED | CHOICE_YELLOW);
  winner_sound();
  setLEDs(CHOICE_GREEN | CHOICE_BLUE);
  winner_sound();
  setLEDs(CHOICE_RED | CHOICE_YELLOW);
  winner_sound();
}

// Play the winner sound
// This is just a unique (annoying) sound we came up with, there is no magic to it
void winner_sound(void)
{
  // Toggle the buzzer at various speeds
  for (byte x = 250 ; x > 70 ; x--)
  {
    for (byte y = 0 ; y < 3 ; y++)
    {
     // digitalWrite(BUZZER2, HIGH);
      digitalWrite(BUZZER1, LOW);
      delayMicroseconds(x);

      //digitalWrite(BUZZER2, LOW);
      digitalWrite(BUZZER1, HIGH);
      delayMicroseconds(x);
    }
  }
}

// Play the loser sound/lights
void play_loser(void)
{
  setLEDs(CHOICE_RED | CHOICE_GREEN);
  buzz_sound(255, 1500);

  setLEDs(CHOICE_BLUE | CHOICE_YELLOW);
  buzz_sound(255, 1500);

  setLEDs(CHOICE_RED | CHOICE_GREEN);
  buzz_sound(255, 1500);

  setLEDs(CHOICE_BLUE | CHOICE_YELLOW);
  buzz_sound(255, 1500);
}

// Show an "attract mode" display while waiting for user to press button.
void attractMode(void)
{
  while(1) 
  {
    setLEDs(CHOICE_RED);
    delay(100);
    if (checkButton() != CHOICE_NONE) return;

    setLEDs(CHOICE_BLUE);
    delay(100);
    if (checkButton() != CHOICE_NONE) return;

    setLEDs(CHOICE_GREEN);
    delay(100);
    if (checkButton() != CHOICE_NONE) return;

    setLEDs(CHOICE_YELLOW);
    delay(100);
    if (checkButton() != CHOICE_NONE) return;
  }
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// The following functions are related to Beegees Easter Egg only

// Notes in the melody. Each note is about an 1/8th note, "0"s are rests.
int melody[] = {
  NOTE_G4, NOTE_A4, 0, NOTE_C5, 0, 0, NOTE_G4, 0, 0, 0,
  NOTE_E4, 0, NOTE_D4, NOTE_E4, NOTE_G4, 0,
  NOTE_D4, NOTE_E4, 0, NOTE_G4, 0, 0,
  NOTE_D4, 0, NOTE_E4, 0, NOTE_G4, 0, NOTE_A4, 0, NOTE_C5, 0};
int notes[13] = {370, 185, 277, 370, 415, 494, 277, 494, 466, 277, 466, 415, 370};

int Sweater[] = {
  0,0,0, NOTE_B5, NOTE_C5, NOTE_C5, NOTE_B5, NOTE_D5,
  NOTE_C5, NOTE_C5, 0, 0, NOTE_C5, NOTE_C5, NOTE_B5, NOTE_D5,
  NOTE_D5, NOTE_D5, 0, NOTE_C5, NOTE_C5, NOTE_C5, NOTE_B5, NOTE_D5,
  NOTE_C5, NOTE_C5, NOTE_B5, NOTE_D5, NOTE_D5, NOTE_D5, 0, NOTE_B5,
  NOTE_C5, NOTE_C5, NOTE_D5, NOTE_C5, NOTE_C5, NOTE_B5, NOTE_G4, NOTE_B5,
  NOTE_C5, NOTE_B5, NOTE_G4, NOTE_B5, NOTE_B5, NOTE_B5, NOTE_G4, NOTE_G4,
  NOTE_C5, NOTE_C5, NOTE_C5, NOTE_C5, NOTE_C5, NOTE_C5, 0
   
  

};

int axel_f[] = {
  NOTE_F4, NOTE_F4, 0, 0, NOTE_GS4, NOTE_GS4, 0, NOTE_F4, 0, NOTE_F4, NOTE_AS4, 0, NOTE_F4, 0, NOTE_DS4, 0,
  NOTE_F4, NOTE_F4, 0, 0, NOTE_C5, NOTE_C5, 0, NOTE_F4, 0, NOTE_F4, NOTE_CS4, 0, NOTE_C5, 0, NOTE_GS4, 0,
  NOTE_F4, 0, NOTE_C5, 0, NOTE_F5, 0, NOTE_F4, NOTE_DS4, 0, NOTE_DS4, NOTE_C4, 0, NOTE_G4, 0,
  NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
int Pirate[] = {
  NOTE_F4, NOTE_F4, 0, 0, NOTE_GS4, NOTE_GS4, 0, NOTE_F4, 0, NOTE_F4, NOTE_AS4, 0, NOTE_F4, 0, NOTE_DS4, 0,
  NOTE_F4, NOTE_F4, 0, 0, NOTE_C5, NOTE_C5, 0, NOTE_F4, 0, NOTE_F4, NOTE_CS4, 0, NOTE_C5, 0, NOTE_GS4, 0,
  NOTE_F4, 0, NOTE_C5, 0, NOTE_F5, 0, NOTE_F4, NOTE_DS4, 0, NOTE_DS4, NOTE_C4, 0, NOTE_G4, 0,
  NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

int noteDuration = 115; // This essentially sets the tempo, 115 is just about right for a disco groove :)
int LEDnumber = 0; // Keeps track of which LED we are on during the beegees loop

// Do nothing but play bad beegees music
// This function is activated when user holds bottom right button during power up
void play_beegees()
{

  //int notes[13] = {370, 185, 277, 370, 415, 494, 277, 494, 466, 277, 466, 415, 370};
  //Turn on the bottom right (yellow) LED
  setLEDs(CHOICE_YELLOW);
  toner(CHOICE_YELLOW, 150);
  setLEDs(CHOICE_RED | CHOICE_GREEN | CHOICE_BLUE); // Turn on the other LEDs until you release button

  while(checkButton() != CHOICE_NONE) ; // Wait for user to stop pressing button

  setLEDs(CHOICE_NONE); // Turn off LEDs

  delay(1000); // Wait a second before playing song

  digitalWrite(BUZZER1, LOW); // setup the "BUZZER1" side of the buzzer to stay low, while we play the tone on the other pin.
  int haltStong = CHOICE_NONE;
  int pauseBetweenNote=noteDuration;
  while(checkButton() == CHOICE_NONE) //Play song until you press a button
  {
    // iterate over the notes of the melody:
    for (int thisNote = 0; thisNote < 62; thisNote++) {
      changeLED();
       //noTone(BUZZER1);
      if(axel_f[thisNote] == 0) {
        noTone(BUZZER1);
      }else {
        tone(BUZZER1, axel_f[thisNote]);
      }
      // to distinguish the notes, set a minimum time between them.
      // the note's duration + 30% seems to work well:
     


      //tone(BUZZER1, notes[thisNote], 250);
      delay(pauseBetweenNote);
      haltStong = checkButton();
      //delay(300);
      // stop the tone playing:

     
    }
    //noTone(BUZZER1);
  }
}

// Each time this function is called the board moves to the next LED
void changeLED(void)
{
  setLEDs(1 << LEDnumber); // Change the LED

  LEDnumber++; // Goto the next LED
  if(LEDnumber > 3) LEDnumber = 0; // Wrap the counter if needed
}

