#include "projectDefines.h"


extern "C"{



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

void searchHit(hls::stream<stream_t>& in_stream, hls::stream<stream_t>& out_stream){
  #pragma HLS interface ap_ctrl_none port=return
  #pragma HLS INTERFACE mode=axis port=in_stream
  #pragma HLS INTERFACE mode=axis port=out_stream


  hls::stream<Track> tmp;
  stream_t s;


  s = in_stream.read();

  // check if end of data first (read_data will write -1 to first data stream)
  if(data_t(s.data) == data_t(-1)){
    out_stream.write(s);
    return;
  }

  Track t;
  t.NNScore = reinterpret_cast<nnscore_t&>(s.data);
  for(int i = 0; i < NHITS; i++){
    Hit h;
    stream_t s1;
    s1 = in_stream.read();
    h.x = data_t(s1.data);
    s1 = in_stream.read();
    h.y = data_t(s1.data);
    s1 = in_stream.read();
    h.z = data_t(s1.data);
    t.hits[i] = h;
  }
  tmp.write(t);



  while(true){
    s = in_stream.read();
    // check if end of data first (read_data will write -1 to first data stream)
    if(data_t(s.data) == data_t(-1)){
      break;
    }

    int size = tmp.size();
    bool flagged_deletion = false;

    Track trkB;
    trkB.NNScore = reinterpret_cast<nnscore_t&>(s.data);
    for(int i = 0; i < NHITS; i++){
      Hit h;
      stream_t s1;
      s1 = in_stream.read();
      h.x = data_t(s1.data);
      s1 = in_stream.read();
      h.y = data_t(s1.data);
      s1 = in_stream.read();
      h.z = data_t(s1.data);
      trkB.hits[i] = h;
    }

    for(int i = 0; i < size; i++){
      Track trkA = tmp.read();
      int comparison_result = compare(trkA, trkB, MIN_DIST, MAX_SHARED);
      if(comparison_result == COMPARISON::TRKA){ // keep trkA
        tmp.write(trkA);
        flagged_deletion = true;
        break;
      } else if(comparison_result == COMPARISON::TRKB){ // keep trkB
        tmp.write(trkB);
        flagged_deletion = true;
        break;
      } else if(comparison_result == COMPARISON::BOTH){ // keep both, but since we need to check everything, dont do anything with trkB, just write trkA back to tmp
        tmp.write(trkA);
        // tmp.write(trkB);
      }
    }
    if(flagged_deletion == false){ // if nothing overlapped, just write the trkB to tmp
      tmp.write(trkB);
    }
    // finished iterating through tmp stream, read in next trkB to compare to the rest
  }

  // int counter = 0;
  while(tmp.size() > 0){
    Track t = tmp.read();
    stream_t s2;
    s2.data = reinterpret_cast<ap_uint<16>&>(t.NNScore);
    out_stream.write(s2);

    for(int i = 0; i < NHITS; i++){
      s2.data = t.hits[i].x;
      out_stream.write(s2);
      s2.data = t.hits[i].y;
      out_stream.write(s2);
      s2.data = t.hits[i].z;
      out_stream.write(s2);
    }
  }
  // write final data (-1)
  out_stream.write(s);


}

}
