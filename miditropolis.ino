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



//hacky timing variables -- modify these to tweak the processing lag between clock rcv and note out
bool hasOffset = true;
int clkOffset = (24*4); //processing lag... allow a clock run-up to next measure to sync at beginning


//display & selection variables
int encoderVal, encoderPrevVal; //current + previous values for rotary encoder
bool encoderSwVal; //value for rotary enc's switch


//global settings
int g_scale[12][2] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //placeholder for the scale of notes to be worked within
int g_scale_max = 12;

int clkDivider = 96/16; //  96/n  for nth notes  96*n for superwhole
int g_root = 60; //root note value -- all other note values are offsets of this -- 60 == Middle C

//menu index variables (what the display is  focussed on, and it's current setting's index (for retrieving display text)
volatile int menuIndex = 0;
bool subMenuSelected = false;
int g_scaleIndex = MAJ_DIA;
int g_clockDivIndex = SIXTEENTH;
int g_playModeIndex = CHORD;
int g_clockSrcIndex = CS_EXT;
int g_bpm = 120; 
int g_arpTypeIndex = 0; //unimplemented right now...


//arrays holding step data to be populated from knobs -- initialized with test data
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
int in_duration[8][2] ={{HOLD, 0},    //gatemode, value 0-800ish
						{REPEAT, 50}, //when gate == ONCE, play for single clockdiv
						{REPEAT, 800},//when gate == REPEAT, repeat for first X number of divs
						{ONCE, 300},  //                     where X = val/(800/lengthcount)+1??
						{HOLD, 800},  //when HOLD = play & sustain for whole step
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

  static unsigned long lastSwOnTime = 0;
  unsigned long swOnTime;
  if  (digitalRead(rotaryEncoderSwPin)){
    swOnTime = millis();
    if(swOnTime - lastSwOnTime > 750){
        //button pressed, and its the first time its been pressed in little while
        subMenuSelected = !subMenuSelected; //toggle menu/submenu selection
        lastSwOnTime = swOnTime;
    }
  }
    
    encoderVal = digitalRead(rotaryEncoderPin[0]);
    if(encoderVal != encoderPrevVal){
      changeTime = millis();
      if(changeTime - lastChangeTime > 150){ //if last change was less than X ms ago, ignore it... (debouncing)          
        menuNavigate(subMenuSelected,(encoderVal != digitalRead(rotaryEncoderPin[1])));        
        updateDisplay();
        lastChangeTime = changeTime;
      }
    }
    encoderPrevVal = encoderVal;   
}

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
      case  MI_BPM:
        modifyBPM(increment);
        break;
    }
  }   
}


void modifyKey(bool increment){
		
	if(increment){
		if(g_root < noteMax) //im thinking key should bottom out rather than wrap around
			g_root++;		
	}else{
		if( g_root > noteMin)
			g_root--;
	};
}
void modifyScale(bool increment){
	if(increment){		
		g_scaleIndex++;
		if(g_scaleIndex > SCALE_ITEMCOUNT)
			g_scaleIndex = 0;
	}else{
		g_scaleIndex--;
		if(g_scaleIndex < 0)
			g_scaleIndex = SCALE_ITEMCOUNT-1;
	}
  setScaleFromEnum(g_scaleIndex);
}
void modifyPlayMode(bool increment){
	if(increment){
		g_playModeIndex++;
		if(g_playModeIndex > PLAY_MODES_ITEMCOUNT)
			g_playModeIndex = 0;
	}else{
		g_playModeIndex--;
		if(g_playModeIndex < 0)
			g_playModeIndex = PLAY_MODES_ITEMCOUNT-1;
	}
}
void modifyClockDiv(bool increment){
  if(increment){
		if(g_clockDivIndex < CLOCK_DIVS_ITEMCOUNT-1) //clock div should bottom out rather than wrap around
			g_clockDivIndex++;
	}else{
		if(g_clockDivIndex > 0)
			g_clockDivIndex--;		
	}
}
void modifyArpType(bool increment){
	if(increment){
		g_arpTypeIndex++;
		if(g_arpTypeIndex > ARPTYPE_ITEMCOUNT)
			g_arpTypeIndex = 0;
	}else{
		g_arpTypeIndex--;
		if(g_arpTypeIndex < 0)
			g_arpTypeIndex = ARPTYPE_ITEMCOUNT-1;
	}
}
void modifyClockSource(bool increment){
	if(increment){
		g_clockSrcIndex++;
		if(g_clockSrcIndex >CLKSRC_ITEMCOUNT)
			g_clockSrcIndex = 0;
	}else{
		g_clockSrcIndex--;
		if(g_clockSrcIndex < 0)
			g_clockSrcIndex = CLKSRC_ITEMCOUNT-1;
	}
}
void modifyBPM(bool increment){
  if(increment){
    if(g_bpm < bpmMax)
      g_bpm++;
  }else{
    if(g_bpm > 0)
      g_bpm--;
  }
}

void updateDisplay(){
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  
  if(!subMenuSelected)
	  display.print(">");
  display.println(MENU_TEXT[menuIndex]);
  
  if(subMenuSelected)
	  display.print(">");
  if(menuIndex == MI_KEY){
	  display.print(getNoteLetter(g_root));
	  display.println(getNoteOctave(g_root));
  }else if(menuIndex == MI_BPM){
    display.println(g_bpm);
  }else{
	display.println(getSubMenuText());
  }
  display.display();
}

