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
int compare(data_t* trkA, data_t* tmp, int j, nnscore_t nnA, nnscore_t nnB, int max_shared){
  // #pragma HLS INLINE off
  // #pragma HLS PIPELINE
  // #pragma HLS UNROLL
  int nShared = 0;
  int ii = 0;
  int jj = 0;
  INNERLOOP:
  for(ii = 0; ii < NHITS; ii++){
    // #pragma HLS PIPELINE
    // #pragma HLS UNROLL
    // #pragma HLS UNROLL off
    data_t x1 = trkA[ii*NPARS + 0];
    data_t y1 = trkA[ii*NPARS + 1];
    data_t z1 = trkA[ii*NPARS + 2];
    INNER2LOOP:
    for(jj = 0; jj < NHITS; jj++){
      // #pragma HLS PIPELINE
      // #pragma HLS UNROLL
      // #pragma HLS UNROLL off
      if(x1 == 0) // can just check one of them probably... 
        continue;
      
      data_t x2 = tmp[j*NHITS*NPARS + jj*NPARS + 0];
      data_t y2 = tmp[j*NHITS*NPARS + jj*NPARS + 1];
      data_t z2 = tmp[j*NHITS*NPARS + jj*NPARS + 2];
      
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


void searchHit(data_t* inTracks, nnscore_t* in_nn, nnscore_t* out_nn, int max_shared, int num_tracks){
  #pragma HLS INLINE off
  // #pragma HLS PIPELINE off
  // #pragma HLS PIPELINE


  OUTER:
  for(int i = 0; i < MAX_TRACK_SIZE; i++){
    #pragma HLS PIPELINE
    // #pragma HLS UNROLL off // doesnt seem to have a difference? probably default PIPELINE
    // #pragma HLS PIPELINE off

    if(i > num_tracks)
      break;

    nnscore_t nnA = in_nn[i];
    nnscore_t final_nn = nnA;

    data_t trkA[NHITS * NPARS];
    #pragma HLS ARRAY_PARTITION variable=trkA complete
    TRKA:
    for(int j = 0; j < NHITS * NPARS; j++){
      trkA[j] = inTracks[i*NHITS*NPARS + j];
    }

    INNER:
    for(int j = 0; j < MAX_TRACK_SIZE; j++){
      // #pragma HLS UNROLL off
      #pragma HLS PIPELINE
      // #pragma HLS PIPELINE off
      nnscore_t nnB = in_nn[j];
      if(nnB < nnscore_t(0)){
        continue;
      }
      if(j > num_tracks)
        break;
      if(j == i)
        continue;

      int cmpr = compare(trkA, inTracks, j, nnA, nnB, max_shared);

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

// void readData(Track* inputTracks, data_t* inTracks_copy, nnscore_t* inTracks_nn){
void readData(Track* inputTracks, data_t* inTracks_copy){
  for(int i = 0; i < MAX_TRACK_SIZE; i++){
    #pragma HLS PIPELINE
    Track trk = inputTracks[i];
    for(int j=0; j < NHITS; j++){
      // #pragma HLS UNROLL
      inTracks_copy[i*NHITS*NPARS + j*NPARS + 0] = trk.hits[j].x;
      inTracks_copy[i*NHITS*NPARS + j*NPARS + 1] = trk.hits[j].y;
      inTracks_copy[i*NHITS*NPARS + j*NPARS + 2] = trk.hits[j].z;
    }
  }
}

void writeData(Track* outTracks, nnscore_t* outTracks_nn){
  for(int i = 0; i < MAX_TRACK_SIZE; i++){
    #pragma HLS PIPELINE
    nnscore_t nn = outTracks_nn[i];
    // Track trk = inTracks_STRUCT[i];
    // if(nn > nnscore_t(0))
    //   continue;
    // trk.NNScore = nn;
    // outTracks[i] = trk;
    outTracks[i].NNScore = nn;
  }
}


// void runner(Track* inTracks, int min_dist, int max_shared){
void runner(Track* inTracks, int max_shared, int num_tracks){
  // #pragma HLS PIPELINE
  // #pragma HLS DATAFLOW
  #pragma HLS INTERFACE mode=m_axi port=inTracks offset=slave bundle=aximm1
  // #pragma HLS INTERFACE mode=m_axi port=outTracks offset=slave bundle=aximm2
  // #pragma HLS STABLE variable=min_dist
  #pragma HLS STABLE variable=max_shared
  #pragma HLS STABLE variable=num_tracks
  
  data_t inTracks_copy[MAX_TRACK_SIZE * NHITS * NPARS];
  #pragma HLS ARRAY_PARTITION variable=inTracks_copy complete

  // Track inTracks_STRUCT[MAX_TRACK_SIZE];
  // #pragma HLS ARRAY_PARTITION variable=inTracks_STRUCT complete

  // nnscore_t inTracks_nn[MAX_TRACK_SIZE];
  nnscore_t midTracks_nn[MAX_TRACK_SIZE];
  nnscore_t outTracks_nn[MAX_TRACK_SIZE];
  // #pragma HLS ARRAY_PARTITION variable=inTracks_nn complete
  #pragma HLS ARRAY_PARTITION variable=midTracks_nn complete
  #pragma HLS ARRAY_PARTITION variable=outTracks_nn complete

  // int min_dist_read = min_dist;
  // int max_shared_read = max_shared;

  // readData(inTracks, inTracks_copy, inTracks_nn);
  readData(inTracks, inTracks_copy);

  for(int i = 0; i < MAX_TRACK_SIZE; i++){
    #pragma HLS PIPELINE
    // #pragma HLS PIPELINE off
    // #pragma HLS UNROLL off
    if(i > num_tracks)
      break;

    input_t inLayer[N_INPUT_1_1];
    #pragma HLS ARRAY_PARTITION var=inLayer complete
    for(int j = 0; j < NHITS * NPARS; j++){
      #pragma HLS PIPELINE
      inLayer[j] = inTracks_copy[i*NHITS*NPARS + j];
    }
    nnscore_t nn = 0;
    runScoringNetwork(inLayer, &nn);
    filterLowNN(&nn);
    midTracks_nn[i] = nn;
  }

  searchHit(inTracks_copy, midTracks_nn, outTracks_nn, max_shared, num_tracks);

  writeData(inTracks, outTracks_nn);
}


}