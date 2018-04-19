


#include "cudaUtils.h"
#include "../equations/wave.h"
#include "../decomposition/decomp.h"

int main(int argc, char *argv[])
{
    makeMPI(argc, argv);

    std::string scheme = argv[1];

    // Equation, grid, affinity data
    std::ifstream injson(argv[2], std::ifstream::in);
    injson >> inJ;
    injson.close();

    parseArgs(argc, argv);
    initArgs();

    Region **regions;
    setRegion(regions);

    std::string pth = argv[3];
    int localRegions = 1 + cGlob.hasGpu*(cGlob.gpuA - 1);

    for (int k = 0; k<localRegions; k++)
    {
        regions[k]->initializeState(scheme, pth);
    }

    // FOR REGION IN REGIONS WRITE IT OUT.
    
    return 0;
}