#pragma once

#include <fstream>
#include <iostream>
#include <stdlib.h>

#include "projectDefines.h"

int read_data(Track* inTracks, int* nTracks, nnscore_t* outScores, std::string filename){

  // OLD DATAFILE:
  // // Input hit list to search
  // int count = 0;
  // // std::ifstream trackFile("../tb_files/data/formatted_data.csv");
  // std::ifstream trackFile("../tb_files/data/tb_track_data.dat");
  // std::ifstream scoreFile("../tb_files/data/tb_NNscore.dat");
  // if(trackFile.is_open() && scoreFile.is_open()){
  //   // Read the input file
  //   std::string trackLine, scoreLine;
  //   while(std::getline(trackFile, trackLine) && std::getline(scoreFile, scoreLine)){
  //     std::stringstream linestream(trackLine);
  //     std::string s;

  //     double NNScore = std::stof(scoreLine); // get NN score from the other file
  //     inTracks[count].NNScore = NNScore;
  //     // inTracks[count].flag_delete = ap_int<2>(0);

  //     // outTracks[count].NNScore = 0;
  //     // outTracks[count].flag_delete = ap_int<2>(0);
  //     // for(int i = 0; i < NHITS; i++){
  //     //   outTracks[count].hits[i].x = 0;
  //     //   outTracks[count].hits[i].y = 0;
  //     //   outTracks[count].hits[i].y = 0;
  //     // }

  //     // store it in a float
  //     std::vector<double> inValue;
  //     while (std::getline(linestream, s, ' ')){
  //       inValue.push_back(std::stof(s));
  //     }

  //     // Organize it into hits
  //     for(int i = 0; i < NHITS; i++){
  //       inTracks[count].hits[i].x = int(inValue[i + 0 * NHITS]); // TODO: is int() needed?
  //       inTracks[count].hits[i].y = int(inValue[i + 1 * NHITS]);
  //       inTracks[count].hits[i].z = int(inValue[i + 2 * NHITS]);
  //     }
  //     count++;
  //     if(count >= MAX_TRACK_SIZE) break;
  //   }
  // } else {
  //   printf("\n\nFailed to open input files!\n\n");
  //   trackFile.close();
  //   scoreFile.close();
  //   return 0;
  // }
  // trackFile.close();
  // scoreFile.close();


  // Input hit list to search
  // int count = 0;
  std::ifstream trackFile(filename);
  if(trackFile.is_open()){
    // Read the input file
    std::string hitline;
    // skip header
    std::getline(trackFile, hitline);
    for(int b = 0; b < BATCH_SIZE; b++){
      nTracks[b] = MAX_TRACK_SIZE;
      for(int t = 0; t < MAX_TRACK_SIZE; t++){
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

          int index =  b*MAX_TRACK_SIZE + t;

          // printf("%d %d :  %.2f %.2f %.2f\n", index, h, x, y, z);

          inTracks[index].hits[h].x = x;
          inTracks[index].hits[h].y = y;
          inTracks[index].hits[h].z = z;
          inTracks[index].NNScore = nnscore_t(nnscore); // redundant but eh
        }
        // count++;
        // if(count >= MAX_TRACK_SIZE * batch_size) break;
      }
    }
  } else {
    printf("\n\nFailed to open the file!!!\n\n\n");
    trackFile.close();
    return 1;
  }
  trackFile.close();
  return 0;
}
