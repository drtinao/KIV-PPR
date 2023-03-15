#pragma once
class ExponentialDistrib
{
	private:
		//constructor variables - START
		double lambda; //average dataset value
		//constructor variables - END

	public:
		ExponentialDistrib(double lambda); //constructor takes just dataset avg
		double calc_distrib_func(double x); //calculates distrib. func.
};

