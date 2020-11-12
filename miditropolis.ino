/***************************************************************************************************************************************************
 * MIDIBOX
 * An Arduino-driven midi sequencer, 
 * inspired by the Intellijel Metropolis Eurorack Module, and transitively, ryk's Roland 185 sequencer
 * 
 * 
 * 
 *  
 * 
 * Coding TODO:  Features still needed:
 * HandleStop
 * Arpeggiation
 * Step hold-- switch,  while note on, sustains current note, while off, does not?  (resume quantized?) 
 * More Scales / Modes  -- 
 * PolyMode - Currently Poly is Chord Triad only, different intervals,  thirds, fifths, etc?
 * Settable MIDI channels (listen, output)
 * Harmony Midi - for intervals/triads, output on different midi channels for poly using multi instruments?
 * 
 * Needs Testing:
 * 
 * 
 * Bugs:
 * Staccato overrides sustained note mode
 * Cycling through menu messes up notes
 * 
 * *************************************************************************************************************************************************
 */

#include "splash.h"
#include <MIDI.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/TomThumb.h>
#include "constants.h"
#include "musicdata.h"
#include "pins.h"

MIDI_CREATE_INSTANCE(HardwareSerial, Serial3, MIDI);  //Serial3 uses pins 14 & 15,  default instance used Serial0, created a conflict with debug logging

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


/************************************************************************************************************************************
 *                         BEGIN VARIABLE DECLARATION
 ************************************************************************************************************************************/

//timing variables
int clk = 0; //counter for midi clock pulses (24 per quarter note)
int clkDivider = CLK_DIVS[EIGHTH]; //  96/n  for nth notes  96*n for superwhole
int g_clkOffset = 0;//(24*4); //processing lag... allow a clock run-up to next measure to sync at beginning
bool noteOn = false; //flag for whether a note is actually playing
int stepindex = 0; //counter for sequencer steps
int nextStepIndex = 0; //placeholder for next step in sequence (for non-standard sequence orders)
int stepLengthIndex = 0; //counter for clock divisions within a seq step (longer notes than others, etc)
bool midiPlaying = false; //flag for whether DAW is currently playing (start signal sent)
bool flushingReads = false; //flag for whether MIDI read is being flushed
bool prevToggleVal = false;

//display & selection variables
int encoderVal, encoderPrevVal; //current + previous values for rotary encoder
bool encoderSwVal; //value for rotary enc's switch

//global settings
int g_key = 60; //root note value -- all other note values are offsets of this -- 60 == Middle C
int g_scale[12][2] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //placeholder for the scale of notes to be worked within
int g_scale_max = 12; //number of notes in scale

//menu index variables (what the display is  focussed on, and it's current setting's index (for retrieving display text)
int menuIndex = MI_INFO;
bool subMenuSelected = false;
int g_scaleIndex = MAJ_DIA;
int g_clockDivIndex = EIGHTH;
int g_clockSrcIndex = CS_EXT;
int g_bpm = 120; //bpm value for internal clock 
int g_playModeIndex = CHORD;
int g_arpTypeIndex = 0; //index for which style of arpeggiation to use (when selected as playmode)
int g_infoModeIndex = SEQINFO;
int g_seqOrderIndex = FORWARD;
int g_seqOrderPing = 1; //when seqorder is set to ping-pong, this will keep track of whether we're pinging or ponging
int g_staccato = 0;

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
int in_velocity[8] = {110,90,80,90, 90,50,50,50}; //velocity value == <0 indicates reset steps (needs testing)
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

//raw knob values for debugging / component testing
int raw_note[8] = {0,0,0,0,0,0,0,0};
int raw_length[8] = {0,0,0,0,0,0,0,0};
int raw_duration[8] = {0,0,0,0,0,0,0,0};
int raw_velocity[8] = {0,0,0,0,0,0,0,0};
///////////////////////////END VARIABLE DECLARATION////////////////////////////////////////////////////////////////////////////////////


/************************************************************************************************************************************
 *                         BEGIN PHYSICAL  I / O CODE
 ************************************************************************************************************************************/
void writeMuxLED(int muxIndex, bool on){
  //set digital pins to mux selector
  for(int i=0; i<4; i++){
    digitalWrite(ledMuxPins[i], MUX_TRUTH_TABLE[muxIndex][i]);
  }
  //write signal
  digitalWrite(ledMuxPins[4], on);
}

int readMuxValue(const unsigned inputMuxPins[], const unsigned signalPin, int muxGroup, int muxIndex){  
  //Serial.println(muxIndex+(8*muxGroup));

  //set digital pins to mux selector
  for(int i=0; i<4; i++){
    digitalWrite(inputMuxPins[i], //muxGroup 0 or 1, determines whether first or second 8 in 16 channel multiplexer
    MUX_TRUTH_TABLE[muxIndex+(8*muxGroup)][i]);
  }
  //read signal
  return analogRead(signalPin);
}

void pollAllSteps(int n){
  //called when sequencer has nothing better to do
  //every n millis, will poll the knobs and update the display
  
  static unsigned long lastDisplayMillis = 0;    
  unsigned currentMillis = millis();

  //check to see if info menu is selected and if refresh time is elapsed
  if(menuIndex == MI_INFO && currentMillis-lastDisplayMillis >= n){      
    pollAllSteps();
    
    updateDisplay();
    //mark the time
    lastDisplayMillis = currentMillis;
  }
}

