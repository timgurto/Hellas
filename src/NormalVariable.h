#ifndef NORMAL_VARIABLE_H
#define NORMAL_VARIABLE_H

#include <random>

// Generates random numbers with a normal distribution, based on provided
// mean/SD
class NormalVariable {
  bool _zeroSD;
  double _mean, _sd;
  std::normal_distribution<double> *_dist;

  static std::default_random_engine randomEngine;

 public:
  NormalVariable(double mean = 0, double sd = 1);
  NormalVariable(const NormalVariable &rhs);
  ~NormalVariable();
  NormalVariable &operator=(const NormalVariable &rhs);

  double generate() const;
  double operator()() const { return generate(); }
};

#endif
