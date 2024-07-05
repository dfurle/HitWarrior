#include "projectDefines.h"

// #include <iostream>

#include "nnscore_kernel/myproject.h"

#define RUN_PIPELINE


extern "C"{


void runScoringNetwork(data_t* inTracks, nnscore_t* outScore){
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    #pragma HLS PIPELINE
    input_t inLayer[N_INPUT_1_1];
    result_t outLayer[N_LAYER_8];
    #pragma HLS ARRAY_PARITION var=inLayer complete
    #pragma HLS ARRAY_PARITION var=outLayer complete
    for(int j = 0; j < NHITS * NPARS; j++){
      inLayer[j] = inTracks[i*NHITS*NPARS + j];
    }
    myproject(inLayer, outLayer);
    outScore[i] = outLayer[0];
  }
}

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
    INNER2LOOP:
    for(jj = 0; jj < NHITS; jj++){
      #pragma HLS UNROLL
      int distx = x1 - tmp[j*NHITS*NPARS + jj*NPARS + 0];
      int disty = y1 - tmp[j*NHITS*NPARS + jj*NPARS + 1];
      int distz = z1 - tmp[j*NHITS*NPARS + jj*NPARS + 2];

      int dist = distx*distx + disty*disty + distz*distz;
      if(x1 == 0 && y1 == 0 && z1 == 0) // can just check one of them probably... 
        continue;
      if(dist < data_t(min_dist)){
        nShared++;
      }
    }
  }

  if(nShared >= max_shared){
    if(nnA == nnB){
      return COMPARISON::EQUAL;
    } else if(nnA > nnB){ // choose to keep track1 if nnscores exactly equal
      return COMPARISON::TRKA; // trkA survives
    } else {
      return COMPARISON::TRKB; // trkB survives
    }
  } else {
    return COMPARISON::BOTH; // both survive
  }
}

void searchHit(data_t* inTracks, nnscore_t* in_nn, nnscore_t* out_nn, int min_dist, int max_shared){
  // #pragma HLS INLINE off
  // #pragma HLS PIPELINE off
  // #pragma HLS PIPELINE

  OUTER:
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    // #pragma HLS UNROLL off
    // #pragma HLS PIPELINE off
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
      // #pragma HLS PIPELINE off
      nnscore_t nnB = in_nn[j];
      if(nnB < nnscore_t(0)){
        continue;
      }
      int cmpr = compare(trkA, inTracks, j, nnA, nnB, min_dist, max_shared);

      // if((i == 260 && j == 346) || (j == 260 && i == 346)){
      //   printf("%d  %.2f %.2f %.2f | %d \n", i, float(trkA[0]), float(trkA[1]), float(trkA[2]), j);
      //   printf(">%d\n", cmpr);
      // }
      if(j == i)
        continue;
      if(cmpr == COMPARISON::EQUAL){ // choose the lower index to use
        if(i > j)
          final_nn = -0.5;
      } else if(cmpr == COMPARISON::TRKA){

      } else if(cmpr == COMPARISON::TRKB){
        final_nn = -0.5;
      } else {

      }
    }
    if(nnA < nnscore_t(0)){
      out_nn[i] = -0.5;
    } else {
      out_nn[i] = final_nn;
    }
  }
}

void readData(Track* inputTracks, data_t* inTracks_copy, nnscore_t* inTracks_nn){
  READ:
  for(int i=0; i < INPUTTRACKSIZE; i++) {
    #pragma HLS UNROLL
    Track trk = inputTracks[i];
    for(int j=0; j < NHITS; j++){
      // #pragma HLS UNROLL
      inTracks_copy[i*NHITS*NPARS + j*NPARS + 0] = trk.hits[j].x;
      inTracks_copy[i*NHITS*NPARS + j*NPARS + 1] = trk.hits[j].y;
      inTracks_copy[i*NHITS*NPARS + j*NPARS + 2] = trk.hits[j].z;
    }
    // inTracks_nn[i] = trk.NNScore; // ignore this for now
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
  #pragma HLS DATAFLOW
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

  int min_dist_read = min_dist;
  int max_shared_read = max_shared;

  // readData(inTracks, inTracks_copy, inTracks_STRUCT, inTracks_nn);
  readData(inTracks, inTracks_copy, inTracks_nn);

  runScoringNetwork(inTracks_copy, inTracks_nn);

  filterLowNN(inTracks_nn, midTracks_nn);
  // searchHit(inTracks_copy, indexDelete, midTracks_nn, outTracks_nn, min_dist_read, max_shared_read);
  searchHit(inTracks_copy, midTracks_nn, outTracks_nn, min_dist_read, max_shared_read);

  // writeData(outTracks, inTracks_STRUCT, outTracks_nn);
  writeData(inTracks, outTracks_nn);

}


}