void pollAllSteps(){
    for(int i=0;i<stepMax;i++){
      pollStep(i);
    }
}

void pollStep(int s){
  //poll the knob settings for step s and write them to their corresponding array positions

  //Serial.print("Polling step: ");
  //Serial.println(s);
 
  //get positions (0-1023)
  int noteVal = readMuxValue(row12MuxPins, row12SignalPin, 0, s);
  int lengthVal = readMuxValue(row12MuxPins, row12SignalPin, 1, s);
  int durationVal = readMuxValue(row34MuxPins, row34SignalPin, 0, s);
  int velocityVal = readMuxValue(row34MuxPins, row34SignalPin, 1, s);

  //save off raw data for debugging/display
  raw_note[s] = noteVal;
  raw_length[s] = lengthVal;
  raw_duration[s] = durationVal;
  raw_velocity[s] = velocityVal;
  
  //perform logic based on position divisions
  int note_octave = map(noteVal,0,1023,-1,2);
  int note_val =  map(noteVal,0,1023,0,3*(g_scale_max+1))%(g_scale_max+1);
  
  int dur_mode =  map(durationVal,0,1023,0,4); //split into 2 quarters + half : HOLD, ONCE, REPEAT*2 
  if(dur_mode >= 2) 
     dur_mode = 2; //Q3 and Q4 = REPEAT = 2;  
  int dur_val =  map(durationVal,0,1023,(0-DURATION_MAX),DURATION_MAX); //first half, value is ignored, so begin iteration at 512=0

  //these mappings when using proper maxes barely reach their upper bound as far as a useable setting (i.e. of 0-1023 :: 1-8, only 1023 :: 8);
  //so going to use slightly larger maxes and round down when necessary below
  int len_val = map(lengthVal,0,1023,1,9);
  if(len_val > 8){
    len_val = 8;
  }
  int vel_val = map(velocityVal,0,1023,-50,121);
  if(vel_val > 120){
    vel_val = 120;
  }
  //provide a slightly larger "dead spot" for 0 velocity but not in the skip range 
  if(vel_val > -12 && vel_val < 0){
    vel_val = 0;
  }  


  //Serial.print("Step ");
  //Serial.println(s);
  
  //test to see if potentiometer readings are enabled, if so, assign its mapped value to memory
  //Serial.print(" note ");
  //Serial.print(POT_ENABLED[0][s]);
  if(POT_ENABLED[0][s]){
    in_note[s][0] = note_octave;
    in_note[s][1] = note_val;
  }else{
    in_note[s][0] = DEFAULT_OCTAVE;
    in_note[s][1] = DEFAULT_NOTE;
  }
  //Serial.print(" len ");
  //Serial.print(POT_ENABLED[1][s]);
  if(POT_ENABLED[1][s]){
    in_length[s] = len_val;
  }else{
    in_length[s] = DEFAULT_LENGTH;  
  }
  //Serial.print(" dur ");
  //Serial.print(POT_ENABLED[2][s]);
  if(POT_ENABLED[2][s]){
    in_duration[s][0] = dur_mode; 
    in_duration[s][1] = dur_val;
  }else{
    in_duration[s][0] = DEFAULT_DUR_MODE;
    in_duration[s][1] = DEFAULT_DURATION;
  }
  //Serial.print(" vel ");
  //Serial.println(POT_ENABLED[3][s]);
  if(POT_ENABLED[3][s]){
    in_velocity[s] = vel_val;  
  }else{
    in_velocity[s] = DEFAULT_VELOCITY;
  }  
}



////////////////////////////END PHYSICAL I / O CODE////////////////////////////////////////////////////////////////////////////////////


/************************************************************************************************************************************
 *                         BEGIN  MENU & VALUE ASSIGNMENTS
 ************************************************************************************************************************************/
void pollEncoder(){
  //debouncing variables
  static unsigned long lastChangeTime = 0;  
  unsigned long changeTime;

  static unsigned long lastSwOnTime = 0;
  unsigned long swOnTime;

  //these probably shouldnt go in this function, but this gets polled frequently
  if(!midiPlaying && digitalRead(extClockTogglePin) == LOW){
    //switch flipped on detected -- throw a handlestart
    handleStart();
  }
  if(midiPlaying && digitalRead(extClockTogglePin) == HIGH){
    //switch flipped off detected -- throw a handleStop
    handleStop();
  }
  

  if  (!digitalRead(rotaryEncoderSwPin)){
    swOnTime = millis();
    if(swOnTime - lastSwOnTime > 750){
        //button pressed, and its the first time its been pressed in little while
        subMenuSelected = !subMenuSelected; //toggle menu/submenu selection
        updateDisplay();
        lastSwOnTime = swOnTime;
    }
  }
    
    encoderVal = digitalRead(rotaryEncoderPin[0]);
    if(encoderVal != encoderPrevVal){
      changeTime = millis();
      if(changeTime - lastChangeTime > 150){ //if last change was less than X ms ago, ignore it... (debouncing)  

        bool increment = (encoderVal != digitalRead(rotaryEncoderPin[1]));
                       
        menuNavigate(subMenuSelected, increment);        
        updateDisplay();
        lastChangeTime = changeTime;
      }
    }
    encoderPrevVal = encoderVal;   
}

