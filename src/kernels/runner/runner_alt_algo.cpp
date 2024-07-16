#include "projectDefines.h"

// #include <iostream>

#include "nnscore_kernel/myproject.h"

#define RUN_PIPELINE


extern "C"{


void runScoringNetwork(input_t* inLayer, nnscore_t* out){
  #pragma HLS PIPELINE
  result_t outLayer[N_LAYER_8];
  #pragma HLS ARRAY_PARTITION var=outLayer complete
  // for(int j = 0; j < NHITS * NPARS; j++){
  //   inLayer[j] = inTracks[i*NHITS*NPARS + j];
  // }
  myproject(inLayer, outLayer);
  *out = nnscore_t(outLayer[0]);
}

void filterLowNN(nnscore_t* in_nn){
  #pragma HLS PIPELINE
  if(*in_nn < MIN_THRESHOLD){
    *in_nn = -0.5; // flag to not use this in overlap checking
  }
}

// returns TRKA if trkA survives or TRKB or BOTH
int compare(Track trkA, Track* inTracks, int j, nnscore_t nnA, nnscore_t nnB, int max_shared){
  int nShared = 0;
  int ii = 0;
  int jj = 0;
  INNERLOOP:
  for(ii = 0; ii < NHITS; ii++){
    data_t x1 = trkA.hits[ii].x;
    data_t y1 = trkA.hits[ii].y;
    data_t z1 = trkA.hits[ii].z;
    INNER2LOOP:
    for(jj = 0; jj < NHITS; jj++){
      if(x1 == 0) // can just check one of them probably... 
        continue;
      
      data_t x2 = inTracks[j].hits[jj].x;
      data_t y2 = inTracks[j].hits[jj].y;
      data_t z2 = inTracks[j].hits[jj].z;

      if(x1 == x2 && y1 == y2 && z1 == z2)
        nShared++;


      // int distx = x1 - tmp[j*NHITS*NPARS + jj*NPARS + 0];
      // int disty = y1 - tmp[j*NHITS*NPARS + jj*NPARS + 1];
      // int distz = z1 - tmp[j*NHITS*NPARS + jj*NPARS + 2];

      // int dist = distx*distx + disty*disty + distz*distz;
      // if(x1 == 0 && y1 == 0 && z1 == 0) // can just check one of them probably... 
      //   continue;
      // if(dist < data_t(min_dist)){
      //   nShared++;
      // }
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


void searchHit(Track trkA, nnscore_t nnA, int i, Track* inTracks, nnscore_t* in_nn, nnscore_t* out_nn, int max_shared){
  #pragma HLS INLINE off

  nnscore_t final_nn = nnA;

  INNER:
  for(int j = 0; j < MAX_TRACK_SIZE; j++){
    #pragma HLS PIPELINE

    if(j >= i)
      break;

    nnscore_t nnB = in_nn[j];
    if(nnB < nnscore_t(0)){
      continue;
    }

    int cmpr = compare(trkA, inTracks, j, nnA, nnB, max_shared);

    if(cmpr == COMPARISON::EQUAL){
      out_nn[i] = -0.5; // remove trkA and break
      break;
    } else if(cmpr == COMPARISON::TRKA){
      out_nn[j] = -0.5; // remove trkB and continue
    } else if(cmpr == COMPARISON::TRKB){
      out_nn[i] = -0.5; // remove trkA and break
      break;
    } else {
      // COMPARISON::BOTH; not intersecting
    }
  }
}

void runner(Track* inTracks, Track* outTracks, int max_shared, int num_tracks){
  // #pragma HLS PIPELINE
  // #pragma HLS DATAFLOW
  #pragma HLS INTERFACE mode=m_axi port=inTracks offset=slave bundle=aximm1
  #pragma HLS INTERFACE mode=m_axi port=outTracks offset=slave bundle=aximm2
  #pragma HLS STABLE variable=max_shared
  #pragma HLS STABLE variable=num_tracks
  
  Track inTracks_copy[MAX_TRACK_SIZE];
  #pragma HLS ARRAY_PARTITION variable=inTracks_copy complete

  nnscore_t midTracks_nn[MAX_TRACK_SIZE];
  nnscore_t outTracks_nn[MAX_TRACK_SIZE];
  #pragma HLS ARRAY_PARTITION variable=midTracks_nn complete
  #pragma HLS ARRAY_PARTITION variable=outTracks_nn complete


  for(int i = 0; i < MAX_TRACK_SIZE; i++){
    #pragma HLS PIPELINE
    // #pragma HLS PIPELINE off
    // #pragma HLS UNROLL off
    if(i > num_tracks)
      break;

    // readData(inTracks, inTracks_copy);
    Track trk = inTracks[i];
    inTracks_copy[i] = trk;

    input_t inLayer[N_INPUT_1_1];
    #pragma HLS ARRAY_PARTITION var=inLayer complete
    for(int j = 0; j < NHITS; j++){
      inLayer[j*NPARS + 0] = trk.hits[j].x;
      inLayer[j*NPARS + 1] = trk.hits[j].y;
      inLayer[j*NPARS + 2] = trk.hits[j].z;
    }
    nnscore_t nn = 0;
    runScoringNetwork(inLayer, &nn);
    filterLowNN(&nn);
    midTracks_nn[i] = nn;

    searchHit(trk, nn, i, inTracks_copy, midTracks_nn, outTracks_nn, max_shared);
  }

  for(int i = 0; i < MAX_TRACK_SIZE; i++){
    #pragma HLS PIPELINE
    Track trk = inTracks[i];
    nnscore_t nn = outTracks_nn[i];
    trk.NNScore = nn;
    outTracks[i] = trk;
  }

}


}