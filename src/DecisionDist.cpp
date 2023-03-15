#include <cmath>
#include <iostream>
#include "DecisionDist.h"
#include "cl_defines.h"

/*
Increase total dataset count only if std::fpclassify returns FP_NORMAL or FP_ZERO for given number.
int count_to_add = valid number count to add
*/
void DecisionDist::update_count(int count_to_add)
{
	this->count += count_to_add;
}

/*
Performs normalization of average / standard deviation. Ie. each number is divided by dataset max before calculation of Welfords algorithm.
After calculation, average / standard deviation must be multiplicated to get the right value.
double dataset_max = maximum number from dataset
*/
void DecisionDist::enable_avg_var_normalization(double dataset_max) {
	this->normalize = true;
	this->normalize_val = dataset_max;
}

/*
Multiplicates calculated average and standard deviation to get right values. Can be applied only if normalization is active.
*/
void DecisionDist::finalize_avg_std_dev_normalization() {
	if (this->normalize) {
		this->avg *= this->normalize_val;
		this->std_dev *= this->normalize_val;
	}
}

/*
Updates rolling mean and standard deviation of dataset using given number.
Calculation is done using Welfords online algorithm.
double num = number from dataset using which mean and standard deviation will be udated
*/
void DecisionDist::update_avg_var(double num) {
	if (this->normalize) {
		num /= this->normalize_val;
	}

	double delta = num - this->avg;
	double divider = this->welford_counter + 1;
	this->avg += delta / divider;
	this->m_sum += delta * (num - this->avg);

	this->variance = this->m_sum / (this->welford_counter + 1);

	welford_counter++;
}

/*
Calculates standard deviation of dataset. Variance must be determined before calculation.
*/
void DecisionDist::calc_std_dev()
{
	this->std_dev = sqrt(this->variance);
}

/*
Getter for min_value variable.
*/
double DecisionDist::get_min_value()
{
	return this->min_value;
}

/*
Getter for max_value variable.
*/
double DecisionDist::get_max_value()
{
	return this->max_value;
}

/*
Getter for avg variable.
*/
double DecisionDist::get_avg()
{
	return this->avg;
}

/*
Getter for std_dev variable.
*/
double DecisionDist::get_std_dev()
{
	return this->std_dev;
}

/*
Getter for count variable.
*/
long DecisionDist::get_count()
{
	return this->count;
}

/*
Used for resetting (= 0) variable which reflects number of valid numbers in dataset.
*/
void DecisionDist::reset_count() {
	this->count = 0;
}

/*
Getter for dec_point_num variable.
*/
bool DecisionDist::get_dec_point_num() {
	return this->dec_point_num;
}

/*
Getter for negative_num variable.
*/
bool DecisionDist::get_negative_num() {
	return this->negative_num;
}

/*
Setter for min_value variable.
*/
void DecisionDist::set_min_value(double min_value)
{
	this->min_value = min_value;
}

/*
Setter for max_value variable.
*/
void DecisionDist::set_max_value(double max_value)
{
	this->max_value = max_value;
}

/*
Setter for dec_point_num variable.
bool found = value to set
*/
void DecisionDist::set_dec_point_num(bool found)
{
	this->dec_point_num = found;
}

/*
Setter for negative_num variable.
bool found = value to set
*/
void DecisionDist::set_negative_num(bool found)
{
	this->negative_num = found;
}