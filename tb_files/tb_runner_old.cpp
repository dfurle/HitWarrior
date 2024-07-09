#include <fstream>
#include <iostream>
#include <stdlib.h>

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
  std::ifstream trackFile("tb_track_data.dat");
  std::ifstream scoreFile("tb_NNscore.dat");
  if(trackFile.is_open() && scoreFile.is_open()){
    // Read the input file
    std::string trackLine, scoreLine;
    while(std::getline(trackFile, trackLine) && std::getline(scoreFile, scoreLine)){
      std::stringstream linestream(trackLine);
      std::string s;

      double NNScore = std::stof(scoreLine); // get NN score from the other file
      inTracks[count].NNScore = nnscore_t(NNScore);
      printf("WRITING: %d | %f\n", count, float(nnscore_t(NNScore)));
      // inputTracks[count].flag_delete = ap_int<2>(0);

      // outTracks[count].NNScore = 0;
      // // outTracks[count].flag_delete = ap_int<2>(0);
      // for(int i = 0; i < NHITS; i++){
      //   outTracks[count].hits[i].x = 0;
      //   outTracks[count].hits[i].y = 0;
      //   outTracks[count].hits[i].y = 0;
      // }

      // store it in a float
      std::vector<double> inValue;
      while (std::getline(linestream, s, ' ')){
        inValue.push_back(std::stof(s));
      }

      // Organize it into hits
      for(int i = 0; i < NHITS; i++){
        // printf("Assigning %d : %f %f %f\n", i, inValue[i + 0 * NHITS], inValue[i + 1 * NHITS], inValue[i + 2 * NHITS]);
        inTracks[count].hits[i].x = data_t(inValue[i + 0 * NHITS]);
        inTracks[count].hits[i].y = data_t(inValue[i + 1 * NHITS]);
        inTracks[count].hits[i].z = data_t(inValue[i + 2 * NHITS]);
      }
      count++;
      if(count >= MAX_TRACK_SIZE) break;
    }
  } else {
    printf("\n\nFailed to open one of the files!!!\n\n\n");
    trackFile.close();
    scoreFile.close();
    return 0;
  }
  trackFile.close();
  scoreFile.close();

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
