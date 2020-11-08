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


const int MENU_ITEMCOUNT = 6;
enum MENU_ITEMS_EN {MI_KEY, MI_SCALE, MI_PLAYMODE, MI_CLOCKDIV, MI_ARPTYPE, MI_CLOCKSRC, MI_BPM};
char MENU_TEXT[8][10] = {"KEY", "SCALE", "NOTE MODE", "CLOCK DIV", "ARP TYPE", "CLOCK SRC", "INT. BPM"};


int PLAY_MODES_ITEMCOUNT = 3;
enum PLAY_MODES {CHORD, SINGLE, ARP};
char PLAY_MODES_TEXT[3][10] = {"PolyChord", "MonoNote", "MonoArp"};


const int CLOCK_DIVS_ITEMCOUNT = 6;
enum CLOCK_DIV_EN {SIXTEENTH, EIGHTH, QUARTER, HALF, WHOLE, DOUBLEWHOLE};
char CLK_DIV_TEXT[6][9] = {"1/16", "1/8", "1/4", "1/2", "1", "2"};
const int  CLK_DIVS[6] = {		6,	12,	24,	48,	96,	192};
						//96/16, 96/8, 96/4, 96/2, 96, 96*2

const int CLKSRC_ITEMCOUNT = 2;
enum CLKSRC_EN {CS_INT, CS_EXT};
char CLKSRC_TEXT[2][10] = {"Internal", "External"};

enum DURATION_MODE {HOLD,ONCE,REPEAT};
const int DURATION_MAX = 800;

const int ARPTYPE_ITEMCOUNT = 4;
enum ARPTYPE_EN {UP, DOWN, UPDOWN, RANDO};
char ARPTYPE_TEXT[4][10] = {"UP","DOWN","UP&DOWN"};


#endif //_CONSTANTS_H
