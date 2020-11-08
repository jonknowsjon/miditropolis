#ifndef _CONSTANTS_H
#define _CONSTANTS_H

//timing constants
const int clkMax=96*4; //max clock value: 4 measures worth... 
             //4 measures allows for breve notes (double whole)

const int stepMax = 8; //number of steps being programmed for (don't change this unless you're looking to add/remove knobs and tweak array sizes

//Min+Max for Key variable -- note spread will be an octave above and below key note, so need to allow room for it
const int noteMin = 12; //one octave up from 0 (actual min)
const int noteMax = 115; //one octave down from 127 (actual max)

const int bpmMin = 40;
const int bpmMax = 240;
const int offsetMin = 0; //values for clock offset range
const int offsetMax = 192; //used to mitigate processing delay between external clock and notes output
const int offsetStep = 1; //amount offset will be incremented by when adjusted... if offset is dialed in, perhaps a coarser grain could be used to syncopate/judder the output

const int staccMin = 0;  
const int staccMax = 47;


const int MENU_ITEMCOUNT = 12; //11?
enum MENU_ITEMS_EN {      MI_INFO,  MI_SCALE,   MI_KEY,   MI_PLAYMODE,  MI_CLOCKDIV,  MI_ARPTYPE,   MI_STACC,   MI_SEQORDER,  MI_CLOCKSRC,  MI_BPM,     MI_CLKOFFSET, MI_DEBUG};
char MENU_TEXT[12][10] = {"INFO",   "SCALE",    "KEY",    "NOTE MODE",  "CLOCK DIV",  "ARP TYPE",   "STACCATO", "SEQ ORDER" , "CLOCK SRC",  "INT. BPM", "EXTOFFSET",  "InputTest"};


int INFO_MODES_ITEMCOUNT = 11;
enum INFO_MODES_EN {ABOUT, SEQINFO, STEPVALUE,STP1,STP2,STP3,STP4,STP5,STP6,STP7,STP8};
char INFO_MODES_TEXT[11][10] = {"ABOUT","Sequence","Curr.Step","S1 Vals","S2 Vals","S3 Vals","S4 Vals","S5 Vals","S6 Vals","S7 Vals","S8 Vals"};

int PLAY_MODES_ITEMCOUNT = 3;
enum PLAY_MODES {CHORD, SINGLE, ARP};
char PLAY_MODES_TEXT[3][10] = {"PolyChord", "MonoNote", "MonoArp"};

const int CLOCK_DIVS_ITEMCOUNT = 6;
enum CLOCK_DIV_EN {SIXTEENTH, EIGHTH, QUARTER, HALF, WHOLE, DOUBLEWHOLE};
char CLK_DIV_TEXT[6][9] = {"16th", "8th", "Qtr", "Half", "Whole", "DblWhole"};
const int  CLK_DIVS[6] = {		6,	12,	24,	48,	96,	192};
						//96/16, 96/8, 96/4, 96/2, 96, 96*2

const int CLKSRC_ITEMCOUNT = 2;
enum CLKSRC_EN {CS_INT, CS_EXT};
char CLKSRC_TEXT[2][10] = {"Internal", "External"};

enum DURATION_MODE {HOLD,ONCE,REPEAT};
char DURATION_MODE_TEXT[3][4] = {"HLD","ONE","RP"};
const int DURATION_MAX = 800;

const int ARPTYPE_ITEMCOUNT = 4;
enum ARPTYPE_EN {UP, DOWN, UPDOWN, RANDO};
char ARPTYPE_TEXT[4][10] = {"UP","DOWN","UP&DOWN","RAND"};

const int SEQ_ORDERS_ITEMCOUNT = 5;
enum SEQ_ORDERS_EN {FORWARD,REVERSE, RAND, BROWNIAN, PINGPONG};
char SEQ_ORDERS_TEXT[5][10] = {"Forward","Reverse", "Random", "Brownian", "PingPong"};



//table for whether to read the potentiometer or not
//in the event that some of the pots arent working, this quick n dirty table
//can allow partial usage/testing while filling in 
static const bool POT_ENABLED[4][8] = {
                                        {1,1,1,1,1,1,1,1},
                                        {0,1,1,1,1,1,1,1},
                                        {1,1,1,1,1,1,1,1},
                                        {1,0,0,0,1,0,1,0}
                                        };

static const int DEFAULT_OCTAVE = 0;
static const int DEFAULT_NOTE = 0;
static const int DEFAULT_LENGTH = 1;
static const int DEFAULT_DURATION = 1;
static const int DEFAULT_DUR_MODE = ONCE;
static const int DEFAULT_VELOCITY = 127;

#endif //_CONSTANTS_H