void menuNavigate(bool submenu, bool increment){  
  //Serial.print(submenu);
  //Serial.println(increment);
//  memoryLog();
  
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
    //Serial.print("Submenu ");
    //Serial.println(menuIndex);
    
    if(menuIndex == MI_SCALE){
        modifyScale(increment);
    }
    if(menuIndex == MI_PLAYMODE){
        modifyPlayMode(increment);
    }
    if(menuIndex == MI_CLOCKDIV){
        modifyClockDiv(increment);
    }
    if(menuIndex == MI_ARPTYPE){
      modifyArpType(increment);   
    }
    if(menuIndex == MI_CLOCKSRC){
        modifyClockSource(increment);
    }
    if(menuIndex == MI_BPM){
        modifyBPM(increment);
    }
    if(menuIndex == MI_KEY){
        modifyKey(increment);
    }
    if(menuIndex == MI_INFO){
        modifyInfoMode(increment);
    }
    if(menuIndex == MI_CLKOFFSET){
        modifyClockOffset(increment);
    }
    if(menuIndex == MI_STACC){
        modifyStaccato(increment);
    }
    if(menuIndex == MI_SEQORDER){
        modifySeqOrder(increment);
    }      
  }   
}

void modifyScale(bool increment){
	Serial.print("scale ");
  Serial.println(increment);
	if(increment){		
		g_scaleIndex++;
		if(g_scaleIndex > SCALE_ITEMCOUNT-1)
			g_scaleIndex = 0;
	}else{
		g_scaleIndex--;
		if(g_scaleIndex < 0)
			g_scaleIndex = SCALE_ITEMCOUNT-1;
	}
  //possibly hacky... call midi panic and end all playing notes
  //if sequence is running, the note-end event may never be called if old note is not in new scale
  panic();
  setScaleFromEnum(g_scaleIndex);
}

void modifyPlayMode(bool increment){
	Serial.print("pm ");
  Serial.println(increment);
	if(increment){
		g_playModeIndex++;
		if(g_playModeIndex > PLAY_MODES_ITEMCOUNT-1)
			g_playModeIndex = 0;
	}else{
		g_playModeIndex--;
		if(g_playModeIndex < 0)
			g_playModeIndex = PLAY_MODES_ITEMCOUNT-1;
	}
}

void modifyClockDiv(bool increment){
  Serial.print("clkdiv ");
  Serial.println(increment);
  if(increment){
		if(g_clockDivIndex < CLOCK_DIVS_ITEMCOUNT-1) //clock div should bottom out rather than wrap around
			g_clockDivIndex++;
	}else{
		if(g_clockDivIndex > 0)
			g_clockDivIndex--;		
	}
 clkDivider = CLK_DIVS[g_clockDivIndex];
}

void modifyArpType(bool increment){
	Serial.print("arpty ");
  Serial.println(increment);
	if(increment){
		g_arpTypeIndex++;
		if(g_arpTypeIndex > ARPTYPE_ITEMCOUNT-1)
			g_arpTypeIndex = 0;
	}else{
		g_arpTypeIndex--;
		if(g_arpTypeIndex < 0)
			g_arpTypeIndex = ARPTYPE_ITEMCOUNT-1;
	}
}

void modifyClockSource(bool increment){
	Serial.print("clksc ");
  Serial.println(increment);
	if(increment){
		g_clockSrcIndex++;
		if(g_clockSrcIndex >CLKSRC_ITEMCOUNT-1)
			g_clockSrcIndex = 0;
	}else{
		g_clockSrcIndex--;
		if(g_clockSrcIndex < 0)
			g_clockSrcIndex = CLKSRC_ITEMCOUNT-1;
	}
}

void modifyInfoMode(bool increment){
  Serial.print("infomd ");
  Serial.println(increment);
  if(increment){
    g_infoModeIndex++;
    if(g_infoModeIndex >INFO_MODES_ITEMCOUNT-1)
      g_infoModeIndex = 0;
  }else{
    g_infoModeIndex--;
    if(g_infoModeIndex < 0)
      g_infoModeIndex = INFO_MODES_ITEMCOUNT-1;
  }
}

void modifySeqOrder(bool increment){
  Serial.print("seqorder ");
  Serial.println(increment);
  if(increment){
    g_seqOrderIndex++;
    if(g_seqOrderIndex > SEQ_ORDERS_ITEMCOUNT-1)
      g_seqOrderIndex = 0;
  }else{
    g_seqOrderIndex--;
    if(g_seqOrderIndex < 0)
      g_seqOrderIndex = SEQ_ORDERS_ITEMCOUNT-1;
  }
}

void modifyClockOffset(bool increment){
  Serial.print("clkoff ");
  Serial.println(increment);
  if(increment){
    if(g_clkOffset < offsetMax)
      g_clkOffset += offsetStep;
  }else{
    if(g_clkOffset > offsetMin)
      g_clkOffset -= offsetStep;    
  }
}

void modifyStaccato(bool increment){
  Serial.print("stacc ");
  Serial.println(increment);
  if(increment){
    if(g_staccato < staccMax)
      g_staccato++;
  }else{
    if(g_staccato > staccMin)
      g_staccato--;
  }
}

void modifyBPM(bool increment){
  Serial.print("bpm ");
  Serial.println(increment);
  if(increment){
    if(g_bpm < bpmMax)
      g_bpm++;
  }else{
    if(g_bpm > bpmMin)
      g_bpm--;
  }
}

void modifyKey(bool increment){
  Serial.print("ky ");
  Serial.println(increment);
    if(increment){
      if(g_key < noteMax)
        g_key++;
    }else{
      if(g_key > noteMin)
        g_key--;
    }
}

