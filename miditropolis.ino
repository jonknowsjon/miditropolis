#include <MIDI.h>
#include "forms.h"

// Simple tutorial on how to receive and send MIDI messages.
// Here, when receiving any message on channel 4, the Arduino
// will blink a led and play back a note for 1 second.

MIDI_CREATE_DEFAULT_INSTANCE();

//mux pinout
const bool muxTruthTable[16][4] = { {0,0,0,0},
                                    {1,0,0,0},
                                    {0,1,0,0},
                                    {1,1,0,0},
                                    {0,0,1,0},
                                    {1,0,1,0},
                                    {0,1,1,0},
                                    {1,1,1,0},
                                    {0,0,0,1},
                                    {1,0,0,1},
                                    {0,1,0,1},
                                    {1,1,0,1},
                                    {0,0,1,1},
                                    {1,0,1,1},
                                    {0,1,1,1},
                                    {1,1,1,1},
                                   };

//pin constants
  static const unsigned clkPin = 52;   
  static const unsigned ledMuxPins[5] = {22,23,24,25,53};

//timing variables
int clk = 0; //counter for midi clock pulses (24 per quarter note)
bool noteOn = false; //flag for whether a note is actually playing
int stepindex = 0; //counter for sequencer steps
int stepLengthIndex = 0; //counter for clock divisions within a seq step (longer notes than others, etc)

//timing constants
const int clkMax=96*4; //clock will count every 4 measures (not really a big deal, the bigger, the long divisions possible)

//hacky timing variables -- modify these to tweak the processing lag between clock rcv and note out
bool hasOffset = false;
int clkOffset = (24*4); //processing lag... allow a clock run-up to next measure to sync at beginning




//global settings
int g_scale[12][2] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

int clkDivider = 96/16; //  96/n  for nth notes  96*n for superwhole
int g_root = 60; //read singular root knob/property

int stepMax = 8; //should be done away with after -1 testing?


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
int in_velocity[8] = {110,90,80,90, 50,50,50,50}; //velocity value == <0 indicates reset steps (needs testing)
int in_length[8] = {1,1,1,1, 1,1,1,1}; // how many clock divs to count for this step
int in_duration[8] = {0,0,0,0, 0,0,0,0};  //How long each note lasts -- TBD, may also be a gate mode (whether for longer lengths note repeated, or once)  
                                          //^^^??? First half 1 hit in increments of length setting, second half all hits in increments of length                              



void setup(){
    setGlobalScale(MAJ_CHORD_PROG);
    
    pinMode(ledMuxPins[0], OUTPUT);
    pinMode(ledMuxPins[1], OUTPUT);
    pinMode(ledMuxPins[2], OUTPUT);
    pinMode(ledMuxPins[3], OUTPUT);
    pinMode(ledMuxPins[4], OUTPUT);
    
    MIDI.begin(MIDI_CHANNEL_OMNI);                      // Launch MIDI and listen to all channels
    MIDI.setHandleStart(handleStart);
    MIDI.setHandleClock(handleClock);
}

void loop(){
  MIDI.read();    
}

void handleStart(void){
  clk = 0;
  if(hasOffset)
    clk = 0-clkOffset;
  noteOn = false; 
  stepindex = 0;
  stepLengthIndex = 0; 
}
//TODO write handleStop -- when DAW sends stop... housekeeping stuff like turn off LEDs, reset clocks to 0?


//attempt at handling via a measure clock and subdividing from it
void handleClock(void){    
    
    if(clk>=0 && (clk%clkDivider==0)){
      //clock is on the trigger pulse for a step      
      stepOn(g_root, g_scale, in_note[stepindex][0], in_note[stepindex][1], in_velocity[stepindex]);      
      writeMuxLED(stepindex,HIGH);
      //digitalWrite(clkPin,HIGH);
    }
    
    if(clk>=0 && ((clk+1)%clkDivider==0) ){
      //clock is on the pulse before next trigger, turn off last step's note
      if(noteOn){        
        stepOff(g_root, g_scale, in_note[stepindex][0], in_note[stepindex][1]);        
        //digitalWrite(clkPin,LOW);
        writeMuxLED(stepindex,LOW);
        stepindex++;
      }          
    }
      
    clk++;
    if(clk>clkMax-1){
      clk = 0;
    }
    
    if(stepindex>stepMax-1){
      stepindex = 0;
    }
}


////////////////////////////////////////
//for variable-defined progressions
//min chord prog => int scale[12][2]
//prog => byte octaveOffset, byte scaleIndex

