#include "cl_defines.h"
#include <iostream>
#include <fstream>
#include "NormalDistrib.h"

/*
Constructor takes reference to values which are required for solving distribution function of normal distribution.
These values must be known before class initialization and must be same for every interval.
double me = average, calculated from dataset
double sigma = standard deviation, calculated from dataset
*/
NormalDistrib::NormalDistrib(double me, double sigma)
{
    this->me = me;
    this->sigma = sigma;
}

/*
Function standardizes interval using formula: U = (x - me) / sigma. Returns number for which corresponding value of distribution function could be found in st lookup table. 
double x = upper boundary of interval
*/
double NormalDistrib::standardize_interval(double x)
{
    return (x - this->me) / sigma;
}

/*
Function will find and return corresponding value of distribution function for given standardized value of normal distribution.
Value is picked from predefined lookup table realized by arrays.
double u = standardized value (interval) of normal distribution
*/
double NormalDistrib::find_distrib_func(double u)
{
    double distrib_res;

    int target_index_stand = static_cast<int>(round(std::abs(u) / STANDARDIZE_DIST_ARR_STEP));
    if (target_index_stand > STANDARDIZE_DIST_ARR_SIZE - 1) { //out of bounds, limit close to 1
        distrib_res = 1;
    }
    else {
        distrib_res = this->standardize_dist_arr[target_index_stand];
    }

    if (u < 0) { //num is negative: 1 - fi(u)
        distrib_res = 1 - distrib_res;
    }

    return distrib_res;
}
