#ifndef _FORMS_H
#define _FORMS_H

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



static int CHROMATIC_PROG[12][2] = {      {0,MAJ},
                                          {1,ROOTONLY},
                                          {2,ROOTONLY},
                                          {3,ROOTONLY},
                                          {4,ROOTONLY},
                                          {5,ROOTONLY},
                                          {6,MAJ}, 
                                          {7,ROOTONLY},
                                          {8,ROOTONLY},
                                          {9,MAJ},
                                          {10,ROOTONLY},
                                          {11,ROOTONLY}};

static int MAJ_CHORD_PROG[12][2] = {      {0,MAJ},
                                          {2,MIN},
                                          {4,MIN},
                                          {5,MAJ},
                                          {7,MAJ},
                                          {9,MIN},
                                          {11,DIM},  //WTF... should be 11?  But 12 plays correctly???
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
                                          {10,MAJ},  // SAME... should be 10...?? but 11 is better?
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF},
                                          {-1,UNDEF}};



/*
static byte SCALE_CHRO[12] = {0,1,2,3,4,5,6,7,8,9,10,11};
static byte SCALE_MAJ_DIA[7] = {0,2,4,5,7,9,11};
static byte SCALE_MIN_DIA[7] = {0,2,3,5,7,9,10};

static byte TRIAD_MAJ[3] = {0,4,7}; //0
static byte TRIAD_MIN[3] = {0,3,7}; //1
static byte TRIAD_DIM[3] = {0,3,6}; //2
static byte TRIAD_SUS2[3] = {0,2,7}; //3
static byte TRIAD_SUS4[3] = {0,5,7}; //4

static byte INTERVAL_MAJ_THIRD[3] = {0,4};
static byte INTERVAL_MIN_THIRD[3] = {0,3};
static byte INTERVAL_FIFTH[3] = {0,7};

static byte MAJ_CHORD_PROG[7][2] = { {0,MAJ},
                                          {2,MIN},
                                          {4,MIN},
                                          {5,MAJ},
                                          {7,MAJ},
                                          {9,MIN},
                                          {11,DIM} };

                                          
static byte MIN_CHORD_PROG[7][2] = { {0,MIN},
                                          {2,DIM},
                                          {3,MAJ},
                                          {5,MIN},
                                          {7,MIN},
                                          {8,MAJ},
                                          {10,MAJ} };

*/


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

#endif // _FORMS_H
