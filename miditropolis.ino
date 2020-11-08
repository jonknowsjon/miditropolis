#include <splash.h>
#include <MIDI.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "constants.h"
#include "musicdata.h"
#include "pins.h"

MIDI_CREATE_DEFAULT_INSTANCE();


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



//timing variables
int clk = 0; //counter for midi clock pulses (24 per quarter note)
bool noteOn = false; //flag for whether a note is actually playing
int stepindex = 0; //counter for sequencer steps
int stepLengthIndex = 0; //counter for clock divisions within a seq step (longer notes than others, etc)
bool midiPlaying = false; //flag for whether DAW is currently playing (start signal sent)

//timing constants
const int clkMax=96*4; //max clock value: 4 measures worth... 
             //4 measures allows for breve notes (double whole)

//hacky timing variables -- modify these to tweak the processing lag between clock rcv and note out
bool hasOffset = true;
int clkOffset = (24*4); //processing lag... allow a clock run-up to next measure to sync at beginning


//display & selection variables
int encoderVal, encoderPrevVal; //current + previous values for rotary encoder
bool encoderSwVal; //value for rotary enc's switch


//global settings
int g_scale[12][2] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //placeholder for the scale of notes to be worked within
int clkDivider = 96/16; //  96/n  for nth notes  96*n for superwhole
int g_root = 60; //root note value -- all other note values are offsets of this -- 60 == Middle C

volatile int menuIndex = 0;




//value testing data -- before inputs avail
int in_note[8][2] =    {{0, 1},  //octave, position
                        {0, 2},
                        {0, 3},
                        {0, 4},
                        {0, 5},
                        {0, 6},
                        {0, 7},
                        {1, 1}
                       };
             
int in_velocity[8] = {110,90,80,90, -10,50,50,50}; //velocity value == <0 indicates reset steps (needs testing)
int in_length[8] = {4,8,4,8, 1,1,1,1}; // how many clock divs to count for this step	
int in_duration[8][2] ={{HOLD, 0},  //gatemode, value 0-800ish
						{REPEAT, 50},   //when gate == ONCE, play for single clockdiv
						{REPEAT, 800},   //when gate == REPEAT, repeat for first X number of divs
						{ONCE, 300},   //                     where X = val/(800/lengthcount)+1??
						{HOLD, 800},   //when HOLD = play & sustain for whole step
						{HOLD, 800},
						{HOLD, 0},
						{HOLD, 0}
					   };



//////////////////////  I / O ////////////////////////////
void writeMuxLED(int muxIndex, bool on){
  //set digital pins to mux selector
  for(int i=0; i<4; i++){
    digitalWrite(ledMuxPins[i], MUX_TRUTH_TABLE[muxIndex][i]);
  }
  //write signal
  digitalWrite(ledMuxPins[4], on);
}

int readMuxValue(const unsigned inputMuxPins[], const unsigned signalPin, int muxGroup, int muxIndex){  
  //set digital pins to mux selector
  for(int i=0; i<4; i++){
    digitalWrite(inputMuxPins[i]+(8*muxGroup), //muxGroup 0 or 1, determines whether first or second 8 in 16 channel multiplexer
    MUX_TRUTH_TABLE[muxIndex][i]);
  }
  //read signal
  return analogRead(signalPin);
}
///////////////////////////////////////////////////////////

void pollEncoder(){
  //debouncing variables
  static unsigned long lastChangeTime = 0;  
  unsigned long changeTime;  
  
    encoderSwVal = digitalRead(rotaryEncoderSwPin);  
    encoderVal = digitalRead(rotaryEncoderPin[0]);
    if(encoderVal != encoderPrevVal){
      changeTime = millis();
      if(changeTime - lastChangeTime > 150){ //if last change was less than X ms ago, ignore it... (debouncing)          
        menuNavigate(!digitalRead(rotaryEncoderSwPin),(encoderVal != digitalRead(rotaryEncoderPin[1])));        
        updateDisplay();
        lastChangeTime = changeTime;
      }
    }
    encoderPrevVal = encoderVal;    
  
}

