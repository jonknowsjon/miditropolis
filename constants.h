#ifndef _CONSTANTS_H
#define _CONSTANTS_H

//mux pinout
const bool MUX_TRUTH_TABLE[16][4] = { {0,0,0,0},
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


enum MENU_ITEMS {MI_KEY, MI_SCALE, MI_PLAYMODE, MI_CLOCKDIV, MI_ARPTYPE, MI_CLOCKSRC);
char MENU_TEXT[14][8] = {"KEY", "SCALE", "NOTE MODE", "CLK DIV", "ARP TYPE", "CLOCK SRC");



enum PLAY_MODES {CHORD, SINGLE, ARP};
char PLAY_MODES_TEXT[9][3] = {"Chord", "Note", "Arpeggio"

enum CLOCK_DIVS {SIXTEENTH, EIGHTH, QUARTER, HALF, WHOLE, DOUBLEWHOLE};
char CLK_DIV_TEXT[7][6] = {"1/16", "1/8" "1/4", "1/2", "1", "2"};
int  CLK_DIV_VALS[6] = {		6,	12,	24,	48,	96,	192};
						//96/16, 96/8, 96/4, 96/2, 96, 96*2

enum CLKSRC {CS_INT, CS_EXT};
char CLKSRC_TEXT[10][2] = {"Internal", "External"};

#endif //_CONSTANTS_H