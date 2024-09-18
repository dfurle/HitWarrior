#include <fstream>
#include <iostream>
#include <stdlib.h>

#include <chrono>
#include "projectDefines.h"
#include "data_reader.h"


int printTiming(std::string str, std::chrono::_V2::system_clock::time_point & begin) {
  auto end = std::chrono::high_resolution_clock::now();
  int diffus = std::chrono::duration_cast < std::chrono::microseconds > (end - begin).count();
  printf(str.c_str(), diffus);
  begin = end;
  return diffus;
}


int main(int argc, char *argv[]) {
  auto begin = std::chrono::high_resolution_clock::now();

  printf("INITIALIZING DATA\n");

  Track* inTracks = new Track[MAX_TRACK_SIZE * BATCH_SIZE];
  int* nTracks = new int[BATCH_SIZE];
  nnscore_t* outScores = new nnscore_t[MAX_TRACK_SIZE * BATCH_SIZE];

  if(1 == read_data(inTracks, nTracks, outScores, "formatted_data.csv"))
    return 1;

  printf("\n---=== Running CSim ===---\n\n");
  printTiming(" - Initialization %d us\n", begin);
  // runner(inTracks, outTracks, MIN_DIST, MAX_SHARED);
  // runner(inTracks, MIN_DIST, MAX_SHARED);
  runner(inTracks, nTracks, outScores, BATCH_SIZE, MAX_SHARED);
  printTiming(" - runner() %d us\n", begin);
  printf("\n---=== Finished CSim ===---\n\n");

  printf("Pred Outs:\n");
  int counter = 0;
  for (int i = 0; i < MAX_TRACK_SIZE; i++) {
    if(float(outScores[i]) < 0.5){
      continue;
    }
    printf("ID: %d : %d | NNScore: %f\n", i, counter++, float(outScores[i]));
    for(int j = 0; j < NHITS; j++){
      printf(" %8.2f %8.2f %8.2f\n",float(inTracks[i].hits[j].x), float(inTracks[i].hits[j].y), float(inTracks[i].hits[j].z));
    }
    printf("\n");
  }

  std::ofstream outputFile("../../../../../../tb_files/tb_output.dat");
  if(outputFile.is_open()){
    for (int i = 0; i < MAX_TRACK_SIZE; i++) {
      // printf("inTracks[%d].NNScore == %.3f\n", i, float(inTracks[i].NNScore));
      // if(float(inTracks[i].NNScore) < 0.5){
      if(float(outScores[i]) < 0.5){
        continue;
      }
      // outputFile << inTracks[i].NNScore << "\n";
      outputFile << outScores[i] << "\n";
      for(int j = 0; j < NHITS; j++){
        outputFile << inTracks[i].hits[j].x << " ";
        outputFile << inTracks[i].hits[j].y << " ";
        outputFile << inTracks[i].hits[j].z << "\n";
      }
    }
  } else {
    printf("\nFailed to open tb_output.dat file!\n\n");
  }
  outputFile.close();

  std::cout << std::endl;

  std::cout << "Finished" << std::endl;
  printTiming(" - Final Prints %d us\n", begin);
  return 0;
}
