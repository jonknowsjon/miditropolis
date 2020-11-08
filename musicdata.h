#ifndef _MUSICDATA_H
#define _MUSICDATA_H

enum FORMS {MAJ, MIN, DIM, SUS2, SUS4, MAJ3, MIN3, FIFTH, AUGMENTED, ROOTONLY, UNDEF};
char FORM_NAMES[11][10] = {"Major","Minor", "Dim", "Sus2","Sus4","MajThird","MinThird","Fifth","Aug","Root","X"};
char FORM_SHORTNAMES[11][5] = {"M","m","°","s2","s4","(M3)","(m3)","(5)","+","","X"};

static int SCALE_UNDEF[12] =       {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

static int SCALE_CHRO[12] =    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11};
static int SCALE_MAJ_DIA[12] = { 0, 2, 4, 5, 7, 9,11,-1,-1,-1,-1,-1};
static int SCALE_MIN_DIA[12] = { 0, 2, 3, 5, 7, 9,10,-1,-1,-1,-1,-1};

static int TRIAD_MAJ[12] =     { 0, 4, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1}; //0
static int TRIAD_MIN[12] =     { 0, 3, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1}; //1
static int TRIAD_DIM[12] =     { 0, 3, 6,-1,-1,-1,-1,-1,-1,-1,-1,-1}; //2
static int TRIAD_SUS2[12] =    { 0, 2, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1}; //3
static int TRIAD_SUS4[12] =    { 0, 5, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1}; //4
static int TRIAD_AUG[12] =     { 0, 4, 8,-1,-1,-1,-1,-1,-1,-1,-1,-1}; //5

static int IVL_MAJ_THIRD[12] = { 0, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
static int IVL_MIN_THIRD[12] = { 0, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
static int IVL_FIFTH[12] =     { 0, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

static int ROOT_ONLY[12] = { 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};


/*
 * Need to add more modes / forms
 * Pentatonic and these below
 * https://en.wikipedia.org/wiki/Mode_(music)#Modern_modes
*/
/*

A B C D E F G A B C D E F G
	0 1 2 3 4 5 6 7
A # B C # D # E F # G # A # B C # D # E F # G
	  0 1 2 3 4 5 6 7 8 9101112

Ionian	CDEFGABC   0 2 4 5 7 9 11 
Dorian  DEFGABCD   2 4 5 7 9 11 0
Phyrgian EFGABCDE  4 5 7 9 11 0 2
Lydian FGABCDEF	   5 7 9 11 0 2 4
Mixolydian GABCDEFG7 9 11 0 2 4 5
Aeolian ABCDEFGA   9 11 0 2 4 5 7
Locrian BCDEFGAB  11  0	2 4 5 7 9
 */

static int CHROMATIC_PROG[12][2] = {      {0,ROOTONLY},
                                          {1,ROOTONLY},
                                          {2,ROOTONLY},
                                          {3,ROOTONLY},
                                          {4,ROOTONLY},
                                          {5,ROOTONLY},
                                          {6,ROOTONLY}, 
                                          {7,ROOTONLY},
                                          {8,ROOTONLY},
                                          {9,ROOTONLY},
                                          {10,ROOTONLY},
                                          {11,ROOTONLY}};

static int MAJ_CHORD_PROG[12][2] = {      {0,MAJ},
                                          {2,MIN},
                                          {4,MIN},
                                          {5,MAJ},
                                          {7,MAJ},
                                          {9,MIN},
                                          {11,DIM},  
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF}};

                                          
static int MIN_CHORD_PROG[12][2] = {      {0,MIN},
                                          {2,DIM},
                                          {3,MAJ},
                                          {5,MIN},
                                          {7,MIN},
                                          {8,MAJ},
                                          {10,MAJ},  
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF}};

static int IONIAN_MODE_PROG[12][2] = {    {0,MAJ},
                                          {2,MIN},
                                          {4,MIN},
                                          {5,MAJ},
                                          {7,MAJ},
                                          {9,MIN},
                                          {11,DIM},  
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF}};
static int DORIAN_MODE_PROG[12][2] = {    {2,MIN},
                                          {4,MIN},
                                          {5,MAJ},
                                          {7,MAJ},
                                          {9,MIN},
                                          {11,DIM},
                                          {0,MAJ},  
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF}};
static int PHRYGIAN_MODE_PROG[12][2] = {  {4,MIN},
                                          {5,MAJ},
                                          {7,MAJ},
                                          {9,MIN},
                                          {11,DIM},
                                          {0,MAJ},
                                          {2,MIN},  
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF}};
static int LYDIAN_MODE_PROG[12][2] = {    {5,MAJ},
                                          {7,MAJ},
                                          {9,MIN},
                                          {11,DIM},
                                          {0,MAJ},
                                          {2,MIN},
                                          {4,MIN},  
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF}};
static int MIXOLYDIAN_MODE_PROG[12][2] ={ {7,MAJ},
                                          {9,MIN},
                                          {11,DIM},
                                          {0,MAJ},
                                          {2,MIN},
                                          {4,MIN},
                                          {5,MAJ},  
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF}};
static int AEOLIAN_MODE_PROG[12][2] = {   {9,MIN},
                                          {11,DIM},
                                          {0,MAJ},
                                          {2,MIN},
                                          {4,MIN},
                                          {5,MAJ},
                                          {7,MAJ},  
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF}};
static int LOCRIAN_MODE_PROG[12][2] = {   {11,DIM},
                                          {0, MAJ},
                                          {2,MIN},
                                          {4,MIN},
                                          {5,MAJ},
                                          {7,MAJ},
                                          {9,MIN},  
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF}};										  
										  
const int SCALE_ITEMCOUNT = 10;
enum SCALE_EN{MAJ_DIA, MIN_DIA, CHROMA,ION,DOR,PHRY,LYD,MIXO,AEOL,LOCR};
char SCALE_TEXT[10][10] = {"MajDiatnc","MinDiatnc","Chromatic","Ionian","Dorian","Phrygian","Lydian","Mixolydn","Aeolian","Locrian",};

int * chordFromForm(int form){
    switch(form){
      case MAJ:
        return TRIAD_MAJ;
        break;
      case MIN:
        return TRIAD_MIN;
        break;
      case DIM:
        return TRIAD_DIM;
        break;
      case SUS2:
        return TRIAD_SUS2;
        break;
      case SUS4:
        return TRIAD_SUS4;
        break;
      case ROOTONLY:
        return ROOT_ONLY;
        break;
      case AUGMENTED:
        return TRIAD_AUG;
        break;
      default:
        return SCALE_UNDEF;
    }
 //maj min dim sus2 sus4  
}

char LETTERS[12][3] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

char* getNoteLetter(int noteVal){  
  return LETTERS[noteVal%12];
}

int getNoteOctave(int noteVal){
  return ((noteVal / 12)-1);
}

#endif // _MUSICDATA_H
