#ifndef _MUSICDATA_H
#define _MUSICDATA_H

enum FORMS {MAJ, MIN, DIM, SUS2, SUS4, MAJ3, MIN3, FIFTH, ROOTONLY, UNDEF};



static int SCALE_UNDEF[12] =       {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

static int SCALE_CHRO[12] =    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11};
static int SCALE_MAJ_DIA[12] = { 0, 2, 4, 5, 7, 9,11,-1,-1,-1,-1,-1};
static int SCALE_MIN_DIA[12] = { 0, 2, 3, 5, 7, 9,10,-1,-1,-1,-1,-1};

static int TRIAD_MAJ[12] =     { 0, 4, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1}; //0
static int TRIAD_MIN[12] =     { 0, 3, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1}; //1
static int TRIAD_DIM[12] =     { 0, 3, 6,-1,-1,-1,-1,-1,-1,-1,-1,-1}; //2
static int TRIAD_SUS2[12] =    { 0, 2, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1}; //3
static int TRIAD_SUS4[12] =    { 0, 5, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1}; //4

static int IVL_MAJ_THIRD[12] = { 0, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
static int IVL_MIN_THIRD[12] = { 0, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
static int IVL_FIFTH[12] =     { 0, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

static int ROOT_ONLY[12] = { 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};


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


const int SCALE_ITEMCOUNT = 3;
enum SCALE_EN{MAJ_DIA, MIN_DIA, CHROMA};
char SCALE_TEXT[3][10] = {"MajDiatnc","MinDiatnc","Chromatic"};

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
