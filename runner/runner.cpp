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


void filterLowNN(Track inTracks[INPUTTRACKSIZE], Track outTracks[INPUTTRACKSIZE]){
  #pragma HLS PIPELINE
  MAINLOOP:
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    Track trk = inTracks[i];
    if(trk.NNScore >= MIN_THRESHOLD){
      outTracks[i] = trk;
    } else {
      trk.NNScore = -0.5; // flag to not use this in overlap checking
      outTracks[i] = trk;
    }
  }
}

// returns TRKA if trkA survives or TRKB or BOTH 
int compare(Track trkA, Track trkB, int min_dist, int max_shared){
  // #pragma HLS PIPELINE
  int nShared = 0;
  int ii = 0;
  int jj = 0;
  INNERLOOP:
  for(ii = 0; ii < NHITS; ii++){
    // #pragma HLS UNROLL
    Hit hit1 = trkA.hits[ii];
    INNER2LOOP:
    for(jj = 0; jj < NHITS; jj++){
      // #pragma HLS UNROLL
      Hit hit2 = trkB.hits[jj];

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
    if(trkA.NNScore >= trkB.NNScore){ // choose to keep track1 if exactly equal
    // if(trkA[0].x >= trkB[0].x){ // choose to keep track1 if exactly equal
      return COMPARISON::TRKA; // trkA survives
    } else {
      return COMPARISON::TRKB; // trkB survives
    }
  } else {
    return COMPARISON::BOTH; // both survive
  }
}

void loopInner(Track trkA, Track inTracks[INPUTTRACKSIZE], Track tmp[INPUTTRACKSIZE], int i, int min_dist, int max_shared){
  INNER:
  // for(int j = i+1; j < INPUTTRACKSIZE; j++){
  for(int k = 0; k < INPUTTRACKSIZE; k++){
    #pragma HLS UNROLL
    // int j = k + i + 1;
    // if(j >= INPUTTRACKSIZE)
      // break;
    // Track trkB = inTracks[j];
    Track trkB = inTracks[k];
    // if(trkB.NNScore < 0){
    //   continue;
    // }
    int cmpr = compare(trkA, trkB, min_dist, max_shared);
    if(cmpr == COMPARISON::TRKA){
      trkB.NNScore = -0.5; // flag to not use anymore
    } else if(cmpr == COMPARISON::TRKB){
      trkA.NNScore = -0.5; // flag to not use anymore
    } else {
      
    }
    tmp[i] = trkA;
    tmp[k] = trkB;
  }
}


void searchHit(Track inTracks[INPUTTRACKSIZE], Track outTracks[INPUTTRACKSIZE], int min_dist, int max_shared){
  // #pragma HLS PIPELINE

  Track tmp[INPUTTRACKSIZE];
  #pragma HLS ARRAY_PARITITION variable=tmp complete
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    // #pragma HLS PIPELINE
    tmp[i] = inTracks[i];
  }

  OUTER:
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    // #pragma HLS PIPELINE
    Track trkA = inTracks[i];
    // if(trkA.NNScore < 0)
    //   continue;
    // loopInner(trkA, tmp);
    loopInner(trkA, inTracks, tmp, i, min_dist, max_shared);
  }

  // int counter = 0;
  FINAL:
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    // #pragma HLS PIPELINE
    Track trk = inTracks[i];
    // if(trk.NNScore < 0){ // reached end of trimmed list
    //   continue;
    // }
    outTracks[i] = trk;
  }
}


void runner(Track* inputTracks, Track* outTracks, int min_dist, int max_shared){
  #pragma HLS PIPELINE
  // #pragma HLS DATAFLOW
  // #pragma HLS INTERFACE mode=m_axi port=inputTracks offset=slave max_read_burst_length= bundle=aximm1
  #pragma HLS INTERFACE mode=m_axi port=inputTracks offset=slave bundle=aximm1
  #pragma HLS INTERFACE mode=m_axi port=outTracks offset=slave bundle=aximm2
  #pragma HLS STABLE variable=min_dist
  #pragma HLS STABLE variable=max_shared
  

  // #pragma HLS INTERFACE ap_ctrl_none port=return


  // // ---=== might be better for certain situations, but will limit maximum size ===---
  Track inTracks_copy[INPUTTRACKSIZE];
  Track midTracks_copy[INPUTTRACKSIZE];
  Track outTracks_copy[INPUTTRACKSIZE];
  #pragma HLS ARRAY_PARTITION variable=inTracks_copy complete
  #pragma HLS ARRAY_PARTITION variable=midTracks_copy complete
  #pragma HLS ARRAY_PARTITION variable=outTracks_copy complete
  #pragma HLS ARRAY_PARTITION variable=inTracks_copyhits complete
  #pragma HLS ARRAY_PARTITION variable=hits complete
  #pragma HLS ARRAY_PARTITION variable=hits complete


  int min_dist_read = min_dist;
  int max_shared_read = max_shared;

  READ:
  for(int i=0; i < INPUTTRACKSIZE; i++) {
    // #pragma HLS PIPELINE
    #pragma HLS UNROLL
    inTracks_copy[i] = inputTracks[i];
  }

  filterLowNN(inTracks_copy, midTracks_copy);
  searchHit(midTracks_copy, outTracks_copy, min_dist_read, max_shared_read);

  WRITE:
  int counter = 0;
  for(int i=0; i < INPUTTRACKSIZE; i++) {
    #pragma HLS UNROLL
    // #pragma HLS PIPELINE
    Track trk = outTracks_copy[i];
    if(trk.NNScore > 0)
      outTracks[counter++] = trk;
  }

  // ---=== otherwise just run something like this ===---
  // searchHit(inputHistList, searchHitList, outList);

}


}