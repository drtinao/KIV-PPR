#pragma once
double calc_fact(int number);
class PoissonDistrib
{
	private:
		//constructor variables - START
		double lambda; //average dataset value
		//constructor variables - END

	public:
		PoissonDistrib(double lambda); //expects dataset avg
		double calc_prob_func_concrete_num(double x); //calculates probability function for given number
		double calc_distrib_func_interval(long lower_boundary, long upper_boundary); //calcs distribution function for every numbers in between interval boundaries
};

