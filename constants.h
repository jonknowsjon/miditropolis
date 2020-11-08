#ifndef _CONSTANTS_H
#define _CONSTANTS_H

const int stepMax = 8; //number of steps being programmed for (don't change this unless you're looking to add/remove knobs and tweak array sizes)

const int MENU_ITEMCOUNT = 6;
enum MENU_ITEMS_EN {MI_KEY, MI_SCALE, MI_PLAYMODE, MI_CLOCKDIV, MI_ARPTYPE, MI_CLOCKSRC};
const char MENU_TEXT[8][14] = {"KEY", "SCALE", "NOTE MODE", "CLK DIV", "ARP TYPE", "CLOCK SRC"};


int PLAY_MODES_ITEMCOUNT = 3;
enum PLAY_MODES {CHORD, SINGLE, ARP};
const char PLAY_MODES_TEXT[3][9] = {"Chord", "Note", "Arpeggio"};


const int CLOCK_DIVS_ITEMCOUNT = 6;
enum CLOCK_DIV_EN {SIXTEENTH, EIGHTH, QUARTER, HALF, WHOLE, DOUBLEWHOLE};
char CLK_DIV_TEXT[6][7] = {"1/16", "1/8" "1/4", "1/2", "1", "2"};
const int  CLK_DIVS[6] = {		6,	12,	24,	48,	96,	192};
						//96/16, 96/8, 96/4, 96/2, 96, 96*2

const int CLKSRC_ITEMCOUNT = 2;
enum CLKSRC_EN {CS_INT, CS_EXT};
const char CLKSRC_TEXT[2][10] = {"Internal", "External"};

enum DURATION_MODE {HOLD,ONCE,REPEAT};
const int DURATION_MAX = 800;

#endif //_CONSTANTS_H
