/***************************************************************************************************************************************************
 * MIDIBOX
 * An Arduino-driven midi sequencer, 
 * inspired by the Intellijel Metropolis Eurorack Module, and by transitive property, ryktnk's Roland 100m sequencer
 * 
 * 
 * 
 * Physical TODO: 
 * Knob data pins wire
 * Mux wiring harnesses/termination -- skipping for how
 * Feet -- Adhesive vs screwed from within
 *  
 * 
 * Coding TODO:  Features still needed:
 * HandleStop
 * Arpeggiation
 * Step Order (Menu item, default forwards... additional: backwards, random, brownian (plinko))   --- RNG NOT WORKING
 * Step hold-- switch,  while note on, sustains current note, while off, does not?  (resume quantized?) 
  * More Scales / Modes  -- 
 * Info Menu:
 *    Current beats/clock (perhaps show step+stepindex as dashes+numbers ie --5---- would be 5th stepindex of 3rd step
 *    Current Settings/Vals (show current step's Values (for help fine tuning)
 *    
 * 
 * MUX analog inputs not working at all...    
 * 
 * 
 * Needs Testing:
 * Knob values / assignments
 * DebugInfo -- seems to be working
 * Sequence Info -- working, needs cosmetics
 * Random+Brownian Sequences  -- modified seed/pin initialization, may work?
 * Staccato -- seems to be working
 * 
 * *************************************************************************************************************************************************
 */


#include <MemoryFree.h>
#include <pgmStrToRAM.h>
#include <splash.h>
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

//display & selection variables
int encoderVal, encoderPrevVal; //current + previous values for rotary encoder
bool encoderSwVal; //value for rotary enc's switch

//global settings
int g_key = 60; //root note value -- all other note values are offsets of this -- 60 == Middle C
int g_scale[12][2] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //placeholder for the scale of notes to be worked within
int g_scale_max = 12;

//menu index variables (what the display is  focussed on, and it's current setting's index (for retrieving display text)
int menuIndex = MI_INFO;
bool subMenuSelected = false;
int g_scaleIndex = MAJ_DIA;
int g_clockDivIndex = EIGHTH;
int g_playModeIndex = CHORD;
int g_clockSrcIndex = CS_EXT;
int g_bpm = 120; //bpm value for internal clock 
int g_arpTypeIndex = 0; //unimplemented right now...
int g_infoModeIndex = SEQINFO;
int g_seqOrderIndex = FORWARD;
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
  //set digital pins to mux selector
  for(int i=0; i<4; i++){
    digitalWrite(inputMuxPins[i]+(8*muxGroup), //muxGroup 0 or 1, determines whether first or second 8 in 16 channel multiplexer
    MUX_TRUTH_TABLE[muxIndex][i]);
  }
  //read signal
  return analogRead(signalPin);
}

void pollStep(int s){
  //poll the knob settings for step s and write them to their corresponding array positions

  //if step to poll >= max, poll 0 instead
  if(s >= stepMax-1)
    s = 0;  
  
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
  int note_octave = map(noteVal,0,1023,-1,1);
  int note_val =  map(noteVal,0,1023,0,36)%12;
  int dur_mode =  map(durationVal,0,1023,0,3); //split into 2 quarters + half : HOLD, ONCE, REPEAT*2 
  if(dur_mode >= 2) 
     dur_mode = 2; //Q3 and Q4 = REPEAT = 2;  
  int dur_val =  map(durationVal,0,1023,(0-DURATION_MAX),DURATION_MAX); //first half, value is ignored, so begin iteration at 512=0


  //UNCOMMENT THESE WHEN READY TO READ KNOB VALUES  (MUX IS HOOKED UP)
//  //assign values to memory
//  in_note[s][0] = note_octave;
//  in_note[s][1] = note_val;
//  in_length[s] = map(lengthVal,0,1023,1,8);
//  in_duration[s][0] = dur_mode; 
//  in_duration[s][1] = dur_val;
//  in_velocity[s] = map(velocityVal,0,1023,-50,120);
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
      if(changeTime - lastChangeTime > 75){ //if last change was less than X ms ago, ignore it... (debouncing)  

        bool increment = (encoderVal != digitalRead(rotaryEncoderPin[1]));
                       
        menuNavigate(subMenuSelected, increment);        
        updateDisplay();
        lastChangeTime = changeTime;
      }
    }
    encoderPrevVal = encoderVal;   
}