void setGlobalScale(int newscale[12][2]){
  for (int i=0; i<12; i++){

    g_scale[i][0] = newscale[i][0];
    g_scale[i][1] = newscale[i][1]; 
    
    if(g_scale[i][1] != UNDEF) //determine highest index with definition (used for segmenting note knob)
      g_scale_max = i;
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
	case ION:
	  setGlobalScale(IONIAN_MODE_PROG);
	  break;
	case DOR:
	  setGlobalScale(DORIAN_MODE_PROG);
	  break;
	case PHRY:
	  setGlobalScale(PHRYGIAN_MODE_PROG);
	  break;
	case LYD:
	  setGlobalScale(LYDIAN_MODE_PROG);
	  break;
	case MIXO:
	  setGlobalScale(MIXOLYDIAN_MODE_PROG);
	  break;
	case AEOL:
	  setGlobalScale(AEOLIAN_MODE_PROG);
	  break;
	case LOCR:
	  setGlobalScale(LOCRIAN_MODE_PROG);
	  break;
    default:
      setGlobalScale(CHROMATIC_PROG);
      break;
  }  
}
////////////////////////////END MENU & VALUE ASSIGNMENT CODE//////////////////////////////////////////////////////////////////////////


/************************************************************************************************************************************
 *                         BEGIN  DISPLAY  & SERIAL FUNCTIONS 
 ************************************************************************************************************************************/
void updateDisplay(){
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(2);
  display.setFont(NULL);
  display.setTextColor(SSD1306_WHITE);

  if(menuIndex != MI_DEBUG){  
    if(!subMenuSelected)
      display.print(">");
    else
      display.print(" ");  
      
    display.println(MENU_TEXT[menuIndex]);

    
    if(subMenuSelected)
      display.print(">");
    else
      display.print(" ");
    
    displaySubMenuValue();
    
    if(menuIndex == MI_INFO){
      displayInfo();  
    }
  }else{
    displayDebug();  
  }
  displaySubMenuDescription();
  
  display.display();
}

void displaySubMenuValue(){
  if(menuIndex == MI_KEY){
    display.print(getNoteLetter(g_key));    
    display.println(getNoteOctave(g_key));    
  }else if(menuIndex == MI_BPM){
    display.println(g_bpm);
  }else if(menuIndex == MI_STACC){
    display.print(g_staccato);
    display.print(" max ");
    display.println(clkDivider-1);
  }else if(menuIndex == MI_CLKOFFSET){
    display.println(g_clkOffset);
  }else{
    display.println(getSubMenuText());
  }
}

void displaySubMenuDescription(){   
  //maybe this isnt needed, but if there's space, provide a little text to describe some opaque menu items??
}
void displayInfo(){
  display.setTextSize(1);
  if(g_infoModeIndex == ABOUT){
                   //123456789012345678901
    display.println(" >> miditropolis << ");
    display.println("a complex sequencer");
    display.println("crapped out by jonno");
    display.println("      howdy.        ");
  }
  if(g_infoModeIndex == SEQINFO){
    //current position line
    //begin by displaying sequence length
    if(getPatternLength()%4 == 0)//highlight text if pattern length is a multiple of 4 (means pattern will gel well in 4/4)
      display.setTextColor(BLACK,WHITE);
          
    if(getPatternLength() < 10)
      display.print(" ");//pad pattern length when one digit
    display.print(getPatternLength());
    //return to regular color
    display.setTextColor(WHITE,BLACK);
    display.print("");
    for(int i=0;i<8;i++){
      displaySeqPos(i);
      display.print(" ");
    }
    display.println("");
    
    //step length line
    display.print("  ");    
    for(int i=0;i<8;i++){
      displaySeqInfo1(i);  
      if(i<7)
        display.print(" ");
    }
    display.println("");

    //current counter line
    display.print("  ");
    for(int i=0;i<8;i++){
      displaySeqInfo2(i);
      if(i<7)
      display.print(" ");
    }
    display.println("");    
  }
  if(g_infoModeIndex == STEPVALUE){
    displayStepValue(stepindex);   
  }
  if(g_infoModeIndex == STP1){
    pollStep(0);
    displayStepValue(0);
  }
  if(g_infoModeIndex == STP2){
    pollStep(1);
    displayStepValue(1);
  }
  if(g_infoModeIndex == STP3){
    pollStep(2);
    displayStepValue(2);
  }
  if(g_infoModeIndex == STP4){
    pollStep(3);
    displayStepValue(3);
  }
  if(g_infoModeIndex == STP5){
    pollStep(4);
    displayStepValue(4);
  }
  if(g_infoModeIndex == STP6){
    pollStep(5);
    displayStepValue(5);
  }
  if(g_infoModeIndex == STP7){
    pollStep(6);
    displayStepValue(6);
  }
  if(g_infoModeIndex == STP8){
    pollStep(7);
    displayStepValue(7);
  }
}