//scale
//octave + scalevalue

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
//////////////////////////////////////




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


void writeMuxLED(int muxIndex, bool on){
  //set digital pins to mux selector
  for(int i=0; i<4; i++){
    digitalWrite(ledMuxPins[i], muxTruthTable[muxIndex][i]);
  }
  //write signal
  digitalWrite(ledMuxPins[4], on);
}

/////UNCOMMENT ME WHEN READY FOR MULTIPLEXING INPUTS
//int readMuxValue(int muxGroup, int muxIndex){
//  //set digital pins to mux selector
//  for(int i=0; i<4; i++){
//    digitalWrite(inputMuxPins[i], muxTruthTable[muxIndex][i]);  //todo: add another dimension to inputMuxPins array... chooses which multiplexer to address for input
//  }
//  //write signal
//  analogRead(inputMuxPins[4], on);
//}


void setGlobalScale(int newscale[12][2]){
  for (int i=0; i<12; i++){
    g_scale[i][0] = newscale[i][0];
    g_scale[i][1] = newscale[i][1];  
  }  
}

///////////////////////////////DEPRECATED OLD STUFF BELOW ///////////////////////////////////


////////////////////////////////////////
////for self contained progressions (hard coded arrays)
//void stepOn(int root, int prog[4][2], int i, int velocity){
//  
//  if(!noteOn){
//    chordOn(  root+(12*prog[i][0]) +MIN_CHORD_PROG[prog[i][1]-1][0], 
//              chordFromForm(MIN_CHORD_PROG[prog[i][1]-1][1]),velocity);
//    noteOn = true;
//  }
//}
//void stepOff(int root, int prog[4][2], int i){
//    chordOff(  root+(12*prog[i][0]) +MIN_CHORD_PROG[prog[i][1]-1][0], 
//              chordFromForm(MIN_CHORD_PROG[prog[i][1]-1][1]));
//    noteOn = false;
//}
///////////////////////////////////////////


//void playProgression(int root, int duration, int rest, int prog[4][2]){ 
//  for(int i=0;i<4;i++){
//    playChord(  root+(12*prog[i][0]) +MIN_CHORD_PROG[prog[i][1]-1][0], 
//              chordFromForm(MIN_CHORD_PROG[prog[i][1]-1][1]), 
//              duration);
//    delay(rest);  
//  }
//  
//}

//void playChord(int root, int chord[3], int duration){
//    
//        if(root > 120)
//          root = 120;
//  
//        MIDI.sendNoteOn(root+chord[0], 90, 1);
//        MIDI.sendNoteOn(root+chord[1], 90, 1);
//        MIDI.sendNoteOn(root+chord[2], 90, 1);
//
//        delay(duration);
//        
//        MIDI.sendNoteOff(root+chord[0], 0, 1);               
//        MIDI.sendNoteOff(root+chord[1], 0, 1);
//        MIDI.sendNoteOff(root+chord[2], 0, 1);  
//      
//}
//
//void playSeq(int root, int chord[3]){
//  digitalWrite(ledPin, HIGH);
//        if(root > 120)
//          root = 120;
//  
//        MIDI.sendNoteOn(root+chord[0], 70, 1);
//        delay(500);               
//        MIDI.sendNoteOff(root+chord[0], 0, 1);     
//        delay(100);
//        digitalWrite(ledPin, LOW);
//
//        MIDI.sendNoteOn(root+chord[1], 90, 1);
//        delay(500);               
//        MIDI.sendNoteOff(root+chord[1], 0, 1);     
//        delay(50);
//
//        MIDI.sendNoteOn(root+chord[2], 110, 1);
//        delay(500);               
//        MIDI.sendNoteOff(root+chord[2], 0, 1);     
//        delay(100);
//
//        MIDI.sendNoteOn(root+chord[1], 90, 1);
//        delay(500);               
//        MIDI.sendNoteOff(root+chord[1], 0, 1);     
//        delay(50);
//      
//}

/* 
C  |  D  |  E  F  |  G  |  A  |  B  C
0  1  2  3  4  5  6  7  8  9  A  B  C
                             10 11 127]
-scales-
Dia Maj 0 2 4 5 7 9 11
Dia Min 0 2 3 5 7 9 11

-chord forms-
Maj Triad 0 4 7 
Min Triad 0 3 7
Sus4 Triad 0 5 7
Sus2 Triad 0 2 7

-intervals-
Maj Third 0 4
Min Third 0 3
Fifth     0 7

Maj Add7 +11
Min Add7 +10
*/
