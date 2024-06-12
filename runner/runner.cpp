#include "projectDefines.h"

#include <iostream>

#define RUN_PIPELINE


extern "C"{


void filterLowNN(Track* inTracks, Track* outTracks){
  #pragma HLS interface ap_ctrl_none port=return
  int counter = 0;
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    Track trk = inTracks[i];
    if(trk.NNScore >= MIN_THRESHOLD){
      outTracks[counter++] = inTracks[i];
    }
  }
  if(counter != INPUTTRACKSIZE){
    outTracks[counter].NNScore = -1;
  }
}


int compare(Track trkA, Track trkB, int min_dist, int max_shared){
  int nShared = 0;
  INNERLOOP:
  for(int ii = 0; ii < NHITS; ii++){
    #pragma HLS unroll complete
    Hit hit1 = trkA.hits[ii];
    INNER2LOOP:
    for(int jj = 0; jj < NHITS; jj++){
      #pragma HLS unroll complete
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

void searchHit(Track* inTracks, Track* outTracks, int min_dist, int max_shared){
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    Track trkA = inTracks[i];
    if(trkA.NNScore < 0){ // reached end of trimmed list
      continue;
      // break;
    }
    for(int j = i+1; j < INPUTTRACKSIZE; j++){
      Track trkB = inTracks[j];
      if(trkB.NNScore < 0){ // reached end of trimmed list
        continue;
        // break;
      }
      int cmpr = compare(trkA, trkB, min_dist, max_shared);
      if(cmpr == COMPARISON::TRKA){
        trkB.NNScore = -1;
      } else if(cmpr == COMPARISON::TRKB){
        trkA.NNScore = -1;
      } else {
        
      }
    }
  }


  int counter = 0;
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    Track trk = inTracks[i];
    if(trk.NNScore < 0){ // reached end of trimmed list
      continue;
    }
    outTracks[counter++] = trk;
  }
}

// void searchHit(Track* inTracks, Track* outTracks){

//   hls::stream<Track> tmp;

//   inTracks
//   tmp.write(t);

//   while(true){
//     s = in_stream.read();
//     // check if end of data first (read_data will write -1 to first data stream)
//     if(data_t(s.data) == data_t(-1)){
//       break;
//     }

//     int size = tmp.size();
//     bool flagged_deletion = false;

//     Track trkB;
//     trkB.NNScore = reinterpret_cast<nnscore_t&>(s.data);
//     for(int i = 0; i < NHITS; i++){
//       Hit h;
//       stream_t s1;
//       s1 = in_stream.read();
//       h.x = data_t(s1.data);
//       s1 = in_stream.read();
//       h.y = data_t(s1.data);
//       s1 = in_stream.read();
//       h.z = data_t(s1.data);
//       trkB.hits[i] = h;
//     }

//     for(int i = 0; i < size; i++){
//       Track trkA = tmp.read();
//       int comparison_result = compare(trkA, trkB, MIN_DIST, MAX_SHARED);
//       if(comparison_result == COMPARISON::TRKA){ // keep trkA
//         tmp.write(trkA);
//         flagged_deletion = true;
//         break;
//       } else if(comparison_result == COMPARISON::TRKB){ // keep trkB
//         tmp.write(trkB);
//         flagged_deletion = true;
//         break;
//       } else if(comparison_result == COMPARISON::BOTH){ // keep both, but since we need to check everything, dont do anything with trkB, just write trkA back to tmp
//         tmp.write(trkA);
//         // tmp.write(trkB);
//       }
//     }
//     if(flagged_deletion == false){ // if nothing overlapped, just write the trkB to tmp
//       tmp.write(trkB);
//     }
//     // finished iterating through tmp stream, read in next trkB to compare to the rest
//   }

//   // int counter = 0;
//   while(tmp.size() > 0){
//     Track t = tmp.read();
//     stream_t s2;
//     s2.data = reinterpret_cast<ap_uint<16>&>(t.NNScore);
//     out_stream.write(s2);

//     for(int i = 0; i < NHITS; i++){
//       s2.data = t.hits[i].x;
//       out_stream.write(s2);
//       s2.data = t.hits[i].y;
//       out_stream.write(s2);
//       s2.data = t.hits[i].z;
//       out_stream.write(s2);
//     }
//   }
//   // write final data (-1)
//   out_stream.write(s);
// }


void runner(Track* inputTracks, Track* outTracks, int min_dist, int max_shared){
  // #pragma HLS PIPELINE
  // #pragma HLS DATAFLOW
  #pragma HLS INTERFACE m_axi port=inputTracks offset=slave bundle=aximm1
  #pragma HLS INTERFACE m_axi port=outTracks offset=slave bundle=aximm1

  // #pragma HLS INTERFACE ap_ctrl_none port=return


  // ---=== might be better for certain situations, but will limit maximum size ===---
  Track inTracks_copy[INPUTTRACKSIZE];
  Track midTracks_copy[INPUTTRACKSIZE];
  Track outTracks_copy[INPUTTRACKSIZE];
  #pragma HLS ARRAY_PATITION inTracks_copy complete
  #pragma HLS ARRAY_PATITION midTracks_copy complete
  #pragma HLS ARRAY_PATITION outTracks_copy complete
  int min_dist_read = min_dist;
  int max_shared_read = max_shared;

  for(int i=0; i < INPUTTRACKSIZE; i++) {
    #pragma HLS PIPELINE
    inTracks_copy[i] = inputTracks[i];
  }

  filterLowNN(inTracks_copy, midTracks_copy);
  searchHit(midTracks_copy, outTracks_copy, min_dist_read, max_shared_read);

  for(int i=0; i < INPUTTRACKSIZE; i++) {
    #pragma HLS PIPELINE
    outTracks[i] = outTracks_copy[i];
  }

  // ---=== otherwise just run something like this ===---
  // searchHit(inputHistList, searchHitList, outList);

}


}