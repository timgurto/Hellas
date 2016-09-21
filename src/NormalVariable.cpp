// (C) 2016 Tim Gurto

#include "NormalVariable.h"

std::default_random_engine NormalVariable::randomEngine;

NormalVariable::NormalVariable(double mean, double sd):
_zeroSD(false),
_mean(mean),
_sd(sd),
_dist(nullptr){
    if (sd == 0)
        _zeroSD = true;
    else
        _dist = new std::normal_distribution<double>(mean, sd);
}

NormalVariable::NormalVariable(const NormalVariable &rhs):
_zeroSD(rhs._zeroSD),
_mean(rhs._mean),
_sd(rhs._sd),
_dist(nullptr)
{
    if (!_zeroSD)
        _dist = new std::normal_distribution<double>(_mean, _sd);
}

NormalVariable::~NormalVariable(){
    if (_dist != nullptr)
        delete _dist;
}

NormalVariable &NormalVariable::operator=(const NormalVariable &rhs){
    if (this != &rhs){
        if (_dist != nullptr)
            delete _dist;
        _zeroSD = rhs._zeroSD;
        _mean = rhs._mean;
        _sd = rhs._sd;
        if (!_zeroSD)
            _dist = new std::normal_distribution<double>(_mean, _sd);
        else
            _dist = nullptr;
    }
    return *this;
}

double NormalVariable::generate() const{
    if (_zeroSD)
        return _mean;
    return (*_dist)(randomEngine);
}
