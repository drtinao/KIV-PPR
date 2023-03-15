#pragma once

//used for making decisions related to closest distribution
class DecisionDist
{
	private:
		bool dec_point_num; //true if atleast one number in dataset decimal point, else false
		bool negative_num; //true if atleast one number in dataset negative, else false

		double min_value; //minimum value found in dataset
		double max_value; //maximum value found in dataset

		long count; //total count of VALID numbers in dataset (std::fpclassify gives FP_NORMAL / FP_ZERO)

		double avg; //average of dataset
		double variance; //variance of dataset
		double m_sum; //specific for Welfords online algorithm
		bool normalize; //true if normalization should be applied
		double normalize_val; //constant which is used for normalization, dividing - usually dataset max
		double std_dev; //standard deviation of dataset (must be calculated after variance is determined)
		long welford_counter; //count of numbers processed by welfords algo
	public:
		void update_count(int count_to_add); //increase counter of valid number by given number
		void update_avg_var(double num); //update avg + variance using Welfords online algorithm
		void calc_std_dev(); //calculates standard deviance of dataset (variance must be determined before)
		double get_min_value(); //getter for min_value variable
		double get_max_value(); //getter for max_value variable.
		double get_avg(); //getter for dataset average
		double get_std_dev(); //getter for standard deviation of dataset
		long get_count(); //getter for count of valid numbers
		void enable_avg_var_normalization(double dataset_max); //enables normalization using given number
		void finalize_avg_std_dev_normalization(); //multiplies using number used for normalization
		void reset_count(); //resets valid number counter
		bool get_dec_point_num(); //tells whether dataset contains decimal point number
		bool get_negative_num(); //tells whether dataset contains negative number
		void set_min_value(double min_value); //setter for dataset min value
		void set_max_value(double max_value); //setter for dataset max value
		void set_dec_point_num(bool found); //setter for dec_point_num variable
		void set_negative_num(bool found); //setter for negative_num variable
};