#pragma once

#include <math.h>


//-------------------------------------
//  MersenneTwister
//  A random number generator with good randomness
//  in a small number of instructions.
//-------------------------------------
#define MT_IA  397
#define MT_LEN 624

class MersenneTwister
{
    unsigned int m_buffer[MT_LEN];
    int m_index;

public:
    MersenneTwister();
    MersenneTwister(unsigned int seed);
    void reseed(unsigned int seed);
    unsigned int integer();
    float expoVariate(float ooRateParameter)
    {
        return -logf(1.0f - integer() / 4294967296.0f) * ooRateParameter;
    }
};
