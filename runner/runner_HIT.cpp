#include "projectDefines.h"

#include <iostream>

#define RUN_PIPELINE




/*

INFO: [HLS 214-178] Inlining function 'std::array<Hit, 5ul>::array() (.26.35.47.59.71)' into 'hls::vector<Hit, 5ul>::vector() (.17.23.32.44.56.68)' (/HDD/Xilinx/Vitis_HLS/2023.2/common/technology/autopilot/hls_vector.h:109:0)
INFO: [HLS 214-178] Inlining function 'hls::vector<Hit, 5ul>::vector() (.17.23.32.44.56.68)' into 'Track::Track() (.144)' (/home/denis/UBUNTU_TEST/HitWarrior/projectDefines.h:40:0)
INFO: [HLS 214-178] Inlining function 'hls::vector<Hit, 5ul>::vector() (.17.23.32.44.56.68)' into 'Track::Track()' (/home/denis/UBUNTU_TEST/HitWarrior/projectDefines.h:40:0)
INFO: [HLS 214-178] Inlining function 'std::__array_traits<Hit, 5ul>::_S_ref(Hit const (&) [5], unsigned long)' into 'std::array<Hit, 5ul>::operator[](unsigned long)' (/HDD/Xilinx/Vitis_HLS/2023.2/tps/lnx64/gcc-8.3.0/lib/gcc/x86_64-pc-linux-gnu/8.3.0/../../../../include/c++/8.3.0/array:186:0)
INFO: [HLS 214-178] Inlining function 'std::array<Hit, 5ul>::operator[](unsigned long)' into 'compare' (/home/denis/UBUNTU_TEST/HitWarrior/runner/runner.cpp:37:0)
INFO: [HLS 214-178] Inlining function 'Track::Track()' into 'searchHit' (/home/denis/UBUNTU_TEST/HitWarrior/runner/runner.cpp:78:0)
INFO: [HLS 214-178] Inlining function 'std::array<Hit, 5ul>::operator[](unsigned long)' into 'searchHit' (/home/denis/UBUNTU_TEST/HitWarrior/runner/runner.cpp:78:0)
INFO: [HLS 214-178] Inlining function 'Track::Track() (.144)' into 'runner' (/home/denis/UBUNTU_TEST/HitWarrior/runner/runner.cpp:187:0)

*/



extern "C"{


void filterLowNN(nnscore_t* in_nn, nnscore_t* out_nn){
  #pragma HLS PIPELINE
  MAINLOOP:
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    nnscore_t nn = in_nn[i];
    if(nn < MIN_THRESHOLD){
      nn = -0.5; // flag to not use this in overlap checking
    }
    out_nn[i] = nn;
  }
}

// returns TRKA if trkA survives or TRKB or BOTH
int compare(Hit* trkA, Hit* trkB, nnscore_t nnA, nnscore_t nnB, int min_dist, int max_shared){
  // #pragma HLS INLINE off
  #pragma HLS PIPELINE
  int nShared = 0;
  int ii = 0;
  int jj = 0;
  INNERLOOP:
  for(ii = 0; ii < NHITS; ii++){
    // #pragma HLS UNROLL
    Hit hit1 = trkA[ii];
    INNER2LOOP:
    for(jj = 0; jj < NHITS; jj++){
      // #pragma HLS UNROLL
      Hit hit2 = trkB[jj];

      int distx = hit1.x - hit2.x;
      int disty = hit1.y - hit2.y;
      int distz = hit1.z - hit2.z;

      int dist = distx*distx + disty*disty + distz*distz;
      if(dist < min_dist){
        nShared++;
      }
    }
  }

  if(nShared >= max_shared){
    if(nnA >= nnB){ // choose to keep track1 if exactly equal
    // if(trkA[0].x >= trkB[0].x){ // choose to keep track1 if exactly equal
      return COMPARISON::TRKA; // trkA survives
    } else {
      return COMPARISON::TRKB; // trkB survives
    }
  } else {
    return COMPARISON::BOTH; // both survive
  }
}