//void encoderISR(void){
//  static unsigned long lastInterruptTime = 0;
//  unsigned long interruptTime = millis();
//  bool swPressed = false;
//  
//  // If interrupts come faster than 5ms, assume it's a bounce and ignore
//  if (interruptTime - lastInterruptTime > 50) {
//    swPressed = digitalRead(rotaryEncoderSwPin);
//    if (digitalRead(rotaryEncoderPin[1]) == LOW)  {
//      menuNavigate(swPressed,false);
//    }
//    else {
//      menuNavigate(swPressed,true); // Could be +5 or +10
//    }  
//  }
//  // Keep track of when we were here last (no more than every 5ms)
//  lastInterruptTime = interruptTime;  
//}

void menuNavigate(bool submenu, bool increment){
  if(!submenu){    
    //navigate in submenu
    if(increment){
      //increment menu selection 
      menuIndex++;
      if(menuIndex > MENU_ITEMCOUNT-1)
        menuIndex = 0;
    }else{
      //decrement menu selection      
      menuIndex--;
      if(menuIndex < 0)
        menuIndex = MENU_ITEMCOUNT-1;
    }    
  }
  
  if(submenu){
    switch(menuIndex){
      case MI_KEY: 
        modifyKey(increment);
        break;
      case MI_SCALE: 
        modifyScale(increment);
        break;
      case  MI_PLAYMODE: 
        modifyPlayMode(increment);
        break;
      case  MI_CLOCKDIV: 
        modifyClockDiv(increment);
        break;
      case  MI_ARPTYPE: 
        modifyArpType(increment);
        break;
      case  MI_CLOCKSRC:
        modifyClockSource(increment);
        break;
    }
  }  
}

void modifyKey(bool increment){
  
}
void modifyScale(bool increment){
  
}
void modifyPlayMode(bool increment){
  
}
void modifyClockDiv(bool increment){
  
}
void modifyArpType(bool increment){
  
}
void modifyClockSource(bool increment){
  
}


void updateDisplay(){
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  //display.println("The value is:");
  display.println(MENU_TEXT[menuIndex]);
  //display.print("time ");
  display.println(millis());
  
  display.display();
}

void pollStep(int s){
  //poll the knob settings for step s and write them to their corresponding array positions
  
  //get positions (0-1023)
  int noteVal = readMuxValue(row12MuxPins, row12SignalPin, 0, s);
  int lengthVal = readMuxValue(row12MuxPins, row12SignalPin, 1, s);
  int durationVal = readMuxValue(row34MuxPins, row34SignalPin, 0, s);
  int velocityVal = readMuxValue(row34MuxPins, row34SignalPin, 1, s);
  
  //perform logic based on position divisions?
  
  
  //assign values to memory 
}

void chordOn(int key, int chord[12], int velocity){
        for (int i=0; i<12; i++){
          if(chord[i]<0)
            break;
           

          MIDI.sendNoteOn(key+chord[i], velocity, 1);
          digitalWrite(clkPin, HIGH);
          
        }
}

void chordOff(int key, int chord[12]){
  
        for (int i=0; i<12; i++){
          if(chord[i]<0)
            break;
          
          MIDI.sendNoteOff(key+chord[i], 0, 1);
          digitalWrite(clkPin,LOW);                                   
        }
}

void stepOn(int root, int scale[12][2], int octaveOffset, int scaleIndex, int velocity){
  
  if(!noteOn){
    chordOn(  root+ (octaveOffset*12) + scale[scaleIndex-1][0], 
              chordFromForm(scale[scaleIndex-1][1]),
              velocity);
    noteOn = true;
  }
}
void stepOff(int root, int scale[12][2], int octaveOffset, int scaleIndex){
  
    chordOff(  root+ (octaveOffset*12) + scale[scaleIndex-1][0], 
              chordFromForm(scale[scaleIndex-1][1]));
    noteOn = false;
}

void setGlobalScale(int newscale[12][2]){
  for (int i=0; i<12; i++){
    g_scale[i][0] = newscale[i][0];
    g_scale[i][1] = newscale[i][1];  
  }  
}

