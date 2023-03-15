#include "cl_defines.h"
#include <iostream>
#include <fstream>
#include "IntervalManager.h"
#include <iomanip>

/*
Constructor inits array with occurrance counter for each interval - using Sturges rule: k = 1 + 3,32 × log(n).
double min_value_data = minimum value found in dataset (lower boundary of first counter)
double max_value_data = maximum value found in dataset (upper boundary of last counter)
long count = count of valid numbers in dataset (std::fpclassify gives FP_NORMAL / FP_ZERO)
*/
IntervalManager::IntervalManager(double min_value_data, double max_value_data, long count)
{
	//input values
	this->min_value_data = min_value_data;
	this->max_value_data = max_value_data;
	this->count = count;

	//calculated values - START
	this->interval_count = static_cast<int>(round(1 + 3.32 * log10(this->count))); //Sturges rule
	if (this->min_value_data < 0) { //minimum value in dataset is negative - for adding number use formula (number - abs(minimum dataset)) / interval size
		this->min_value_neg = true;
	}
	else { //add number formula: (number - minimum dataset) / interval size
		this->min_value_neg = false;
	}

	double part_int_size_1 = (this->max_value_data / this->max_value_data) / this->interval_count;
	double part_int_size_2 = (this->min_value_data / this->max_value_data) / this->interval_count;

	this->interval_size = (part_int_size_1 - part_int_size_2); //size: (max - min) / available interval count

	this->interval_counter = std::vector<int>(this->interval_count, 0);
	this->interval_bound_low = std::vector<double>(this->interval_count, 0);
	this->interval_bound_up = std::vector<double>(this->interval_count, 0);

	const int loop_end = this->interval_count;
	std::vector<double> interval_low_vect(this->interval_count,0);
	std::vector<double> interval_up_vect(this->interval_count,0);
	for (int i = 0; i < loop_end; i++) { //define boundaries for each interval, save it to vector
		interval_low_vect[i] = ((this->min_value_data / this->max_value_data) + (this->interval_size * i)) * this->max_value_data; //lower boundary
		interval_up_vect[i] = ((this->min_value_data / this->max_value_data) + (this->interval_size * (i + 1))) * this->max_value_data; //upper boundary
	}
	//calculated values - END
	this->interval_bound_low = interval_low_vect;
	this->interval_bound_up = interval_up_vect;

	this->interval_size *= this->max_value_data; //boundaries calculated, get original number
}

/*
Merges intervals so that every resulting interval has count >= 5. This requirement is given by definition of chi-square goodness of fit test.
Algorithm is: go through intervals - if interval with insufficient count found, try to merge it with following ones until count is >= 5.
*/
void IntervalManager::merge_intervals() {
	std::vector<int> interval_counter_merge; //interval counter with updated intervals (>= 5 count each)
	std::vector<double> interval_bound_low_merge; //updated interval lower boundaries
	std::vector<double> interval_bound_up_merge; //updated interval upper boundaries

	int interval_count_orig; //count of one interval, original vector
	double interval_bound_low_orig; //lower boundary - one interval
	double interval_bound_up_orig; //upper boundary - one interval

	bool merge_last_two_intervals = false;
	int interval_forw_count = 0;
	for (int i = 0; i < this->interval_count; i++) { //go through original intervals
		interval_count_orig = this->interval_counter[i];
		interval_bound_low_orig = this->interval_bound_low[i];
		interval_bound_up_orig = this->interval_bound_up[i];

		interval_forw_count = 0; //determine how many following intervals should be added (merged)
		while (interval_count_orig < 5) { //insufficient count - try to add count of following interval
			if ((i + interval_forw_count + 1) < interval_counter.size()) { //ok, still got something to add from original interval vector
				interval_count_orig += this->interval_counter[i + interval_forw_count + 1];
				interval_bound_up_orig = this->interval_bound_up[i + interval_forw_count + 1];
				interval_forw_count++;
			}
			else {
				merge_last_two_intervals = true;
				break;
			}
		}
		i += interval_forw_count;

		//add updated intervals data to respective vectors
		interval_counter_merge.push_back(interval_count_orig);
		interval_bound_low_merge.push_back(interval_bound_low_orig);
		interval_bound_up_merge.push_back(interval_bound_up_orig);
	}

	//last interval could still have insufficient count - if end of original vector was reached while merging -> merge last interval with previous one in newly created vector
	if (merge_last_two_intervals == true) {
		interval_counter_merge[interval_counter_merge.size() - 2] += interval_counter_merge[interval_counter_merge.size() - 1]; //update count
		interval_bound_up_merge[interval_bound_up_merge.size() - 2] = interval_bound_up_merge[interval_bound_up_merge.size() - 1]; //update upper boundary (lower stays the same)
		//remove last element from each vector
		interval_counter_merge.pop_back();
		interval_bound_low_merge.pop_back();
		interval_bound_up_merge.pop_back();
	}

	//replace original intervals with merged ones
	this->interval_count = static_cast<int>(interval_counter_merge.size());
	this->interval_counter = interval_counter_merge;
	this->interval_bound_low = interval_bound_low_merge;
	this->interval_bound_up = interval_bound_up_merge;
}

/*
Prints count in individual intervals to screen. Useful for debug purposes.
*/
void IntervalManager::print_interval_cont_debug() {
	for (int i = 0; i < this->interval_count; i++) {
		std::cout << "index: " << i << ", interval: " << this->interval_bound_low[i] << " - " << this->interval_bound_up[i] << ", count: " << this->interval_counter[i] << std::endl;
	}
}

/*
Getter for interval_size variable.
*/
double IntervalManager::get_interval_size()
{
	return this->interval_size;
}

/*
Getter for interval_count variable.
*/
int IntervalManager::get_interval_count() {
	return this->interval_count;
}

/*
Getter for interval_counter vector object.
*/
std::vector<int> IntervalManager::get_interval_counter() {
	return this->interval_counter;
}

/*
Getter for interval_bound_low vector object.
*/
std::vector<double> IntervalManager::get_intervals_bound_low() {
	return this->interval_bound_low;
}

/*
Getter for interval_bound_up vector object.
*/
std::vector<double> IntervalManager::get_intervals_bound_up() {
	return this->interval_bound_up;
}

/*
Returns lower boundary of first interval. Useful for informing user about interval scaling.
*/
double IntervalManager::get_first_interval_bound_low()
{
	return this->interval_bound_low[0];
}

/*
Returns upper boundary of first interval.
*/
double IntervalManager::get_first_interval_bound_up()
{
	return this->interval_bound_up[0];
}

/*
Returns lower boundary of last interval.
*/
double IntervalManager::get_last_interval_bound_low()
{
	return this->interval_bound_low[this->interval_count-1];
}

/*
Returns upper boundary of last interval.
*/
double IntervalManager::get_last_interval_bound_up()
{
	return this->interval_bound_up[this->interval_count - 1];
}

/*
Setter for interval counters.
std::vector<int> interval_counter interval_counter = contains numeric counter of occurrence for each interval
*/
void IntervalManager::set_interval_counter(std::vector<int> interval_counter)
{
	this->interval_counter = interval_counter;
}