char* getSubMenuText(){
	switch(menuIndex){
      case MI_SCALE: 
        return SCALE_TEXT[g_scaleIndex];
        break;
      case  MI_PLAYMODE: 
        return PLAY_MODES_TEXT[g_playModeIndex];
        break;
      case  MI_CLOCKDIV: 
        return CLK_DIV_TEXT[g_clockDivIndex];
        break;
      case  MI_ARPTYPE: 
        return ARPTYPE_TEXT[g_arpTypeIndex];
        break;
      case  MI_CLOCKSRC:
        return CLKSRC_TEXT[g_clockSrcIndex];
        break;
    }
}

void setScaleFromEnum(int scale){
  switch(scale){
    case MAJ_DIA:
      setGlobalScale(MAJ_CHORD_PROG);
      break;
    case MIN_DIA:
      setGlobalScale(MIN_CHORD_PROG);
      break;
    case CHROMA:
      setGlobalScale(CHROMATIC_PROG);
      break;
    default:
      setGlobalScale(CHROMATIC_PROG);
      break;
  }  
}

/////////////END MENU & VALUE ASSIGNMENT CODE//////////////////////




void pollStep(int s){
  //poll the knob settings for step s and write them to their corresponding array positions
  
  //get positions (0-1023)
  int noteVal = readMuxValue(row12MuxPins, row12SignalPin, 0, s);
  int lengthVal = readMuxValue(row12MuxPins, row12SignalPin, 1, s);
  int durationVal = readMuxValue(row34MuxPins, row34SignalPin, 0, s);
  int velocityVal = readMuxValue(row34MuxPins, row34SignalPin, 1, s);
  
  //perform logic based on position divisions
  int note_octave = map(noteVal,0,1023,-1,1);
  int note_val =  map(noteVal,0,1023,0,36)%12;
  int dur_mode =  map(durationVal,0,1023,0,3); //split into 2 quarters + half : HOLD, ONCE, REPEAT*2 
  if(dur_mode >= 2) 
     dur_mode = 2; //Q3 and Q4 = REPEAT = 2;  
  int dur_val =  map(durationVal,0,1023,(0-DURATION_MAX),DURATION_MAX); //first half, value is ignored, so begin iteration at 512=0
  
  //assign values to memory 
  in_note[s][0] = note_octave;
  in_note[s][1] = note_val;
  in_length[s] = map(lengthVal,0,1023,1,8);
  in_duration[s][0] = dur_mode; 
  in_duration[s][1] = dur_val;
  in_velocity[s] = map(velocityVal,0,1023,-50,120);
}

void chordOn(int key, int chord[12], int velocity){
  //TODO arpeggiation needs total reworking!
  if(g_playModeIndex == CHORD){
    for (int i=0; i<12; i++){
      if(chord[i]<0)
        break;
      MIDI.sendNoteOn(key+chord[i], velocity, 1);
    }
  }
  if(g_playModeIndex == SINGLE){
    if(chord[0]>=0)
      MIDI.sendNoteOn(key+chord[0],velocity,1);
  }
}

void chordOff(int key, int chord[12]){
  if(g_playModeIndex == CHORD){
    for (int i=0; i<12; i++){
      if(chord[i]<0)
        break;          
      MIDI.sendNoteOff(key+chord[i], 0, 1);                                       
    }
  }
  if(g_playModeIndex == SINGLE){
    if(chord[0]>=0)
      MIDI.sendNoteOff(key+chord[0],0,1);
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
    
    if(g_scale[i][1] != UNDEF) //determine highest index with definition (used for segmenting note knob)
      g_scale_max = i;
  }  
}

void setup(){
  
  //initialize LED mux pins
  for(int i=0; i<5;i++){
    pinMode(ledMuxPins[i], OUTPUT);    
  }
  
  //initialize knob input mux pins
  for(int i=0; i<4;i++){
    pinMode(row12MuxPins[i],OUTPUT);
    pinMode(row34MuxPins[i],OUTPUT);
  }
  pinMode(row12SignalPin, INPUT);
  pinMode(row34SignalPin, INPUT);  
  
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

void internalClockTick(){
  static unsigned long lastPulseMillis = 0;    
  unsigned currentMillis = millis();    
  
  //determine milli interval between pulses for current bpm  
  int clkInterval = (g_bpm*(1000/60))/24;  //24 pulses per quarter... 60bpm = 1 beat per 1000ms =  (bpm*(1000/60))/24 = ms-per-clk
  
  //see if internal clock toggle is switched on
  if(digitalRead(extClockTogglePin)){
    //test to see if that interval has passed
    if(currentMillis-lastPulseMillis >= clkInterval){
      //if so, call handleclock
      handleClock();
      //mark pulse time
      lastPulseMillis = currentMillis;
    }
  }
}

// the main running loop,  should put as few things here as possible to reduce latency
void loop(){
 
  if(g_clockSrcIndex == CS_EXT)
    MIDI.read();
  
  if(g_clockSrcIndex == CS_INT)  
    internalClockTick();
    
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