void searchHit(Hit* inTracks, nnscore_t* in_nn, nnscore_t* out_nn, int min_dist, int max_shared){
  // #pragma HLS INLINE off
  // #pragma HLS PIPELINE

  for(int i = 0; i < INPUTTRACKSIZE; i++){
    nnscore_t nn = in_nn[i];
    out_nn[i] = nn;
  }

  OUTER:
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    #pragma HLS INLINE off
    Hit trkA[NHITS];
    nnscore_t nnA = in_nn[i];
    if(nnA < nnscore_t(0))
      continue;
    #pragma HLS ARRAY_PARTITION variable=trkA complete
    for(int h = 0; h < NHITS; h++){
      trkA[h] = inTracks[i*NHITS + h];
    }
    INNER:
    for(int j = 0; j < INPUTTRACKSIZE; j++){
      #pragma HLS INLINE off
      // #pragma HLS UNROLL
      if(j <= i)
        continue;
      Hit trkB[NHITS];
      nnscore_t nnB = in_nn[j];
      if(nnB < nnscore_t(0))
        continue;
      #pragma HLS ARRAY_PARTITION variable=trkB complete
      for(int h = 0; h < NHITS; h++){
        trkB[h] = inTracks[j*NHITS + h];
      }
      int cmpr = compare(trkA, trkB, nnA, nnB, min_dist, max_shared);
      if(cmpr == COMPARISON::TRKA){
        nnB = -0.5; // flag to not use anymore
        // printf("remove: i_ %d\n", j);
      } else if(cmpr == COMPARISON::TRKB){
        nnA = -0.5; // flag to not use anymore
        // printf("remove: _j %d\n", i);
      } else {
        
      }
      out_nn[j] = nnB;
    }
    out_nn[i] = nnA;
  }
}

void readData(Track* inputTracks, Hit* inTracks_copy, nnscore_t* inTracks_nn){
  READ:
  for(int i=0; i < INPUTTRACKSIZE; i++) {
    #pragma HLS UNROLL
    Track trk = inputTracks[i];
    for(int j=0; j < NHITS; j++){
      inTracks_copy[i*NHITS + j] = trk.hits[j];
    }
    inTracks_nn[i] = trk.NNScore;
  }
}

void writeData(Track* outTracks, Hit* inTracks_copy, nnscore_t* outTracks_nn){
  WRITE:
  int counter = 0;
  for(int i=0; i < INPUTTRACKSIZE; i++) {
    #pragma HLS UNROLL
    nnscore_t nn = outTracks_nn[i];
    if(nn < nnscore_t(0))
      continue;
    Track trk;
    for(int j=0; j < NHITS; j++){
      trk.hits[j] = inTracks_copy[i*NHITS + j];
    }
    trk.NNScore = nn;
    // printf("trk: %7.2f %7.2f %7.2f | %7.3f\n", float(trk.hits[0].x), float(trk.hits[0].y), float(trk.hits[0].z), float(trk.NNScore));
    outTracks[counter] = trk;
    counter++;
  }
}


void runner(Track* inputTracks, Track* outTracks, int min_dist, int max_shared){
  // #pragma HLS PIPELINE
  #pragma HLS DATAFLOW
  // #pragma HLS INTERFACE mode=m_axi port=inputTracks offset=slave max_read_burst_length= bundle=aximm1
  #pragma HLS INTERFACE mode=m_axi port=inputTracks offset=slave bundle=aximm1
  #pragma HLS INTERFACE mode=m_axi port=outTracks offset=slave bundle=aximm2
  #pragma HLS STABLE variable=min_dist
  #pragma HLS STABLE variable=max_shared
  

  // #pragma HLS INTERFACE ap_ctrl_none port=return


  // // ---=== might be better for certain situations, but will limit maximum size ===---
  Hit inTracks_copy[INPUTTRACKSIZE * NHITS];
  #pragma HLS ARRAY_PARTITION variable=inTracks_copy complete

  nnscore_t inTracks_nn[INPUTTRACKSIZE];
  nnscore_t midTracks_nn[INPUTTRACKSIZE];
  nnscore_t outTracks_nn[INPUTTRACKSIZE];
  #pragma HLS ARRAY_PARTITION variable=inTracks_nn complete
  #pragma HLS ARRAY_PARTITION variable=midTracks_nn complete
  #pragma HLS ARRAY_PARTITION variable=outTracks_nn complete

  int min_dist_read = min_dist;
  int max_shared_read = max_shared;

  readData(inputTracks, inTracks_copy, inTracks_nn);

  filterLowNN(inTracks_nn, midTracks_nn);
  searchHit(inTracks_copy, midTracks_nn, outTracks_nn, min_dist_read, max_shared_read);

  writeData(outTracks, inTracks_copy, outTracks_nn);

}


}