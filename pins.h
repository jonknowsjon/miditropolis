#ifndef _PINS_H
#define _PINS_H


//pin constants
static const unsigned clkPin = 52; //pin for tempo LED, also used for debugging loops

static const unsigned ledMuxPins[5] = {22,23,24,25,53}; //4 pins for interfacing multiplexer + 1 signal pin

static const unsigned row12MuxPins[4] = {26,27,28,29}; //pins for multiplexer for rows 1 and 2    
static const unsigned row34MuxPins[4] = {26,27,28,29}; //pins for multiplexer for rows 3 and 4
static const unsigned row12SignalPin = 8;
static const unsigned row34SignalPin = 9;

static const unsigned rotaryEncoderPin[2] = {2,3}; //two pins for rotary encoder values
static const unsigned rotaryEncoderSwPin = 46;

static const unsigned extClockTogglePin = 50; //when clock source is internal, toggle for whether clock is running or not




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


#endif //_PINS_H
