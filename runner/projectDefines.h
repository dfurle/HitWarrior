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

struct Hit{
  int x = 0;
  int y = 0;
  int z = 0;
};

struct Track{
  hls::vector<Hit, NHITS> hits;
  ap_fixed<16,1> NNScore;
  ap_int<2> flag_delete;
};

// typedef hls::vector<Hit, NHITS> Track;

typedef ap_fixed<16,6> data_t;

extern "C"{

void runner(Track* inputTracks, Track* outTracks, int min_dist, int max_shared);

}

#endif