#include "cl_defines.h"
#include "PoissonDistrib.h"
#include <cmath>
#include "const.h"
#include <iostream>

/*
Constructor takes reference to values which are required for solving distribution function and are same for all of intervals. These values should be known before creating instance of the class.
double lambda = average, calculated from dataset
*/
PoissonDistrib::PoissonDistrib(double lambda)
{
	this->lambda = lambda;
}

/*
Calculates probability function of Poisson distribution for one concrete num and returns result.
Formula for Poisson distribution function: ((lambda ^ x) / (x!)) * (e ^ -lambda)
double x = number from interval for which probability function should be solved
*/
double PoissonDistrib::calc_prob_func_concrete_num(double x)
{
	if (x > 20 || isinf(x)) { //use Ramanujan approximation only if factorial is too high (> 20)
		double logarithm_lambda = x * log(this->lambda) - this->lambda - (x * log(x) - x + log(x * (1 + 4 * x * (1 + 2 * x))) / 6 + log(PI) / 2);
		return exp(logarithm_lambda);
	}
	else { //calculate probability using unmodified function (with factorial), no approximation required
		return ((pow((this->lambda), x) / calc_fact(static_cast<int>(x))) * exp(-this->lambda));
	}
}

/*
Calculates factorial of given number and returns result.
int number = number for which factorial is calculated
*/
double calc_fact(int number) {
	long cur_num = number; //init fact to input num
	double fact = 1;
	while (cur_num >= 1)
	{
		fact = fact * cur_num;
		cur_num--;
	}

	return fact;
}

/*
Calculates Poisson distribution function and returns result. Basically calculates probability function for every integer number located in interval and sums up all results.
long lower_boundary = lower boundary of interval for which distrib. func. should be solved
long upper_boundary = upper boundary of interval which should be solved
*/
double PoissonDistrib::calc_distrib_func_interval(long lower_boundary, long upper_boundary)
{
	double totalSum = 0;

	for (int currentNum = lower_boundary; currentNum <= upper_boundary; currentNum++) {
		totalSum += calc_prob_func_concrete_num(currentNum);
	}

	return totalSum;
}