void setup(){
  
  //initialize LED mux pins
  for(int i=0; i<5;i++){
    pinMode(ledMuxPins[i], OUTPUT);    
  }
  //TODO initialize knob input mux pins

  
  
  pinMode(clkPin, OUTPUT);
  pinMode(rotaryEncoderPin[0],INPUT);
  pinMode(rotaryEncoderPin[1],INPUT);
  pinMode(rotaryEncoderSwPin,INPUT_PULLUP);
  
  setGlobalScale(MAJ_CHORD_PROG);
  
  MIDI.begin(MIDI_CHANNEL_OMNI);                      // Launch MIDI and listen to all channels
  MIDI.setHandleStart(handleStart);
  MIDI.setHandleStop(handleStop);
  MIDI.setHandleClock(handleClock);
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000);
  updateDisplay();

  
}

void loop(){
  MIDI.read();  

  pollEncoder();
}

void handleStart(void){
  clk = 0;
  if(hasOffset)
    clk = 0-clkOffset;
  noteOn = false; 
  stepindex = 0;
  stepLengthIndex = 0; 
  midiPlaying = true;
}

void handleStop(void){
  //TODO write handleStop -- when DAW sends stop... housekeeping stuff like turn off LEDs, reset clocks to 0?
  
  midiPlaying = false;
}

//attempt at handling via a measure clock and subdividing from it
void handleClock(void){
  
  if(clk>=0 && (clk%clkDivider==0)){
    //clock is on the trigger pulse for a step      
    
    //at the first step (for all modes)
    bool isFirstStep = (stepLengthIndex == 0);
    //determine if mode is set to repeating
    bool isRepeating = (in_duration[stepindex][0]==REPEAT);
    //determine if step is within repeating mode's pattern length value
    bool isWithinPattern = (stepLengthIndex > 0 && stepLengthIndex < in_duration[stepindex][1]/(DURATION_MAX/in_length[stepindex])+1);
    
    if( isFirstStep || (isRepeating && isWithinPattern)){
      //play the step
      stepOn(g_root, g_scale, in_note[stepindex][0], in_note[stepindex][1], in_velocity[stepindex]);       
    }   
    
    writeMuxLED(stepindex,HIGH);      
  }
  
  if(clk>=0 && ((clk+1)%clkDivider==0) ){    
    //clock is on the pulse before next trigger, time to stop notes (when appropriate) 
    //and increment counters
    
    if(noteOn){
      if( 
          (in_duration[stepindex][0]==HOLD && stepLengthIndex == in_length[stepindex]-1)//HOLD && stepindex == max-1
          || (in_duration[stepindex][0]==ONCE && stepLengthIndex == 0) //ONCE && stepindex == 0
          || (in_duration[stepindex][0]==REPEAT) //REPEAT at every step-end (if note is on)
         ){
            stepOff(g_root, g_scale, in_note[stepindex][0], in_note[stepindex][1]); 
        }
    } 
    
    writeMuxLED(stepindex,LOW);
    
    stepLengthIndex++; 
    if(stepLengthIndex > in_length[stepindex]-1){ //determine if last beat within step
      stepLengthIndex = 0; //if so, reset step length counter
      pollStep(stepindex+1); //read next step's data from knobs
      
      //test to see if next step's velocity < 0 (flag for stop seq);
      if((stepindex < stepMax-1) && (in_velocity[stepindex+1] < 0)){
        stepindex = 0;
      }else{
        stepindex++;
      }
    }         
  }

  //step may have been freshly incremented above, set to 0 if over max
  if(stepindex>stepMax-1){
    stepindex = 0;
  }      
  

  //LED logic: clock pulse on quarter notes (regardless of divider setting)
  if(clk>=0 && (clk%CLK_DIVS[QUARTER]==0)){
      digitalWrite(clkPin,HIGH);
  }
  //LED logic: turn off before next sixteenth
  if(clk>=0 && ((clk+1)%CLK_DIVS[SIXTEENTH]==0) ){
    digitalWrite(clkPin,LOW);
  }  
  

  //increment clock, reset when > max
  clk++;
  if(clk>clkMax-1){
    clk = 0;
  }
}
