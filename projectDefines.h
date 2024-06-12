#ifndef PROJECT_DEFINES_H
#define PROJECT_DEFINES_H

#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_vector.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"

// #define SIZE_LIST 10

#define INPUTTRACKSIZE 100
#define NHITS 5
#define MIN_DIST 3000 // ~30x30 area // max distance is 1000
#define MAX_SHARED 2
#define MIN_THRESHOLD 0.5

#define HIT_SIZE 16
#define SCORE_SIZE 16

typedef ap_int<HIT_SIZE> data_t;
typedef ap_fixed<SCORE_SIZE, 1> nnscore_t;
// typedef ap_axiu<HIT_SIZE*3,0,0,0> Hit_t;
// typedef ap_axiu<HIT_SIZE*3*NHITS+SCORE_SIZE,0,0,0> Track_t;
typedef ap_axiu<HIT_SIZE,0,0,0> stream_t;
typedef ap_axiu<3,0,0,0> done_t;

struct Hit{
  data_t x = 0;
  data_t y = 0;
  data_t z = 0;
};

struct Track{
  hls::vector<Hit, NHITS> hits;
  nnscore_t NNScore;
};



// typedef ap_fixed<16,6> data_t;

enum COMPARISON{BOTH, TRKA, TRKB};

// extern "C"{

// void runner(Track* inputTracks, Track* outTracks, int min_dist, int max_shared);

// }

#endif