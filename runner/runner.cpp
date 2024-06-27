#include "projectDefines.h"

// #include <iostream>

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
int compare(data_t* trkA, data_t* tmp, int j, nnscore_t nnA, nnscore_t nnB, int min_dist, int max_shared){
// int compare(data_t* trkA_list, int i, data_t* tmp, int j, nnscore_t nnA, nnscore_t nnB, int min_dist, int max_shared){
  #pragma HLS INLINE off
  // #pragma HLS PIPELINE
  // #pragma HLS UNROLL
  int nShared = 0;
  int ii = 0;
  int jj = 0;
  INNERLOOP:
  for(ii = 0; ii < NHITS; ii++){
    #pragma HLS UNROLL
    data_t x1 = trkA[ii*NPARS + 0];
    data_t y1 = trkA[ii*NPARS + 1];
    data_t z1 = trkA[ii*NPARS + 2];
    // data_t x1 = trkA_list[i*NHITS*NPARS + ii*NPARS + 0];
    // data_t y1 = trkA_list[i*NHITS*NPARS + ii*NPARS + 1];
    // data_t z1 = trkA_list[i*NHITS*NPARS + ii*NPARS + 2];
    INNER2LOOP:
    for(jj = 0; jj < NHITS; jj++){
      #pragma HLS UNROLL

      int distx = x1 - tmp[j*NHITS*NPARS + jj*NPARS + 0];
      int disty = y1 - tmp[j*NHITS*NPARS + jj*NPARS + 1];
      int distz = z1 - tmp[j*NHITS*NPARS + jj*NPARS + 2];
      // int distx = x1 - tmp[i*INPUTTRACKSIZE*NHITS*NPARS + j*NHITS*NPARS + jj*NPARS + 0];
      // int disty = y1 - tmp[i*INPUTTRACKSIZE*NHITS*NPARS + j*NHITS*NPARS + jj*NPARS + 1];
      // int distz = z1 - tmp[i*INPUTTRACKSIZE*NHITS*NPARS + j*NHITS*NPARS + jj*NPARS + 2];

      int dist = distx*distx + disty*disty + distz*distz;
      if(dist < data_t(min_dist)){
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

// void searchHit(data_t* inTracks, int* indexDelete, nnscore_t* in_nn, nnscore_t* out_nn, int min_dist, int max_shared){
void searchHit(data_t* inTracks, nnscore_t* in_nn, nnscore_t* out_nn, int min_dist, int max_shared){
  #pragma HLS INLINE off
  #pragma HLS PIPELINE off
  // #pragma HLS PIPELINE

  OUTER:
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    // #pragma HLS UNROLL off
    #pragma HLS PIPELINE off
    nnscore_t nnA = in_nn[i];
    nnscore_t final_nn = nnA;

    data_t trkA[NHITS * NPARS];
    #pragma HLS ARRAY_PARTITION variable=trkA complete
    TRKA:
    for(int j = 0; j < NHITS * NPARS; j++){
      trkA[j] = inTracks[i*NHITS*NPARS + j];
    }

    INNER:
    for(int j = 0; j < INPUTTRACKSIZE; j++){
      // #pragma HLS UNROLL off
      #pragma HLS PIPELINE off
      nnscore_t nnB = in_nn[j];
      if(nnB < nnscore_t(0)){
        continue;
      }
      int cmpr = compare(trkA, inTracks, j, nnA, nnB, min_dist, max_shared);

      if(j == i)
        continue;
      if(cmpr == COMPARISON::TRKA){
      } else if(cmpr == COMPARISON::TRKB){
        final_nn = -0.5;
      } else {

      }
    }

    if(nnA < nnscore_t(0)){
      out_nn[i] = -0.5;
      continue;
    } else {
      out_nn[i] = final_nn;
    }
    // printf("\n");
  }


  // // WRITEOUT:{
  // //   #pragma HLS protocol fixed
  //   WRITEOUTPUT:
  //   for(int i = 0; i < INPUTTRACKSIZE; i++){
  //     // #pragma HLS UNROLL
  //     #pragma HLS PIPELINE
  //     nnscore_t nn = in_nn[i];
  //     // printf("del %d = %d\n",i, indexDelete[i]);
  //     if(indexDelete[i] == 1){
  //       out_nn[i] = -0.5;
  //     } else {
  //       out_nn[i] = nn;
  //     }
  //   }
  // // }

}

void readData(Track* inputTracks, data_t* inTracks_copy, nnscore_t* inTracks_nn){
  READ:
  for(int i=0; i < INPUTTRACKSIZE; i++) {
    #pragma HLS UNROLL
    Track trk = inputTracks[i];
    // inTracks_STRUCT[i] = trk;
    for(int j=0; j < NHITS; j++){
      // #pragma HLS UNROLL
      inTracks_copy[i*NHITS*NPARS + j*NPARS + 0] = trk.hits[j].x;
      inTracks_copy[i*NHITS*NPARS + j*NPARS + 1] = trk.hits[j].y;
      inTracks_copy[i*NHITS*NPARS + j*NPARS + 2] = trk.hits[j].z;
    }
    inTracks_nn[i] = trk.NNScore;
  }
}

void writeData(Track* outTracks, nnscore_t* outTracks_nn){
  WRITE:
  for(int i=0; i < INPUTTRACKSIZE; i++) {
    #pragma HLS UNROLL
    nnscore_t nn = outTracks_nn[i];
    // Track trk = inTracks_STRUCT[i];
    // if(nn > nnscore_t(0))
    //   continue;
    // trk.NNScore = nn;
    // outTracks[i] = trk;
    outTracks[i].NNScore = nn;
  }
}


void runner(Track* inTracks, int min_dist, int max_shared){
  // #pragma HLS PIPELINE
  // #pragma HLS DATAFLOW
  // #pragma HLS INTERFACE mode=m_axi port=inputTracks offset=slave max_read_burst_length= bundle=aximm1
  #pragma HLS INTERFACE mode=m_axi port=inTracks offset=slave bundle=aximm1
  // #pragma HLS INTERFACE mode=m_axi port=outTracks offset=slave bundle=aximm2
  #pragma HLS STABLE variable=min_dist
  #pragma HLS STABLE variable=max_shared
  

  data_t inTracks_copy[INPUTTRACKSIZE * NHITS * NPARS];
  #pragma HLS ARRAY_PARTITION variable=inTracks_copy complete

  // Track inTracks_STRUCT[INPUTTRACKSIZE];
  // #pragma HLS ARRAY_PARTITION variable=inTracks_STRUCT complete

  nnscore_t inTracks_nn[INPUTTRACKSIZE];
  nnscore_t midTracks_nn[INPUTTRACKSIZE];
  nnscore_t outTracks_nn[INPUTTRACKSIZE];
  #pragma HLS ARRAY_PARTITION variable=inTracks_nn complete
  #pragma HLS ARRAY_PARTITION variable=midTracks_nn complete
  #pragma HLS ARRAY_PARTITION variable=outTracks_nn complete

  // having this here and passing in, doesnt remove the violations but increases latency and decreases resources
  // int indexDelete[INPUTTRACKSIZE];
  // #pragma HLS ARRAY_PARTITION variable=indexDelete complete

  int min_dist_read = min_dist;
  int max_shared_read = max_shared;

  // readData(inTracks, inTracks_copy, inTracks_STRUCT, inTracks_nn);
  readData(inTracks, inTracks_copy, inTracks_nn);

  filterLowNN(inTracks_nn, midTracks_nn);
  // searchHit(inTracks_copy, indexDelete, midTracks_nn, outTracks_nn, min_dist_read, max_shared_read);
  searchHit(inTracks_copy, midTracks_nn, outTracks_nn, min_dist_read, max_shared_read);

  // writeData(outTracks, inTracks_STRUCT, outTracks_nn);
  writeData(inTracks, outTracks_nn);

}


}