void displaySeqInfo1(int s){
  //first line -- displays settings (trying out length)
  display.print(in_length[s]);
}
void displaySeqInfo2(int s){
  //second line --displays current position 
  // ---6---- indicates 7th substep of 4th step
  if(s == stepindex){
    display.print(stepLengthIndex+1); 
  }else{
    if(s == getMaxStep()+1){
      display.print("<");
    }else if(s > getMaxStep()){
      display.print(" ");
    }else{
      display.print("-");  
    }    
  }
}
void displaySeqPos(int s){
  if(s == stepindex){
    display.print("*"); 
  }else{
    if(s == getMaxStep()+1){
      display.print("<");
    }else{
      display.print(" ");
    }
  }
}
void displayStepValue(int s){
  //things to display:
  //current step no, octave, interval, note,  length, duration+mode, velocity   
  
  //line one: step no
  display.print("Step: ");
  display.println(s+1);

  //line two,  len, duration, velocity
  display.print("Len:");
  display.print(in_length[s]);
  display.print(" Dur:"); 
  display.print(DURATION_MODE_TEXT[in_duration[s][0]]); 
  if(in_duration[s][0]==REPEAT){    
    display.print("");
    //when repeating, list the number of times to repeat
    display.print(in_duration[s][1]/(DURATION_MAX/in_length[s])+1);
  }
  display.print(" Vel:");
  if(in_velocity[s]<0){
    display.println("SKP");
  }else{
    display.println(in_velocity[s]);
  }

  //line three, octave, interval, note, form
  display.print("Oct:");
  display.print(in_note[s][0]);
  display.print(" Pos:"); 
  display.print(in_note[s][1]);
  display.print(" ");

  int note = g_key+ (in_note[s][0]*12) + g_scale[in_note[s][1]][0];

  display.print(getNoteLetter(note));    
  display.print(getNoteOctave(note)); 
  display.print(" ");
  display.println(FORM_NAMES[g_scale[in_note[s][1]][1]]);
}

void displayDebug(){  
  if(subMenuSelected){
    //get fresh info
    for(int i=0;i<8;i++){
      pollStep(i);
    }
        
    display.setTextSize(1);
    display.setFont(&TomThumb);
    display.println("");
                   //12345678901234567890123456789012
    display.println("     Displaying Raw Knob Data   ");
    display.println("          Press to Stop");
    
    display.print("N");
    for(int i=0;i<8;i++){
      display.print(" ");
      padDec(raw_note[i]);      
      display.print(raw_note[i],DEC);        
    }
    //display.println("");
    display.println("");
    display.print("L");

    for(int i=0;i<8;i++){
      display.print(" ");
      padDec(raw_length[i]);  
      display.print(raw_length[i],DEC);        
    }
    //display.println("");
    display.println("");  
    display.print("D");
  
    for(int i=0;i<8;i++){
      display.print(" ");
      padDec(raw_duration[i]);  
      display.print(raw_duration[i],DEC);        
    }
    //display.println("");
    display.println("");
    display.print("V");    
    for(int i=0;i<8;i++){
      display.print(" ");
      padDec(raw_velocity[i]);  
      display.print(raw_velocity[i],DEC);        
    }
    display.println("");
    
  }else{    
    display.setTextSize(2);
    display.println("InputTest");
    display.setTextSize(1);
    display.println("");
                   //123456789012345678901
    display.println("      Debug Off      ");
    display.println("   Press to Enable   ");
  }
}

void padHex(int val){
  if(val < 0xFF){
    display.print("0");
  }
  if(val < 0xF){
    display.print("0");
  }
}
void padDec(int val){
  if(val < 1000){
    display.print("0");
  }
  if(val < 100){
    display.print("0");
  }
  if(val < 100){
    display.print("0");
  }
}

bool stepInfoSelected(){
  return (menuIndex == MI_INFO && (g_infoModeIndex >= STP1)); 
}

void infoRefresh(int n){
  //when called within a loop, will refresh the display every N milliseconds
  //used for debugging/testing components
  
  static unsigned long lastDisplayMillis = 0;    
  unsigned currentMillis = millis();
  
  if(currentMillis-lastDisplayMillis >= n){    
    updateDisplay();
    //mark the time
    lastDisplayMillis = currentMillis;
  }
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
      case MI_INFO:
        return INFO_MODES_TEXT[g_infoModeIndex];
       break;
      case MI_SEQORDER:
        return SEQ_ORDERS_TEXT[g_seqOrderIndex];
       break;
    }
}

void arbitraryDebug(){
  //this is called from the main loop, and should contain whatever arbitrary code is needed to figure out if there's something wrong
  //currently spits out a serial output test of the knob value range for notes/chords
  int note_octave;
  int note_val;
  int prev_val;

  Serial.print("BEGIN FOR ");
  Serial.print(SCALE_TEXT[g_scaleIndex]);
  Serial.print("  max positions: ");
  Serial.println(g_scale_max);
  
  for(int i=0;i<1023;i++){
    note_octave = map(i,0,1023,-1,2);
    note_val =  map(i,0,1023,0,3*(g_scale_max+1))%(g_scale_max+1);

    int note = g_key+ (note_octave*12) + g_scale[note_val][0];

    if(prev_val != note_val){      
      Serial.print(i);
      Serial.print(" ");
      Serial.print(note_octave);
      Serial.print(" ");
      Serial.print(note_val);
      Serial.print(" == ");
      Serial.print(getNoteLetter(note));    
      Serial.print(getNoteOctave(note));
      Serial.print(" ");
      Serial.println(FORM_NAMES[g_scale[note_val][1]]);
    }
    prev_val = note_val;
  }
  Serial.println("-----------DONE----------");
}


/////////////////////////END DISPLAY & SERIAL FUNCTIONS //////////////////////////////////////////////////////////////////////////////


/************************************************************************************************************************************
 *                         BEGIN MIDI SIGNALING CODE
 ************************************************************************************************************************************/

