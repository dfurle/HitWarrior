#include "projectDefines.h"

#include <iostream>

#define RUN_PIPELINE


extern "C"{

void runner(Track* inputTracks, Track* outTracks, int min_dist, int max_shared){
  // #pragma HLS PIPELINE
  #pragma HLS DATAFLOW
  #pragma HLS INTERFACE m_axi port=inputTracks offset=slave bundle=aximm1
  #pragma HLS INTERFACE m_axi port=outTracks offset=slave bundle=aximm1

  // #pragma HLS INTERFACE ap_ctrl_none port=return


  // ---=== might be better for certain situations, but will limit maximum size ===---
  hls::stream<Track> stream1;
  hls::stream<Track> stream2;
  hls::stream<Track> stream3;
  int min_dist_read = min_dist;
  int max_shared_read = max_shared;
  hls::stream<int> done_flagger;

  // read_data(inputTracks, stream1);
  // filterLowNN(stream1, stream2);
  // printf("\nRejected %d tracks by NN score\n\n", NNScore_rejected);
  // searchHit(stream2, stream3, min_dist_read, max_shared_read);
  // write_data(outTracks, stream3);
  
  // ---=== otherwise just run something like this ===---
  // searchHit(inputHistList, searchHitList, outList);

}


}