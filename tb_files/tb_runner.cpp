#include <fstream>
#include <iostream>
#include <stdlib.h>

#include "projectDefines.h"

int main(int argc, char *argv[]) {

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
      inputTracks[count].NNScore = NNScore;
      inputTracks[count].flag_delete = ap_int<2>(0);

      outTracks[count].NNScore = 0;
      outTracks[count].flag_delete = ap_int<2>(0);
      for(int i = 0; i < NHITS; i++){
        outTracks[count].hits[i].x = 0;
        outTracks[count].hits[i].y = 0;
        outTracks[count].hits[i].y = 0;
      }

      // store it in a float
      std::vector<double> inValue;
      while (std::getline(linestream, s, ' ')){
        inValue.push_back(std::stof(s));
      }

      // Organize it into hits
      for(int i = 0; i < NHITS; i++){
        inputTracks[count].hits[i].x = inValue[i + 0 * NHITS];
        inputTracks[count].hits[i].y = inValue[i + 1 * NHITS];
        inputTracks[count].hits[i].z = inValue[i + 2 * NHITS];
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
  runner(inputTracks, outTracks, MIN_DIST, MAX_SHARED);
  printf("\n---=== Finished CSim ===---\n\n");

  printf("Pred Outs:\n");
  for (int i = 0; i < INPUTTRACKSIZE; i++) {
    if(float(outTracks[i].NNScore) < 0.5){
      break;
    }
    printf("ID: %d | NNScore: %f\n", i, float(outTracks[i].NNScore));
    for(int j = 0; j < NHITS; j++){
      printf(" %d %d %d\n",outTracks[i].hits[j].x, outTracks[i].hits[j].y, outTracks[i].hits[j].z);
    }
    printf("\n");
  }

  std::ofstream outputFile("../tb_output.dat");
  if(outputFile.is_open()){
    for (int i = 0; i < INPUTTRACKSIZE; i++) {
      if(float(outTracks[i].NNScore) < 0.5){
        break;
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
  return 0;
}