void chordOn(int key, int chord[12], int velocity, int playMode){

  if(playMode == CHORD){
    //Serial.print("playing notes: ");
    for (int i=0; i<12; i++){
      if(chord[i]<0)
        break;
      MIDI.sendNoteOn(key+chord[i], velocity, 1);
      //Serial.print(key+chord[i]);
      //Serial.print(" ");
    }
  }
  if(playMode == SINGLE){
    if(chord[0]>=0)
      MIDI.sendNoteOn(key+chord[0],velocity,1);
  }

  //Serial.println("");
}

void chordOff(int key, int chord[12], int playMode){
  if(playMode == CHORD){
    for (int i=11; i>-1; i--){  //experimenting with decrementing -- last note turned off is doing so at a delay, happens both incrementing and decrementing...
      if(chord[i]>=0){
        MIDI.sendNoteOff(key+chord[i], 0, 1);                                       
      }
    }
  }
  if(playMode == SINGLE){
    if(chord[0]>=0)
      MIDI.sendNoteOff(key+chord[0],0,1);
  }
  Serial.println("");
}




void stepOn(int root, int scale[12][2], int octaveOffset, int scaleIndex, int velocity){
  if(!noteOn){
    
    if(g_playModeIndex == ARP){
      //TODO -- determine if in sustain mode
      if(true){
        int key = root+ (octaveOffset*12) + scale[scaleIndex-1][0];
        int * chord = chordFromForm(scale[scaleIndex-1][1]);
      
        int arpNote = getArpNote(chord);
        Serial.print("Arpnote:");
        Serial.print(arpNote);
        Serial.print(" ");
        Serial.println(key+chord[arpNote]);
        MIDI.sendNoteOn(key+chord[arpNote], velocity,1);
      }else{
        //duration is in sustain mode, play chord instead of arping
        chordOn(  root+ (octaveOffset*12) + scale[scaleIndex-1][0], 
            chordFromForm(scale[scaleIndex-1][1]),
            velocity,
            CHORD);
      }
      
    }else{
        chordOn(  root+ (octaveOffset*12) + scale[scaleIndex-1][0], 
            chordFromForm(scale[scaleIndex-1][1]),
            velocity,
            g_playModeIndex);
    }
              
    noteOn = true;
  }
}

void stepOff(int root, int scale[12][2], int octaveOffset, int scaleIndex){

    if(g_playModeIndex == ARP){
      //TODO -- determine if in sustain mode
      if(true){  
        int key = root+ (octaveOffset*12) + scale[scaleIndex-1][0];
        int * chord = chordFromForm(scale[scaleIndex-1][1]);
        
        int arpNote = getArpNote(chord);
        MIDI.sendNoteOn(key+chord[arpNote], 0,1);
      }else{
        //duration is in sustain mode, end the chord 
        chordOff(  root+ (octaveOffset*12) + scale[scaleIndex-1][0], 
              chordFromForm(scale[scaleIndex-1][1]),
              CHORD);
      }
    }else{
        chordOff(  root+ (octaveOffset*12) + scale[scaleIndex-1][0], 
              chordFromForm(scale[scaleIndex-1][1]),
              g_playModeIndex);
    }

    noteOn = false;
}

int getArpNote(int chord[12]){

  static int prevSLI = -1;
  static int randNote = 0;

  //determine chord size -- will form the basis for modulo operations, bounds for randoms
  int chordSize = 0;
  for (int i=11; i>-1; i--){
    //decrement from last position, mark first instance of -1 (no note)
    if(chord[i]<0){      
      chordSize = i;
    }
  }
  
  if(g_arpTypeIndex == UP){
     //arpeggio should increment every note & repeat after reaching end
     //pretty simple should be step % chordsize
     return stepLengthIndex%chordSize;
  }
  if(g_arpTypeIndex == DOWN){
    //arpeggio should start at top and decrement, repeating after reaching end    
    //also pretty simple, i think   chordsize - (stepcount%chordsize) ??
    return chordSize-(stepLengthIndex%chordSize);
  }
  if(g_arpTypeIndex == UPDOWN){
   //arpeggio shoud start at bottom, work its way up, then back down after reaching top, repeating once hitting bottom     
    if(stepLengthIndex > 0){
        //determine if going up or down (first half, or second half of sequence)
        if(stepLengthIndex%(chordSize*2)<chordSize)
            return stepLengthIndex%chordSize;
        else
            return (chordSize)-(stepLengthIndex%chordSize);
    }
    return 0;   
  }
  if(g_arpTypeIndex == UPDOWN){
    //arpeggio should be like updown, but in reverse    
    if(stepLengthIndex%(chordSize*2)>=chordSize)
        return stepLengthIndex%chordSize;
    else
        return (chordSize)-(stepLengthIndex%chordSize);  
  }
  if(g_arpTypeIndex == STAIR){
    //a stairstepping arpeggio
    //inputs should be  0 1 2 3 4 5 6
    //pattern should be 0 2 1 3 5 4 6
    //so essentially alternating 0 +1 -1
    int s = stepLengthIndex%chordSize;
    int trip = s%3;

    switch(trip){
        case 0:
            return s;
            break;
        case 1:
            return s+1;
            break;
        case 2:
            return s-1;
            break;
    }

  }
  if(g_arpTypeIndex == RANDO){
    //play random notes within the chord

    //since this function is expected to be called both for the noteOn and noteOff, we'll need to persist which random note was assigned.
    //determine if this is the first call of this index
    if(prevSLI != stepLengthIndex){      
      //this is the first call, save it
      prevSLI = stepLengthIndex;
      
      //generate, save, and return a random note
      randNote = random(chordSize-1);
      return randNote;
    }else{
      //this is the second call, return persisted note;
      return randNote;
    }   
  }
  
  Serial.println("FELL THROUGH?");
  //fall through -- in case something hasn't been implemented yet
  return 0;
}