void menuNavigate(bool submenu, bool increment){  
  Serial.print(submenu);
  Serial.println(increment);
  memoryLog();
  
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
    Serial.print("Submenu ");
    Serial.println(menuIndex);
    
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

 
  if(!subMenuSelected)
    display.print(">");
  else
    display.print(" ");  
    
  display.println(MENU_TEXT[menuIndex]);

  if(menuIndex != MI_DEBUG){    
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

void displayInfo(){
  display.setTextSize(1);
  if(g_infoModeIndex == GENERAL){
    display.println("Howdy Partner");
  }
  if(g_infoModeIndex == SEQINFO){    
    for(int i=0;i<8;i++){
      displaySeqInfo1(i);  
    }
    display.println("");
    for(int i=0;i<8;i++){
      displaySeqInfo2(i);
    }    
  }
  if(g_infoModeIndex == STEPVALUE){
    display.println("StepHere");
    //things to display:
    //current step no, octave, interval, note,  length, duration+mode, velocity    
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
    display.print("-");
  }
}

void displayDebug(){
  display.setTextSize(1);
  if(subMenuSelected){
    //get fresh info
    for(int i=0;i<8;i++){
      pollStep(i);
    }
        
    display.setFont(&TomThumb);

    display.print("N");
    for(int i=0;i<8;i++){
      display.print(" ");
      display.print(raw_note[i],HEX);        
    }
    display.println("");
    display.print("L");

    for(int i=0;i<8;i++){
      display.print(" ");
      display.print(raw_length[i],HEX);        
    }
    display.println("");  
    display.print("D");
  
    for(int i=0;i<8;i++){
      display.print(" ");
      display.print(raw_duration[i],HEX);        
    }
    display.println("");
    display.print("V");    
    for(int i=0;i<8;i++){
      display.print(" ");
      display.print(raw_velocity[i],HEX);        
    }
    display.println("");
    
  }else{    
    display.println("Debug Off");
    display.println("Press to Enable");
  }
}

void debugRefresh(){
  //when called within a loop, will refresh the display every N milliseconds
  //used for debugging/testing components
  
  static unsigned long lastDisplayMillis = 0;    
  unsigned currentMillis = millis();
  
  if(currentMillis-lastDisplayMillis >= 750){    
    updateDisplay();
    //mark the time
    lastDisplayMillis = currentMillis;
  }
}

void memoryLog(){
        Serial.print(millis());
        Serial.print(" ");
        Serial.print(menuIndex);
        Serial.print(" ");
        Serial.println(freeMemory());
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
/////////////////////////END DISPLAY & SERIAL FUNCTIONS //////////////////////////////////////////////////////////////////////////////


/************************************************************************************************************************************
 *                         BEGIN MIDI SIGNALING CODE
 ************************************************************************************************************************************/

void chordOn(int key, int chord[12], int velocity){
  //TODO arpeggiation probably requires total reworking!
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
  // the main running loop,  should put as few things here as possible to reduce latency  

  //test for menu knob changes
  pollEncoder();

  if(menuIndex == MI_DEBUG && subMenuSelected){
    //menu's set to debug and debugging enabled, call debug display code
    debugRefresh();
  }else{    
    //we're not debugging, proceed as normal    
    if(digitalRead(extClockTogglePin) == LOW){
      //if toggle switch is on, either listen for midi signals or start internal midi pulsing
      if(g_clockSrcIndex == CS_EXT){
        MIDI.read();
      }  
      if(g_clockSrcIndex == CS_INT){  
        internalClockTick();
      }
    }
  }
}
///////////////////////////END  ARDUINO PROCEDURAL CODE///////////////////////////////////////////////////////////////////////////////


/************************************************************************************************************************************
 *                         BEGIN  MIDI EVENT HANDLING CODE
 ************************************************************************************************************************************/
void handleStart(void){
  clk = 0-g_clkOffset;
  noteOn = false; 
  stepindex = 0;
  stepLengthIndex = 0;
  pollStep(0);
  midiPlaying = true;
}

void handleStop(void){
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
  
  if(clk>=0 && (clk%clkDivider==0)){
    //clock is on the trigger pulse for a step      
    
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
      nextStep(); //and move on to the next step
    }           
  }

  //step may have been freshly incremented above, set to 0 if over max
  if(stepindex>stepMax-1){
    stepindex = 0;
  }  
  
  //Tempo/BPM LED logic: clock pulse on quarter notes (regardless of divider setting)
  if(clk>=0 && (clk%CLK_DIVS[QUARTER]==0)){
      digitalWrite(clkPin,1);
  }
  //LED logic: turn off before next sixteenth
  if(clk>=0 && ((clk+4)%CLK_DIVS[QUARTER]==0) ){
    digitalWrite(clkPin,0);
  }    
  
  //increment clock, reset when > max
  clk++;
  if(clk>clkMax-1){
    clk = 0;
  }
}

void nextStep(){
//    //determine which step is next, poll it, increment
//    Serial.print(clk);
//    Serial.print(" curr step:");
//    Serial.print(stepindex);

    
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
    if(g_seqOrderIndex == RANDO){  //random
      nextStepIndex = random(stepMax-1);
    }
    if(g_seqOrderIndex == BROWNIAN){ //randomly pick increment or decrement
      
      if(random(1)){
        nextStepIndex = stepindex+1;
        if(nextStepIndex > stepMax-1)
          nextStepIndex = 0;  
        }else{
          nextStepIndex = stepindex-1;
          if(nextStepIndex < 0)
            nextStepIndex = stepMax-1;
        }
    }

//    Serial.print(" sw: ");
//    Serial.print(digitalRead(extClockTogglePin));
//    Serial.print(" next step:");
//    Serial.println(nextStepIndex);
    
    pollStep(nextStepIndex); //read next step's data from knobs
    
    //test to see if next step's velocity < 0 (flag for stop seq);
    if((stepindex < stepMax-1) && (in_velocity[nextStepIndex] < 0)){
        nextStepIndex = 0;
        pollStep(0); 
    }    
    stepindex = nextStepIndex;    
}
