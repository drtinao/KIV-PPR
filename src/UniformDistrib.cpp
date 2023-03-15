#include "UniformDistrib.h"

/*
Constructor takes reference to values which are used for calculating distribution function of uniform distribution.
These values are same for all intervals and are known before creating instance of this class.
double a = minimum value present in dataset
double b = maximum value present in dataset
*/
UniformDistrib::UniformDistrib(double a, double b)
{
	this->a = a;
	this->b = b;
}

/*
Function calculates distribution function of uniform distribution for given value and returns result.
double x = upper boundary of interval
*/
double UniformDistrib::calc_distrib_fun(double x)
{
	return (x - this->a) / (this->b - this->a);
}