void panic(){
  for (int i=0;i<128;i++){
     MIDI.sendNoteOff(i,0,1);
  }
}
//////////////////////////  END  MIDI SIGNAL CODE ////////////////////////////////////////////////////////////////////////////////////


/************************************************************************************************************************************
 *                         BEGIN  ARDUINO PROCEDURAL CODE
 *                         (SETUP  &  LOOP)
 ************************************************************************************************************************************/
void setup(){ 

  Serial.begin(9600);
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
  pinMode(extClockTogglePin,INPUT_PULLUP);
  
  pinMode(randomSeedPin,INPUT);
  
  randomSeed(analogRead(randomSeedPin));
  
  setGlobalScale(MAJ_CHORD_PROG);

  //get initial pin states for encoder (first poll often registers a "change" from uninit values, changes menu from desired default)
  pollEncoder(); 
  menuIndex = MI_INFO;
  
  
  MIDI.begin(MIDI_CHANNEL_OMNI);                      // Launch MIDI and listen to all channels
  MIDI.setHandleStart(handleStart);
  MIDI.setHandleStop(handleStop);
  MIDI.setHandleClock(handleClock);
 
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(5000);
  updateDisplay();  
}

void loop(){
  // the main running loop,  should put as few things here as possible to reduce latency  
  //test for menu knob changes
  pollEncoder();

  if(menuIndex == MI_DEBUG && subMenuSelected){
    //menu's set to debug and debugging enabled, call debug display code
    infoRefresh(500);
  }else{    
    //we're not debugging, proceed as normal    
    if(digitalRead(extClockTogglePin) == LOW){
            
      //if toggle switch is on, either listen for midi signals or start internal midi pulsing
      if(g_clockSrcIndex == CS_EXT){
        //Serial.print("r");
        if(prevToggleVal == false){
          //first cycle this switch is on... let's flush the midi read of any transient events...)          
          flushReads();
          prevToggleVal = true;
        }
        
        MIDI.read();
         
      }  
      if(g_clockSrcIndex == CS_INT){internalClockTick();
      }
    }else{
      //toggle switch is off... do anything while idle?
      //maybe repoll steps?
      pollAllSteps(1000);
      
      //TODO  REMOVE/COMMENT ME WHEN DONE DEBUGGING
      //arbitraryDebug();
      
      
      if(prevToggleVal == true){
        Serial.println("Toggled off");
        prevToggleVal = false;
      }
    }
  }
  if(stepInfoSelected()){
    
    infoRefresh(1000);
  }
}
///////////////////////////END  ARDUINO PROCEDURAL CODE///////////////////////////////////////////////////////////////////////////////


/************************************************************************************************************************************
 *                         BEGIN  MIDI EVENT HANDLING CODE
 ************************************************************************************************************************************/

void flushReads(void){
  //called when sequencer is toggled on after being off...
  //basically calls MIDI.read() multiple times with handling code off.
  //thus throwing away any transient midi data from prior to toggle
  flushingReads = true;
  //Serial.println("flushing");
  for(int i=0;i<128;i++){
    MIDI.read();
  }
  flushingReads = false;
}
void handleStart(void){
  //Serial.println("==================handleStart=====================");
  clk = 0-g_clkOffset;
  noteOn = false; 
  stepindex = 0;
  stepLengthIndex = 0;
  g_seqOrderPing = 1;
  pollStep(0);
  midiPlaying = true;
}

void handleStop(void){
  //Serial.println("==================handleStop=======================");
  //TODO write handleStop -- when DAW sends stop... housekeeping stuff like turn off LEDs, reset clocks to 0?

  panic();
  digitalWrite(clkPin,LOW); //turn off clock led
  digitalWrite(ledMuxPins[4], LOW); //turn off step indicator led
  
  midiPlaying = false;
}

void internalClockTick(){
  static unsigned long lastPulseMillis = 0;    
  unsigned currentMillis = millis();    
  
  //determine milli interval between pulses for current bpm  
  int clkInterval = (60000/g_bpm)/24;  //24 pulses per quarter... 60bpm = 1 beat per 1000ms =  (bpm*(1000/60))/24 = ms-per-clk
  
  //test to see if that interval has passed
  if(currentMillis-lastPulseMillis >= clkInterval){
    //if so, call handleclock
    handleClock();
    //mark pulse time
    lastPulseMillis = currentMillis;
  }
}

