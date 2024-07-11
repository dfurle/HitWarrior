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
  #pragma HLS PIPELINE
  // #pragma HLS UNROLL
  int nShared = 0;
  int ii = 0;
  int jj = 0;
  INNERLOOP:
  for(ii = 0; ii < NHITS; ii++){
    #pragma HLS PIPELINE
    // #pragma HLS UNROLL
    // #pragma HLS UNROLL off
    data_t x1 = trkA[ii*NPARS + 0];
    data_t y1 = trkA[ii*NPARS + 1];
    data_t z1 = trkA[ii*NPARS + 2];
    INNER2LOOP:
    for(jj = 0; jj < NHITS; jj++){
      #pragma HLS PIPELINE
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

//(cycles)|    (ns)   |  Latency | Interval| Count| Pipelined|   BRAM   |     DSP    |      FF     |      LUT     | URAM|
//  8  218|  1.090e+03|         -|       85|     -|  dataflow|  31 (~0%)|  1994 (16%)|   74397 (2%)|  145086 (8%)|
// 16  251|  1.255e+03|         -|       93|     -|  dataflow|  31 (~0%)|  2594 (21%)|  156480 (4%)|  253562 (14%)|    -|


//(cycles)|    (ns)   |  Latency | Interval| Count| Pipelined|   BRAM   |     DSP    |      FF     |      LUT     | URAM|
// 32 1413|  7.065e+03|         -|      646|     -|  dataflow|  31 (~0%)|  3794 (30%)|  431324 (12%)|  447297 (25%)|    -| // no pipeline, seperate loops
// 32 2480|  1.240e+04|         -|     2359|     -|  dataflow|  17 (~0%)|  3794 (30%)|  433730 (12%)|  483554 (27%)|    -| // pipeline, joint loops
// 32 3073|  1.639e+04|         -|     2359|     -|  dataflow|  17 (~0%)|  3794 (30%)|  449302 (13%)|  448775 (25%)|    -| // pipeline, even more joint loops?
// 32  911|  4.555e+03|         -|      646|     -|  dataflow|  31 (~0%)|  3794 (30%)|  441191 (12%)|  461142 (26%)|    -| // single input to funcs, reading/writing pipelined
// 32  814|  4.070e+03|         -|      549|     -|  dataflow|  31 (~0%)|  1394 (11%)|  467048 (13%)|  305681 (17%)|    -| // switched to check equality, and 2x pipeline rather than 2x unroll in comp
// 32  911|  4.555e+03|         -|      646|     -|  dataflow|  31 (~0%)|  3794 (30%)|  441191 (12%)|  461142 (26%)|    -| // 1st PIPELINE -> 2nd UNROLL
// 32  911|  4.555e+03|         -|      646|     -|  dataflow|  31 (~0%)|  3794 (30%)|  441191 (12%)|  461142 (26%)|    -| // 1st UNROLL -> 2nd PIPELINE
// 32  814|  4.070e+03|         -|      549|     -|  dataflow|  31 (~0%)|  1394 (11%)|  467048 (13%)|  305681 (17%)|    -| // equality check, 2x unroll
// 32  314|  1.570e+03|         -|      109|     -|  dataflow|  31 (~0%)|  1394 (11%)|  444709 (12%)|  340269 (19%)|    -| // remove pipeline from searchHit


//         (cycles)|    (ns)   |  Latency | Interval| Count| Pipelined|   BRAM   |     DSP    |      FF     |      LUT     | URAM|
// 100/30m    13571|  6.786e+04|         -|    13102|     -|  dataflow|  31 (~0%)|  1394 (11%)|  247468 (7%)|  254843 (14%)|    -|
// 100/? m     1373|  6.865e+03|         -|      904|     -|  dataflow|  31 (~0%)|  1394 (11%)|  3759937 (108%)|  1422094 (82%)|    -| // With DATAFLOW 
// 100/2.7m  302670|  1.513e+06|         -|   302671|     -|        no|  31 (~0%)|  1394 (11%)|   68850 (1%)|  102224 (5%)|    -| // remove DATAFLOW
// 100/3.2m   13870|  6.935e+04|         -|    13871|     -|        no|  31 (~0%)|  1394 (11%)|   96864 (2%)|  104724 (6%)|    -|
// 100/3m     13870|  6.935e+04|         -|    13871|     -|        no|  31 (~0%)|  1394 (11%)|   96864 (2%)|  104724 (6%)|    -|



void searchHit(data_t* inTracks, nnscore_t* in_nn, nnscore_t* out_nn, int max_shared, int num_tracks){
  #pragma HLS INLINE off
  // #pragma HLS PIPELINE off
  // #pragma HLS PIPELINE



  for(int i = 0; i < MAX_TRACK_SIZE; i++){
    #pragma HLS PIPELINE
    // #pragma HLS UNROLL off
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