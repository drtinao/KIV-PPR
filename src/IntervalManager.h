#pragma once
#include <vector>
class IntervalManager
{
	private:
		//constructor variables - START
		double min_value_data; //minimum value present in dataset
		double max_value_data; //maximum value present in dataset
		long count; //total count of VALID numbers in dataset (std::fpclassify gives FP_NORMAL / FP_ZERO)
		//constructor variables - END

		bool min_value_neg; //if dataset minimum value is < 0, then true - else false

		double interval_size; //size of each interval
		int interval_count; //total count of available intervals
		std::vector<int> interval_counter; //interval counters for numbers - each index contains count of items present in respective interval
		std::vector<double> interval_bound_low; //array which contains calculated lower boundaries for each interval
		std::vector<double> interval_bound_up; //array with calculated upper boundary for each interval

	public:
		IntervalManager(double min_value_data, double max_value_data, long count); //takes given values and calculates boundaries of each interval
		void merge_intervals(); //merges intervals so that every resulting interval has count >= 5
		void print_interval_cont_debug(); //prints count of numbers in each interval, usable mainly for debug, but looks nice
		double get_interval_size(); //gets size of each interval
		int get_interval_count(); //returns total count of available intervals
		std::vector<int> get_interval_counter(); //returns vector with counter for each interval
		std::vector<double> get_intervals_bound_low(); //gets interval lower boundaries
		std::vector<double> get_intervals_bound_up(); //gets interval upper boundaries
		double get_first_interval_bound_low(); //gets FIRST interval lower boundary (user info)
		double get_first_interval_bound_up(); //gets FIRST interval upper boundary (user info)
		double get_last_interval_bound_low(); //gets LAST interval lower boundary (user info)
		double get_last_interval_bound_up(); //gets LAST interval uper boundary (user info)
		void set_interval_counter(std::vector<int> interval_counter); //sets counter for each interval
};

