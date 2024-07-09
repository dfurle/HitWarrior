#include <fstream>
#include <iostream>
#include <stdlib.h>

#include <chrono>
#include "projectDefines.h"


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

  Track* inTracks = new Track[MAX_TRACK_SIZE];
  // Track* outTracks = new Track[MAX_TRACK_SIZE];

  // Input hit list to search
  int count = 0;
  std::ifstream trackFile("formatted_data.csv");
  if(trackFile.is_open()){
    // Read the input file
    std::string hitline;
    // skip header
    std::getline(trackFile, hitline);
    while(true){ // assume it never ends... bad but eh
      for(int h = 0; h < NHITS; h++){ // 5 hits
        if(!std::getline(trackFile, hitline)){
          printf("END OF FILE\n");
          break;
      }
        std::stringstream linestream(hitline);
        std::string s;

        std::getline(linestream, s, ',');
        int trackid = std::stoi(s);

        std::getline(linestream, s, ',');
        int hitid = std::stoi(s);

        std::getline(linestream, s, ',');
        float pt = std::stof(s);

        std::getline(linestream, s, ',');
        int truth = std::stoi(s);

        std::getline(linestream, s, ',');
        float nnscore = std::stof(s);

        std::getline(linestream, s, ',');
        float x = std::stof(s);

        std::getline(linestream, s, ',');
        float y = std::stof(s);

        std::getline(linestream, s, ',');
        float z = std::stof(s);

        // printf("%d %d :  %.2f %.2f %.2f\n", count, h, x, y, z);

        inTracks[count].hits[h].x = x;
        inTracks[count].hits[h].y = y;
        inTracks[count].hits[h].z = z;
        inTracks[count].NNScore = nnscore_t(nnscore); // redundant but eh
      }
      count++;
      if(count >= MAX_TRACK_SIZE) break;
    }
  } else {
    printf("\n\nFailed to open one of the files!!!\n\n\n");
    trackFile.close();
    return 0;
  }
  trackFile.close();

  printf("\n---=== Running CSim ===---\n\n");
  printTiming(" - Initialization %d us\n", begin);
  // runner(inTracks, outTracks, MIN_DIST, MAX_SHARED);
  runner(inTracks, MIN_DIST, MAX_SHARED);
  printTiming(" - runner() %d us\n", begin);
  printf("\n---=== Finished CSim ===---\n\n");

  printf("Pred Outs:\n");
  int counter = 0;
  for (int i = 0; i < MAX_TRACK_SIZE; i++) {
    if(float(inTracks[i].NNScore) < 0.5){
      continue;
    }
    printf("ID: %d : %d | NNScore: %f\n", i, counter++, float(inTracks[i].NNScore));
    for(int j = 0; j < NHITS; j++){
      printf(" %8.2f %8.2f %8.2f\n",float(inTracks[i].hits[j].x), float(inTracks[i].hits[j].y), float(inTracks[i].hits[j].z));
    }
    printf("\n");
  }

  std::ofstream outputFile("../../../../../../tb_files/tb_output.dat");
  if(outputFile.is_open()){
    for (int i = 0; i < MAX_TRACK_SIZE; i++) {
      // printf("inTracks[%d].NNScore == %.3f\n", i, float(inTracks[i].NNScore));
      if(float(inTracks[i].NNScore) < 0.5){
        continue;
      }
      outputFile << inTracks[i].NNScore << "\n";
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
  printTiming(" - Finished %d us\n", begin);
  return 0;
}
