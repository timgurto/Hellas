// (C) 2016 Tim Gurto

#ifndef NORMAL_VARIABLE_H
#define NORMAL_VARIABLE_H

// Generates random numbers with a normal distribution, based on provided mean/SD
class NormalVariable{

public:
    NormalVariable(double mean = 0, double sd = 1);
    double generate();
};

#endif
