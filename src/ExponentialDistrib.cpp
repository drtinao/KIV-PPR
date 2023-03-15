#include "cl_defines.h"
#include "ExponentialDistrib.h"
#include <cmath>

/*
Constructor takes reference to some of values which are required for solving distribution function of given distribution.
Constructor takes only values which are valid for all intervals, other required values (different for each interval) are accepted by respective function.
double lambda = average, calculated from dataset
*/
ExponentialDistrib::ExponentialDistrib(double lambda)
{
	this->lambda = lambda;
}

/*
Used for calculating distribution function of exponential distribution using values accepted by function + constructor values.
double x = number for which probability function should be solved
*/
double ExponentialDistrib::calc_distrib_func(double x)
{
	return 1-exp(-(x / this->lambda));
}
