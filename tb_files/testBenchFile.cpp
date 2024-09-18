#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "NNOverlapDecorator_kernel/NNOverlapDecorator_kernel.h"

// hls-fpga-machine-learning insert bram

#define CHECKPOINT 5000
#define MAX_TRACK_SIZE 10

namespace nnet {
    bool trace_enabled = true;
    std::map<std::string, void *> *trace_outputs = NULL;
    size_t trace_type_size = sizeof(double);
} // namespace nnet

int main(int argc, char **argv) {
    // load input data from text file
    std::ifstream file("tb_data/tb_input.dat");
    // load predictions from text file
    std::ifstream fpr("tb_data/tb_output.dat");

    int num_trk = MAX_TRACK_SIZE;


    ///////////////////////
    // Track like object
    ///////////////////////
    NN::InputTrack* trackList = new NN::InputTrack[num_trk];

    std::string sa;
    int index = 0;
    for (int j = 0; j < num_trk; j++) 
    {
        std::cout<<"Getting Track : "<<j<<" "<<std::endl;
        getline(file, sa);
        std::istringstream iss(sa);
        std::string s;

        // store it in a float
        std::vector<double> inValue;
        while (getline(iss, s, ' ')) 
        {
            inValue.push_back(atof(s.c_str()));
        }

        // Organize it into hits
        for(int i = 0; i < NHITS; i++)
        {
            trackList[j].hits[i].X = inValue[i + 0 * NHITS];
            trackList[j].hits[i].Y = inValue[i + 1 * NHITS];
            trackList[j].hits[i].Z = inValue[i + 2 * NHITS];
        }
    }

    for (int j = 0; j < MAX_TRACK_SIZE; j++) 
    {  
        std::cout<<"Track : "<<j<<" ";
        for(int i = 0; i < NHITS; i++)
        {
            std::cout<<trackList[j].hits[i].X<<" "<<trackList[j].hits[i].Y<<" "<<trackList[j].hits[i].Z<<" ";
        }
        std::cout<<std::endl;
    }


    NN::OutputTrack* out1 = new NN::OutputTrack[num_trk];

    NNOverlapDecorator_kernel(trackList, out1);


    std::cout << "Result: \n\n";
    for (int j = 0; j < num_trk; j++) {
        for (int i = 0; i < N_LAYER_8; i++) {
            std::cout << "j: "<<j<<" "<<float(out1[j].NNScore[i]) << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}