void handleClock(void){ 
  //When flag for midi call buffer flush is true, ignore all calls until flushing is over
  if(flushingReads == false){
    //Serial.print(clk);
    //Serial.print("|");
    
    if(clk>=0 && (clk%clkDivider==0)){
      //clock is on the trigger pulse for a step      
      //Serial.println("trig pulse");
        
      updateDisplay(); 
      //at the first step (for all modes)
      bool isFirstStep = (stepLengthIndex == 0);
      //determine if mode is set to repeating
      bool isRepeating = (in_duration[stepindex][0]==REPEAT);
      //determine if step is within repeating mode's pattern length value
      bool isWithinPattern = (stepLengthIndex > 0 && stepLengthIndex < in_duration[stepindex][1]/(DURATION_MAX/in_length[stepindex])+1);
      
      
      if( isFirstStep || (isRepeating && isWithinPattern)){
        //play the step
        stepOn(g_key, g_scale, in_note[stepindex][0], in_note[stepindex][1], in_velocity[stepindex]);
        writeMuxLED(stepindex,HIGH);   
           
      }     
    }
  
    
    if(g_staccato > 0){
      //for when staccato is being used
      //determine staccato interval (distance between current clock and next clock division)
      int staccInterval = g_staccato+1;
      if(staccInterval >= clkDivider){
        //ensure interval never exceeds divider -1 (allows a note 1 clock pulse long, anything shorter is impossible)
        staccInterval = clkDivider-1;
      }
      //truncate the current played note early (by staccato offset interval)
      if(noteOn && clk>=0 && ((clk+staccInterval)%clkDivider==0)){
        stepOff(g_key, g_scale, in_note[stepindex][0], in_note[stepindex][1]);
       
        writeMuxLED(stepindex,LOW);
      }
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
              stepOff(g_key, g_scale, in_note[stepindex][0], in_note[stepindex][1]);
              
              writeMuxLED(stepindex,LOW);  
          }
      }    
      
      stepLengthIndex++;    
      
      //determine if last beat within step
      if(stepLengthIndex > in_length[stepindex]-1){ 
        stepLengthIndex = 0; //if so, reset step length counter    
        nextStep(); //and move on to the next step (as determined by sequence pattern)
      }           
    }    
      
    //step may have been freshly incremented above, set to 0 if over max
    if(stepindex>stepMax-1){
      stepindex = 0;
    }  
    //increment clock, reset when > max
    clk++;
    if(clk>clkMax-1){
      clk = 0;
    }
  
  
    //Tempo/BPM LED logic: clock pulse on quarter notes (regardless of divider setting)
    if(clk>=0 && (clk%CLK_DIVS[QUARTER]==0)){
        digitalWrite(clkPin,1);
    }
    //LED logic: turn off one tick shy of a sixteenth (should be a nice blippy blink)
    if(clk>=0 && ((clk+1)%CLK_DIVS[SIXTEENTH]==0) ){
      digitalWrite(clkPin,0);
    }  
  }
}

void nextStep(){
//    //determine which step is next, poll it, increment
//    Serial.print(clk);
//    Serial.print(" curr step:");
//    Serial.print(stepindex);
    //randomSeed(analogRead(randomSeedPin));
    bool fiftyFifty = random(2);
    int oneOfEight = random(8);

//    Serial.print("Random test,  coin flip:");
//    Serial.print(fiftyFifty);
//    Serial.print(" 8roll: ");
//    Serial.println(oneOfEight);
   
    if(g_seqOrderIndex == FORWARD){ //simple increment
      nextStepIndex = stepindex+1;
      if(nextStepIndex > stepMax-1)
        nextStepIndex = 0;    
    }
    if(g_seqOrderIndex == REVERSE){  //simple decrement
      nextStepIndex = stepindex-1;
      if(nextStepIndex < 0)
        nextStepIndex = stepMax-1;
    }
    if(g_seqOrderIndex == RAND){  //random
      nextStepIndex = oneOfEight;
    }
    if(g_seqOrderIndex == BROWNIAN){ //randomly pick increment or decrement
      
      if(fiftyFifty){
        nextStepIndex = stepindex+1;
        if(nextStepIndex > stepMax-1)
          nextStepIndex = 0;  
        }else{
          nextStepIndex = stepindex-1;
          if(nextStepIndex < 0)
            nextStepIndex = stepMax-1;
        }
    }
    if(g_seqOrderIndex == PINGPONG){ //back and forth  (CHANGED, test me)
      
      if(g_seqOrderPing){   
        //going forward
        nextStepIndex = stepindex+1; //increment 
        if(nextStepIndex == stepMax -1){ //if next step is last
          g_seqOrderPing = 0;     //set backward for next time
        }
      }else{
        //going backward
        nextStepIndex = stepindex-1; //decrement
        if(nextStepIndex == 0) //if next step is first
          g_seqOrderPing = 1; //set forward for next time
      }
    }

//    Serial.print(" sw: ");
//    Serial.print(digitalRead(extClockTogglePin));
//    Serial.print(" next step:");
//    Serial.println(nextStepIndex);
    
    //read next step's data from knobs
    //pollStep(nextStepIndex); 
    //testing overhead of always polling (better for display purposes, possibly performance penalty)
    pollAllSteps(); 
    
    //test to see if next step's velocity < 0 (flag for stop seq);
    if((stepindex < stepMax-1) && (in_velocity[nextStepIndex] < 0)){
        nextStepIndex = 0;
        pollStep(0); 
    }    
    stepindex = nextStepIndex;    
}

int getPatternLength(){
  //totals up contents of length array, minding
  int t = 0;
  
  for(int i=0;i<=getMaxStep();i++){
    if(i<stepMax)  
      t += in_length[i];
  }
  return t;  
}

int getMaxStep(){
  //scans velocities array, returns the max length of pattern as defined by a velocity knob at 0
  for(int i=1;i<stepMax;i++){
    if(in_velocity[i] < 0)
      return i-1;      
  }
  return stepMax;
}
