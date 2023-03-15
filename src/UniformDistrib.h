#pragma once
class UniformDistrib
{
private:
	//constructor variables - START
	double a; //minimum value present in dataset
	double b; //maximum value present in dataset
	//constructor variables - END

public:
	UniformDistrib(double a, double b); //takes just reference to values
	double calc_distrib_fun(double x); //calcs distrib. function
};

