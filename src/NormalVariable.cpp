// (C) 2016 Tim Gurto

#include "NormalVariable.h"

std::default_random_engine NormalVariable::randomEngine;

NormalVariable::NormalVariable(double mean, double sd):
_zeroSD(false),
_mean(mean),
_dist(nullptr){
    if (sd == 0)
        _zeroSD = true;
    else
        _dist = new std::normal_distribution<double>(mean, sd);
}

NormalVariable::~NormalVariable(){
    if (_dist != nullptr)
        delete _dist;
}

double NormalVariable::generate() const{
    if (_zeroSD)
        return _mean;
    return (*_dist)(randomEngine);
}
