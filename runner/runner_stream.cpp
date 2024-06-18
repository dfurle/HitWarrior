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


void filterLowNN(Track* inTracks, Track* outTracks){
  // #pragma HLS interface ap_ctrl_none port=return
  // #pragma HLS PIPELINE
  uintS_t counter = 0;
  uintS_t i = 0;
  MAINLOOP:
  for(i = 0; i < INPUTTRACKSIZE; i++){
    // #pragma HLS PIPELINE II=20
    Track trk = inTracks[i];
    if(trk.NNScore >= MIN_THRESHOLD){
      outTracks[counter++] = inTracks[i];
    }
  }
  if(counter < INPUTTRACKSIZE){
    // Track trk = outTracks[counter];
    // trk.NNScore = -0.5;
    // outTracks[counter] = trk;
    outTracks[counter].NNScore = -0.5;
  }
  // SECONDARY:
  // for(i = counter; i < intS_t(INPUTTRACKSIZE); i++){
  //   outTracks[i].NNScore = -0.5;
  // }
}

// returns TRKA if trkA survives or TRKB or BOTH 
int compare(Track trkA, Track trkB, intS_t min_dist, intS_t max_shared){
  uintS_t nShared = 0;
  uintS_t ii = 0;
  uintS_t jj = 0;
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


#define USE_STREAM


#ifdef USE_STREAM
void searchHit(Track* inTracks, Track* outTracks, int min_dist, int max_shared){
  // #pragma HLS PIPELINE
  hls::stream<Track> tmp;
  uint i = 0;
  uint j = 0;
  uint tmpsize = 0;
  intB_t flagOverlap = 0;

  OUTER:
  for(i = uintS_t(0); i < intS_t(INPUTTRACKSIZE); i++){
    #pragma HLS INLINE off
    Track trkA = inTracks[i];
    if(trkA.NNScore < 0){
      break;
    }
    tmpsize = tmp.size();
    flagOverlap = 0;
    INNER:
    for(j = uintS_t(0); j < tmpsize; j++){
      #pragma HLS INLINE off
      // #pragma HLS UNROLL
      Track trkB = tmp.read();
      int cmpr = compare(trkA, trkB, min_dist, max_shared);
      if(cmpr == COMPARISON::TRKA){
        // do nothing, check rest of em
      } else if(cmpr == COMPARISON::TRKB){
        tmp.write(trkB);
        flagOverlap = 1; // dont add trkA at the end
        // printf("trkB: *%d : %d %d %d\n", i, int(trkB.hits[0].x), int(trkB.hits[0].y), int(trkB.hits[0].z));
      } else {
        tmp.write(trkB);
        // printf("writeback: *%d : %d %d %d\n", i, int(trkB.hits[0].x), int(trkB.hits[0].y), int(trkB.hits[0].z));
      }
    }
    if(flagOverlap == intB_t(0)){
      tmp.write(trkA);
      printf("flagOverlap: %d : %d %d %d\n", i, int(trkA.hits[0].x), int(trkA.hits[0].y), int(trkA.hits[0].z));
    }
  }


  // int tmpsize = tmp.size();
  // for(int i = 0; i < tmpsize; i++){
  int counter = 0;
  FINAL:
  while(tmp.size() > 0){
    // #pragma HLS PIPELINE
    Track trk = tmp.read();
    outTracks[counter++] = trk;
  }
}


#else


void searchHit(Track* inTracks, Track* outTracks, int min_dist, int max_shared){
  // #pragma HLS PIPELINE

  OUTER:
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    #pragma HLS PIPELINE
    Track trkA = inTracks[i];
    if(trkA.NNScore < 0){
      if(trkA.NNScore < -0.5) // reached end of trimmed list
        break;
      continue;
      // break;
    }
    INNER:
    // for(int j = i+1; j < INPUTTRACKSIZE; j++){
    for(int j = i+1; j < INPUTTRACKSIZE; j++){
      if(i == j)
        continue;
      // #pragma HLS PIPELINE
      Track trkB = inTracks[j];
      if(trkB.NNScore < 0){
        if(trkB.NNScore < -0.5) // reached end of trimmed list
          break;
        continue;
        // break;
      }
      int cmpr = compare(trkA, trkB, min_dist, max_shared);
      if(cmpr == COMPARISON::TRKA){
        inTracks[j].NNScore = -0.2;
        // printf("Remove TRK B\n");
      } else if(cmpr == COMPARISON::TRKB){
        inTracks[i].NNScore = -0.2;
        // printf("Remove TRK A\n");
      } else {
        
      }
    }
  }


  int counter = 0;
  FINAL:
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    #pragma HLS PIPELINE
    Track trk = inTracks[i];
    // if(trk.NNScore < 0){
    if(trk.NNScore < 0){ // reached end of trimmed list
      continue;
    }
    outTracks[counter++] = trk;
  }
}

#endif

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
  for(int i=0; i < INPUTTRACKSIZE; i++) {
    #pragma HLS UNROLL
    // #pragma HLS PIPELINE
    outTracks[i] = outTracks_copy[i];
  }

  // ---=== otherwise just run something like this ===---
  // searchHit(inputHistList, searchHitList, outList);

}


}