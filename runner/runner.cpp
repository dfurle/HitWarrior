#include "projectDefines.h"

#include <iostream>

#define RUN_PIPELINE


extern "C"{

// void searchHit(Track* inputTracks, Track* outTracks, int min_dist, int max_shared){
void searchHit(Track* inputTracks, Track* outTracks, int min_dist, int max_shared){

  int NNScore_rejected = 0;
  int Overlap_rejected = 0;
  int Total_counter = 0;

  FIRST_PASS:
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    if(inputTracks[i].NNScore < MIN_THRESHOLD){
      inputTracks[i].flag_delete = ap_int<2>(1);
      NNScore_rejected++;
      printf("Rejecting NN: %3d ( %.4f < %.2f )\n", i, float(inputTracks[i].NNScore), MIN_THRESHOLD);
      continue;
    }
  }

  printf("\nRejected %d tracks by NN score\n\n", NNScore_rejected);

  // Loop over each hit and find the closest hit
  OUTERLOOP:
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    #pragma HLS unroll complete
    Track track1 = inputTracks[i];
    // if(track1.flag_delete == ap_int<2>(1))
    //   continue;


    // if(track1.NNScore < MIN_THRESHOLD){
    //   inputTracks[i].flag_delete = ap_int<2>(1);
    //   NNScore_rejected++;
    //   printf("Rejecting NNi    : %d\n", i);
    //   continue;
    // }


    OUTER2LOOP:
    for(int j = i+1; j < INPUTTRACKSIZE; j++){
      #pragma HLS unroll complete
      Track track2 = inputTracks[j];
      // if(track2.flag_delete == ap_int<2>(1))
      //   continue;


      // if(track2.NNScore < MIN_THRESHOLD){
      //   inputTracks[j].flag_delete = ap_int<2>(1);
      //   NNScore_rejected++;
      //   printf("Rejecting NN j   : %d\n", j);
      //   continue;
      // }
      int nShared = 0;

      INNERLOOP:
      for(int ii = 0; ii < NHITS; ii++){
        #pragma HLS unroll complete
        Hit hit1 = track1.hits[ii];
        INNER2LOOP:
        for(int jj = 0; jj < NHITS; jj++){
          #pragma HLS unroll complete
          Hit hit2 = track2.hits[jj];

          int distx = hit1.x - hit2.x;
          int disty = hit1.y - hit2.y;
          int distz = hit1.z - hit2.z;

          int dist = distx*distx + disty*disty + distz*distz;
          // printf("%d,", int(dist));
          // if(dist < MIN_DIST){
          if(dist < min_dist){
            // printf("Adding, ");
            nShared++;
          }
        }
      }
      // printf("\nnShared final: %d, NNSCORE: %f / %f\n\n", nShared, float(track1.NNScore), float(track2.NNScore));

      if(nShared >= max_shared){
        if(track1.NNScore >= track2.NNScore){ // choose to keep track1 if exactly equal
          if(inputTracks[j].flag_delete != ap_int<2>(1))
            Overlap_rejected++;
          inputTracks[j].flag_delete = ap_int<2>(1);
          printf("Rejecting Overlap _j: %4d ( i = %4d | j = %4d) ( i=%.4f >= j=%.4f ?)\n", j, i, j, float(track1.NNScore), float(track2.NNScore));
        } else {
          if(inputTracks[i].flag_delete != ap_int<2>(1))
            Overlap_rejected++;
          inputTracks[i].flag_delete = ap_int<2>(1);
          printf("Rejecting Overlap i_: %4d ( i = %4d | j = %4d) ( i=%.4f >= j=%.4f ?)\n", i, i, j, float(track1.NNScore), float(track2.NNScore));
          // Overlap_rejected++;
          break;
        }
      } else {
        // if too few hits do something here?
        printf("i = %4d | j = %4d too few shared %d\n", i, j, nShared);
      }
    }
  }

  int counter = 0;
  WRITE_LOOP:
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    #pragma HLS unroll complete
    Track track = inputTracks[i];
    if(track.flag_delete == ap_int<2>(1)){
      // printf("Skipping: %d\n", i);
      continue;
    }
    printf("adding %d / %d\n", counter, i);
    outTracks[counter++] = track;
  }
  printf("\n");
  printf("NNScore Rejected: %d\n", NNScore_rejected);
  printf("Overlap Rejected: %d\n", Overlap_rejected);
  printf("Sum             : %d\n", NNScore_rejected + Overlap_rejected);
  printf("Counter         : %d\n", counter);
}


void read_data(Track* full_array1, Track* array1){
  READ_IN1_ARRAY1:
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    array1[i] = full_array1[i];
  }
}

void write_data(Track* full_output, Track* output){
  WRITE_OUTPUT:
  for(int i = 0; i < INPUTTRACKSIZE; i++){
    full_output[i] = output[i];
  }
}

void runner(Track* inputTracks, Track* outTracks, int min_dist, int max_shared){
  // #pragma HLS PIPELINE
  #pragma HLS DATAFLOW
  #pragma HLS INTERFACE m_axi port=inputTracks offset=slave bundle=aximm1
  #pragma HLS INTERFACE m_axi port=outTracks offset=slave bundle=aximm1

  // #pragma HLS INTERFACE ap_ctrl_none port=return


  // ---=== might be better for certain situations, but will limit maximum size ===---
  Track input_array[INPUTTRACKSIZE];
  Track out_array[INPUTTRACKSIZE];
  int min_dist_read = min_dist;
  int max_shared_read = max_shared;
  #pragma HLS ARRAY_PARTITION variable=input_array complete
  #pragma HLS ARRAY_PARTITION variable=out_array complete
  read_data(inputTracks, input_array);
  searchHit(input_array, out_array, min_dist_read, max_shared_read);
  write_data(outTracks, out_array);
  
  // ---=== otherwise just run something like this ===---
  // searchHit(inputHistList, searchHitList, outList);

}


}