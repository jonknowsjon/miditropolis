#ifndef _PINS_H
#define _PINS_H


//pin constants
static const unsigned clkPin = 23; //pin for tempo LED, also used for debugging loops

static const unsigned ledMuxPins[5] = {30,31,32,33,29}; //4 pins for interfacing multiplexer + 1 signal pin

static const unsigned row12MuxPins[4] = {38,39,40,41}; //pins for multiplexer for rows 1 and 2    
static const unsigned row34MuxPins[4] = {34,35,36,37}; //pins for multiplexer for rows 3 and 4
static const unsigned row12SignalPin = 8;
static const unsigned row34SignalPin = 9;

static const unsigned rotaryEncoderPin[2] = {42,43}; //two pins for rotary encoder values
static const unsigned rotaryEncoderSwPin = 44;

static const unsigned extClockTogglePin = 25; //when clock source is internal, toggle for whether clock is running or not

static const unsigned randomSeedPin = 15; //unconnected pin used for initializing pseudo rng



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
