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

  Track* inputTracks = new Track[INPUTTRACKSIZE];
  Track* outTracks = new Track[INPUTTRACKSIZE];

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
      inputTracks[count].NNScore = nnscore_t(NNScore);
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
        inputTracks[count].hits[i].x = data_t(inValue[i + 0 * NHITS]);
        inputTracks[count].hits[i].y = data_t(inValue[i + 1 * NHITS]);
        inputTracks[count].hits[i].z = data_t(inValue[i + 2 * NHITS]);
      }
      count++;
      if(count >= INPUTTRACKSIZE) break;
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
  // runner(inputTracks, outTracks, MIN_DIST, MAX_SHARED);
  runner(inputTracks, outTracks, MIN_DIST, MAX_SHARED);
  printTiming(" - runner() %d us\n", begin);
  printf("\n---=== Finished CSim ===---\n\n");

  printf("Pred Outs:\n");
  int counter = 0;
  for (int i = 0; i < INPUTTRACKSIZE; i++) {
    if(float(outTracks[i].NNScore) < 0.5){
      continue;
    }
    printf("ID: %d : %d | NNScore: %f\n", i, counter++, float(outTracks[i].NNScore));
    for(int j = 0; j < NHITS; j++){
      printf(" %8.2f %8.2f %8.2f\n",float(outTracks[i].hits[j].x), float(outTracks[i].hits[j].y), float(outTracks[i].hits[j].z));
    }
    printf("\n");
  }

  std::ofstream outputFile("../../../../../../tb_files/tb_output.dat");
  if(outputFile.is_open()){
    for (int i = 0; i < INPUTTRACKSIZE; i++) {
      // printf("outTracks[%d].NNScore == %.3f\n", i, float(outTracks[i].NNScore));
      if(float(outTracks[i].NNScore) < 0.5){
        continue;
      }
      outputFile << outTracks[i].NNScore << "\n";
      for(int j = 0; j < NHITS; j++){
        outputFile << outTracks[i].hits[j].x << " ";
        outputFile << outTracks[i].hits[j].y << " ";
        outputFile << outTracks[i].hits[j].z << "\n";
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
