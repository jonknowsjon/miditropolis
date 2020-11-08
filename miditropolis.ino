#include <MIDI.h>
#include "forms.h"

// Simple tutorial on how to receive and send MIDI messages.
// Here, when receiving any message on channel 4, the Arduino
// will blink a led and play back a note for 1 second.

MIDI_CREATE_DEFAULT_INSTANCE();

static const unsigned ledPin = 53;      // LED pin on Arduino Uno

static const int SCALE_MAJ_DIA[7] = {0,2,4,5,7,9,11};
static const int SCALE_MIN_DIA[7] = {0,2,3,5,7,9,10};

static const int TRIAD_MAJ[3] = {0,4,7}; //0
static const int TRIAD_MIN[3] = {0,3,7}; //1
static const int TRIAD_DIM[3] = {0,3,6}; //2
static const int TRIAD_SUS2[3] = {0,2,7}; //3
static const int TRIAD_SUS4[3] = {0,5,7}; //4

static const int INTERVAL_MAJ_THIRD[3] = {0,4};
static const int INTERVAL_MIN_THIRD[3] = {0,3};
static const int INTERVAL_FIFTH[3] = {0,7};

static const int MAJ_CHORD_PROG[7][2] = { {0,MAJ},
                                          {2,MIN},
                                          {4,MIN},
                                          {5,MAJ},
                                          {7,MAJ},
                                          {9,MIN},
                                          {11,DIM} };

                                          
static const int MIN_CHORD_PROG[7][2] = { {0,MIN},
                                          {2,DIM},
                                          {3,MAJ},
                                          {5,MIN},
                                          {7,MIN},
                                          {8,MAJ},
                                          {10,MAJ} };

int clk = 0;
int clkOffset = (24*4); //processing lag... allow a clock run-up to next measure to sync at beginning
bool hasOffset = false;

int clkMax=96*4;
int clkDivider = 96/16; //  96/n  for nth notes  96*n for superwhole


int stepindex = 0;
int stepMax = 4;

bool noteOn = false;
int progroot = 62;
int progression[4][2] = {   {0,1},
                              {-1,7},
                              {-1,5},
                              {-1,6}
                              };
  


void setup()
{
    pinMode(ledPin, OUTPUT);
    MIDI.begin(MIDI_CHANNEL_OMNI);                      // Launch MIDI and listen to all channels
    MIDI.setHandleStart(handleStart);
    MIDI.setHandleClock(handleClock);
}

void loop()
{
  MIDI.read();    
}

void handleStart(void){
  clk = 0;
  if(hasOffset)
    clk = 0-clkOffset;
  noteOn = false;  
}

//attempt at handling via a measure clock and subdividing from it
void handleClock(void){    
    
    if(clk>=0 && (clk%clkDivider==0)){
      //clock is on the trigger pulse for a step 
      stepOn(progroot, progression, stepindex);
    }
    
    if(clk>=0 && ((clk+1)%clkDivider==0) ){
      //clock is on the pulse before next trigger, turn off last step's note
      if(noteOn){
        stepOff(progroot, progression, stepindex);
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



void stepOn(int root, int prog[4][2], int i){
  
  if(!noteOn){
    chordOn(  root+(12*prog[i][0]) +MIN_CHORD_PROG[prog[i][1]-1][0], 
              chordFromForm(MIN_CHORD_PROG[prog[i][1]-1][1]));
    noteOn = true;
  }
}

void stepOff(int root, int prog[4][2], int i){
    chordOff(  root+(12*prog[i][0]) +MIN_CHORD_PROG[prog[i][1]-1][0], 
              chordFromForm(MIN_CHORD_PROG[prog[i][1]-1][1]));
    noteOn = false;
    
    
}


void playProgression(int root, int duration, int rest, int prog[4][2]){ 
  for(int i=0;i<4;i++){
    playChord(  root+(12*prog[i][0]) +MIN_CHORD_PROG[prog[i][1]-1][0], 
              chordFromForm(MIN_CHORD_PROG[prog[i][1]-1][1]), 
              duration);
    delay(rest);  
  }
  
}


int * chordFromForm(int form){
    switch(form){
      case MAJ:
        return TRIAD_MAJ;
        break;
      case MIN:
        return TRIAD_MIN;
        break;
      case DIM:
        return TRIAD_DIM;
        break;
      case SUS2:
        return TRIAD_SUS2;
        break;
      case SUS4:
        return TRIAD_SUS4;
        break;
      default:
        return TRIAD_MAJ;
    }
 //maj min dim sus2 sus4  
}


void chordOn(int root, int chord[3]){
        if(root > 120)
          root = 120;
  
        MIDI.sendNoteOn(root+chord[0], 90, 1);
        MIDI.sendNoteOn(root+chord[1], 90, 1);
        MIDI.sendNoteOn(root+chord[2], 90, 1);

}
void chordOff(int root, int chord[3]){
        if(root > 120)
          root = 120;
  
        MIDI.sendNoteOff(root+chord[0], 0, 1);               
        MIDI.sendNoteOff(root+chord[1], 0, 1);
        MIDI.sendNoteOff(root+chord[2], 0, 1);  
  
}

void playChord(int root, int chord[3], int duration){
    
        if(root > 120)
          root = 120;
  
        MIDI.sendNoteOn(root+chord[0], 90, 1);
        MIDI.sendNoteOn(root+chord[1], 90, 1);
        MIDI.sendNoteOn(root+chord[2], 90, 1);

        delay(duration);
        
        MIDI.sendNoteOff(root+chord[0], 0, 1);               
        MIDI.sendNoteOff(root+chord[1], 0, 1);
        MIDI.sendNoteOff(root+chord[2], 0, 1);  
      
}

void playSeq(int root, int chord[3]){
  digitalWrite(ledPin, HIGH);
        if(root > 120)
          root = 120;
  
        MIDI.sendNoteOn(root+chord[0], 70, 1);
        delay(500);               
        MIDI.sendNoteOff(root+chord[0], 0, 1);     
        delay(100);
        digitalWrite(ledPin, LOW);

        MIDI.sendNoteOn(root+chord[1], 90, 1);
        delay(500);               
        MIDI.sendNoteOff(root+chord[1], 0, 1);     
        delay(50);

        MIDI.sendNoteOn(root+chord[2], 110, 1);
        delay(500);               
        MIDI.sendNoteOff(root+chord[2], 0, 1);     
        delay(100);

        MIDI.sendNoteOn(root+chord[1], 90, 1);
        delay(500);               
        MIDI.sendNoteOff(root+chord[1], 0, 1);     
        delay(50);
      
}

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
