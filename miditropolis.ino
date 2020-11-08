#include <MIDI.h>
#include "constants.h"
#include "musicdata.h"


MIDI_CREATE_DEFAULT_INSTANCE();

//pin constants
static const unsigned clkPin = 52; //pin for tempo LED, also used for debugging loops
static const unsigned ledMuxPins[5] = {22,23,24,25,53}; //4 pins for interfacing multiplexer + 1 signal pin
static const unsigned row12MuxPins[4] = {26,27,28,29}; //pins for multiplexer for rows 1 and 2    
static const unsigned row34MuxPins[4] = {26,27,28,29}; //pins for multiplexer for rows 3 and 4
static const unsigned row12SignalPin = 8;
static const unsigned row34SignalPin = 9;


//timing variables
int clk = 0; //counter for midi clock pulses (24 per quarter note)
bool noteOn = false; //flag for whether a note is actually playing
int stepindex = 0; //counter for sequencer steps
int stepLengthIndex = 0; //counter for clock divisions within a seq step (longer notes than others, etc)
boolean midiPlaying = false; //flag for whether DAW is currently playing (start signal sent)

//timing constants
const int clkMax=96*4; //max clock value: 4 measures worth... 
					   //4 measures allows for breve notes (double whole)

//hacky timing variables -- modify these to tweak the processing lag between clock rcv and note out
bool hasOffset = false;
int clkOffset = (24*4); //processing lag... allow a clock run-up to next measure to sync at beginning



//global settings
int g_scale[12][2] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //placeholder for the scale of notes to be worked within
int clkDivider = 96/16; //  96/n  for nth notes  96*n for superwhole
int g_root = 60; //root note value -- all other note values are offsets of this -- 60 == Middle C

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
	
    //initialize LED mux pins
	for(int i=0; i<5;i++){
		pinMode(ledMuxPins[i], OUTPUT);    
	}
	pinMode(clkPin, OUTPUT);	
	
    setGlobalScale(MAJ_CHORD_PROG);
	
	MIDI.begin(MIDI_CHANNEL_OMNI);                      // Launch MIDI and listen to all channels
    MIDI.setHandleStart(handleStart);
	MIDI.setHandleStop(handleStop);
    MIDI.setHandleClock(handleClock);
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
        
		
		//TODO increment steplengthindex
		//test to see if steplength > lengthVal-1
		//if so - reset steplengthindex, poll next step, increment stepindex
		pollStep(stepindex+1);
		stepindex++;
      }          
    }      
    
	clk++;
    if(clk>clkMax-1){
      clk = 0;
    }
    
	
	//TODO add condition --  || in_velocity[stepindex] < 0
    if(stepindex>stepMax-1){
      stepindex = 0;
    }
}

void pollEncoder(void){
	//read state information for the encoder, if changed, handle the change, display

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

//////////////////////  I / O ////////////////////////////
void writeMuxLED(int muxIndex, bool on){
  //set digital pins to mux selector
  for(int i=0; i<4; i++){
    digitalWrite(ledMuxPins[i], MUX_TRUTH_TABLE[muxIndex][i]);
  }
  //write signal
  digitalWrite(ledMuxPins[4], on);
}

int readMuxValue(unsigned inputMuxPins[], unsigned signalPin, int muxGroup, int muxIndex){  
  //set digital pins to mux selector
  for(int i=0; i<4; i++){
    digitalWrite(inputMuxPins[i]+(8*muxGroup), //muxGroup 0 or 1, determines whether first or second 8 in 16 channel multiplexer
		MUX_TRUTH_TABLE[muxIndex][i]);
  }
  //read signal
  return analogRead(signalPin);
}
///////////////////////////////////////////////////////////


void setGlobalScale(int newscale[12][2]){
  for (int i=0; i<12; i++){
    g_scale[i][0] = newscale[i][0];
    g_scale[i][1] = newscale[i][1];  
  }  
}