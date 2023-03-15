#include "cl_defines.h"
#include <iostream>
#include <fstream>
#include "ChiSquareManager.h"
#include "PoissonDistrib.h"

/*
Constructor requires total count of valid numbers in dataset which are then used for calculations regarding to chi-square goodness of fit test.
long count = total count of VALID numbers in dataset (std::fpclassify gives FP_NORMAL / FP_ZERO)
double avg = average of dataset (required for Poisson calculation)
int interval_count = count of available intervals into which are number sorted
*/
ChiSquareManager::ChiSquareManager(long count, double avg, int interval_count)
{
	this->count = count;
	this->avg = avg;
	this->interval_count = interval_count;
}

/*
Function calculates expected probability for given interval.
Probability function is calculated as difference between result of distribution function for current interval - result of distribution function for previous interval.
Value of probability function for first interval is always the same as distribution function result.
double d_func_res_prev = result of distribution function for interval n
double d_func_res_next = result of distribution function for interval n + 1
*/
double ChiSquareManager::calc_expected_prob(double d_func_res_prev, double d_func_res_next)
{
	return d_func_res_next - d_func_res_prev;
}

/*
Calculates expected frequency for given interval.
Expected frequency is defined as: count of all valid numbers * expected probability for spec. interval.
double expec_prob = expected probability for given interval
*/
double ChiSquareManager::calc_expected_freq(double expec_prob)
{
	return this->count * expec_prob;
}

/*
Calculates formula specific for chi-square goodness of fit test. Formula is defined as: (ni - expectFreq) ^ 2 / expectFreq.
int ni = number of occurence for specific interval
double expect_freq = expected frequency for given interval
*/
double ChiSquareManager::calc_chi_square_formula(int ni, double expect_freq)
{
	return ((ni - expect_freq) * (ni - expect_freq)) / expect_freq;
}

/*
Function sums calculated chi-square formulas for all of the intervals and returns result.
Result of chi-square sum is referred to as test criteria. Distribution with lowest test criteria will be considered as the closest one.
std::vector<double> calc_chi_interval = array with calculated chi-square formula for each of the intervals
*/
double ChiSquareManager::calc_test_crit(std::vector<double> calc_chi_interval)
{
	double chi_square_formula_sum = 0; //sum of chi square formulas for all intervals

	for (int i = 0; i < this->interval_count; i++) { //go through chi-square formula results for each interval and sum them up
		chi_square_formula_sum += calc_chi_interval[i];
	}

	return chi_square_formula_sum;
}

/*
Performs bulk calculation of expected probabilities. Ie. calculation is performed for every given interval and array with results is returned (1 index = result for one interval).
First index, which contains value of expected probability for first interval, will always have same value as distribution function result. Other vals are calculated by respective function.
double* d_func_res = array with results of distribution function for all available intervals
*/
std::vector<double> ChiSquareManager::calc_expected_prob_bulk(std::vector<double> d_func_res)
{
	std::vector<double> expected_probs(this->interval_count); //create array for expected probabilities (one index = expected prob. for one interval)

	expected_probs[0] = d_func_res[0]; //probability for first interval is the same as distribution function result for given interval
	double calculated_prob; //calculated probability for one interval
	bool prob_pos_num_enc = false; //true if atleast one positive probability (> 0) is encountered 

	for (int i = 1; i < this->interval_count; i++) { //calculate expected probability for rest of the intervals
		int prev_index = i - 1;
		calculated_prob = this->calc_expected_prob(d_func_res[prev_index], d_func_res[i]);
		if (calculated_prob > 0) { //valid probability, assign
			expected_probs[i] = calculated_prob;
			prob_pos_num_enc = true;
		}
		else if(prob_pos_num_enc == true) { //probability cannot be 0, if any interval remaining, divide latest valid probability into remaining intervals
			int remaining_intervals = this->interval_count - i;
			int prev_interv = i - 1;
			int remaining_interv = remaining_intervals + 1;
			double prob_per_interval = expected_probs[prev_interv] / remaining_interv; //divide latest valid probability into remaining intervals

			for (int j = i-1; j < this->interval_count; j++) {
				expected_probs[j] = prob_per_interval;
			}
			break;
		}
	}

	if (expected_probs[0] > 0) { //first probability is positive, ok
		return expected_probs;
	}
	else {
		for (int i = 0; i < this->interval_count; i++) { //find first positive probability and divide it into previous intervals
			if (expected_probs[i] > 0) {
				int follow_interv = i + 1;
				double prob_per_interval = expected_probs[i] / (follow_interv); //divide first valid probability into previous intervals with 0 probability
				for (int j = i; j >= 0; j--) {
					expected_probs[j] = prob_per_interval;
				}
				break;
			}
		}
	}

	return expected_probs;
}

/*
Calculates expected frequencies for more intervals. Accepts array with expected probabilities, where each index contains probability for corresponsing interval.
Function returns array with expected frequencies, where each index contains value for corresponding interval.
std::vector<double> expec_prob_res = array with calculated expected probability for each interval
*/
std::vector<double> ChiSquareManager::calc_expected_freq_bulk(std::vector<double> expec_prob_res)
{
	std::vector<double> expected_freqs(this->interval_count); //array with expected frequencies (size is the same as input array)

	const int loop_end = this->interval_count;
	for (int i = 0; i < loop_end; i++) { //calculate expected frequency for every number present in input array (contains expected probability result)
		expected_freqs[i] = this->calc_expected_freq(expec_prob_res[i]);
	}

	return expected_freqs;
}

/*
Function does bulk calculation chi-square goodness of fit test, ie. calculates chi-square formula for every given interval.
Returns array with solved chi-square formula for all intervals, size is the same as input arrays.
std::vector<int> ni = array with number of occurence for each interval (indexing is same for both input arrays)
std::vector<double> expect_freq = array with expected frequency for each interval (indexing is same for both input arrays)
*/
std::vector<double> ChiSquareManager::calc_chi_square_formula_bulk(std::vector<int> ni, std::vector<double> expect_freq)
{
	std::vector<double> chiSquareFormulas(this->interval_count); //array with calculated chi-square formulas for every interval

	const int loop_end = this->interval_count;
	for (int i = 0; i < loop_end; i++) { //calculate chi-square formula for all intervals
		chiSquareFormulas[i] = this->calc_chi_square_formula(ni[i], expect_freq[i]);
	}

	return chiSquareFormulas;
}

/*
Function picks distribution with lowest calculated test criterium. This distribution is considered (according to chi-square test) as distribution to which is dataset closest.
chi_crit_res_struct* chi_test_crit_res = contains calculated test criterium for each distribution (exponential / Poisson omitted if distribution_limit is POSITIVE_DECIMAL / NEGATIVE)
*/
chi_win_res_struct* ChiSquareManager::pick_lowest_test_crit(chi_crit_res_struct* chi_test_crit_res)
{
	//retrieve calculated test criterium for each distribution
	distribution_limit sel_distribution_limit = chi_test_crit_res->sel_distribution_limit;
	double uniform_res = chi_test_crit_res->uniform_res;
	double normal_res = chi_test_crit_res->normal_res;
	double exponential_res = chi_test_crit_res->exponential_res;
	double poisson_res = chi_test_crit_res->poisson_res;

	chi_win_res_struct* chi_win_dist = new chi_win_res_struct;

	//init with one distribution - uniform / normal always calculated
	if (uniform_res < normal_res) {
		chi_win_dist->win_distribution = distribution_list::UNIFORM;
		chi_win_dist->chi_crit_res = uniform_res; //minimum test criterium found (always > 0)
	}
	else {
		chi_win_dist->win_distribution = distribution_list::NORMAL;
		chi_win_dist->chi_crit_res = normal_res;
	}

	switch (sel_distribution_limit) {
		case POSITIVE_INTEGER: //compare exponential + Poisson
			if (exponential_res < chi_win_dist->chi_crit_res) {
				chi_win_dist->win_distribution = distribution_list::EXPONENTIAL;
				chi_win_dist->chi_crit_res = exponential_res;
			}


			if (poisson_res < chi_win_dist->chi_crit_res) {
				chi_win_dist->win_distribution = distribution_list::POISSON;
				chi_win_dist->chi_crit_res = poisson_res;
			}
			break;
		case POSITIVE_DECIMAL: //compare exponential
			if (exponential_res < chi_win_dist->chi_crit_res) {
				chi_win_dist->win_distribution = distribution_list::EXPONENTIAL;
				chi_win_dist->chi_crit_res = exponential_res;
			}
			break;
		case NEGATIVE:
			break;
		default:
			std::cout << "ERROR - unknown distribution characteristics" << std::endl;
	}

	return chi_win_dist;
}

/*
Prints all results from chi_crit_res structure, which contains calculated test criteria (chi-square goodness of fit test).
chi_crit_res_struct* chi_crit_res = structure contains chi-square test criteria results for each distribution
*/
void ChiSquareManager::print_chi_crit_res(chi_crit_res_struct* chi_crit_res) {
	//get doubles with calculated test criteria for each distribution
	distribution_limit sel_distribution_limit = chi_crit_res->sel_distribution_limit;
	double uniform_res = chi_crit_res->uniform_res;
	double normal_res = chi_crit_res->normal_res;
	double exponential_res = chi_crit_res->exponential_res;
	double poisson_res = chi_crit_res->poisson_res;

	std::cout << "uniform result: " << uniform_res << std::endl;
	std::cout << "normal result: " << normal_res << std::endl;
	switch (chi_crit_res->sel_distribution_limit) {
	case POSITIVE_INTEGER:
		std::cout << "exponential result: " << exponential_res << std::endl;
		std::cout << "Poisson result: " << poisson_res << std::endl;
		break;
	case POSITIVE_DECIMAL:
		std::cout << "exponential result: " << exponential_res << std::endl;
		break;
	case NEGATIVE:
		break;
	default:
		std::cout << "ERROR - unknown distribution characteristics" << std::endl;
	}
}

/*
Presents content of structure chi_win_res, which contains details regarding to closest distribution, in user readable form.
chi_win_res_struct* chi_win_res = contains details regarding to distribution with lowest calculated test criterium (winning distribution)
*/
void ChiSquareManager::print_chi_win_res(chi_win_res_struct* chi_win_res) {
	distribution_list closest_distribution = chi_win_res->win_distribution; //lowest test criteria distribution
	double chi_crit_res = chi_win_res->chi_crit_res; //test criterium for given distribution

	std::cout << "closest distribution is: ";
	switch (closest_distribution) {
	case UNIFORM:
		std::cout << "uniform" << std::endl;
		break;
	case NORMAL:
		std::cout << "normal" << std::endl;
		break;
	case EXPONENTIAL:
		std::cout << "exponential" << std::endl;
		break;
	case POISSON:
		std::cout << "Poisson" << std::endl;
		break;
	default:
		std::cout << "ERROR - unknown distribution" << std::endl;
	}
	std::cout << "test criterium: " << chi_crit_res << std::endl;
}

/*
Prints results present in chi_crit_res_struct structure. Results capture one particular step which is performed in order to to calc chi-square goodness of fit test.
chi_part_res_struct* chi_part_res = structure with partial chi-square test results
*/
void ChiSquareManager::print_chi_part_res(chi_part_res_struct* chi_part_res)
{
	//array with partial results for all distributions
	int res_arr_size = chi_part_res->res_arr_size; //size of all arrays with distribution function results
	distribution_limit sel_distribution_limit = chi_part_res->sel_distribution_limit;
	std::vector<double> uniform_res_arr = chi_part_res->uniform_res_arr;
	std::vector<double> normal_res_arr = chi_part_res->normal_res_arr;
	std::vector<double> exponential_res_arr = chi_part_res->exponential_res_arr;
	std::vector<double> poisson_res_arr = chi_part_res->poisson_res_arr;

	for (int i = 0; i < res_arr_size; i++) { //uniform result - always
		std::cout << "index: " << i << ", uniform result: " << uniform_res_arr[i] << std::endl;
	}

	for (int i = 0; i < res_arr_size; i++) { //normal result - always
		std::cout << "index: " << i << ", normal result: " << normal_res_arr[i] << std::endl;
	}

	switch (chi_part_res->sel_distribution_limit) {
	case POSITIVE_INTEGER:
		for (int i = 0; i < res_arr_size; i++) { //exponential - only positive nums in dataset
			std::cout << "index: " << i << ", exponential result: " << exponential_res_arr[i] << std::endl;
		}

		for (int i = 0; i < res_arr_size; i++) { //Poisson - only positive + integer nums in dataset
			std::cout << "index: " << i << ", Poisson result: " << poisson_res_arr[i] << std::endl;
		}
		break;
	case POSITIVE_DECIMAL:
		for (int i = 0; i < res_arr_size; i++) { //exponential - only positive nums in dataset
			std::cout << "index: " << i << ", exponential result: " << exponential_res_arr[i] << std::endl;
		}
		break;
	case NEGATIVE:
		break;
	default:
		std::cout << "ERROR - unknown distribution characteristics" << std::endl;
	}
}

/*
Calculates chi-square test criterium for each valid distribution which has results in chi_formula_res struct avaiable.
chi_part_res_struct* chi_formula_res = structure with calculated chi-square formula for valid distributions
*/
chi_crit_res_struct* ChiSquareManager::calc_chi_test_crit_all_valid_dist(chi_part_res_struct* chi_formula_res)
{
	//get results of chi-square formulas for each distribution
	distribution_limit sel_distribution_limit = chi_formula_res->sel_distribution_limit;
	std::vector<double> uniform_chi_formula_arr = chi_formula_res->uniform_res_arr;
	std::vector<double> normal_chi_formula_arr = chi_formula_res->normal_res_arr;
	std::vector<double> exponential_chi_formula_arr = chi_formula_res->exponential_res_arr;
	std::vector<double> poisson_chi_formula_arr = chi_formula_res->poisson_res_arr;

	//returned structure will contain calculated chi-square test criterium for each valid distribution
	chi_crit_res_struct* chi_crit_res = new chi_crit_res_struct;
	chi_crit_res->sel_distribution_limit = sel_distribution_limit;

	chi_crit_res->uniform_res = this->calc_test_crit(uniform_chi_formula_arr); //uniform - always
	chi_crit_res->normal_res = this->calc_test_crit(normal_chi_formula_arr); //normal - always
	switch (sel_distribution_limit) {
	case POSITIVE_INTEGER:
		chi_crit_res->exponential_res = this->calc_test_crit(exponential_chi_formula_arr); //exponential defined in input (positive numbers)
		chi_crit_res->poisson_res = this->calc_test_crit(poisson_chi_formula_arr); //Poisson defined in input (positive and integer numbers)
		break;
	case POSITIVE_DECIMAL:
		chi_crit_res->exponential_res = this->calc_test_crit(exponential_chi_formula_arr);
		break;
	case NEGATIVE:
		break;
	default:
		std::cout << "ERROR - unknown distribution characteristics" << std::endl;
	}

	return chi_crit_res;
}

/*
Calculates expected probability for distributions for which distribution function results are defined in input struct.
Ie. pointer to array with corresponding distribution function must not be set to NULL (results not calculated - could be exponential + Poisson).
chi_part_res_struct* distrib_func_res = pointer to struct which contains calculated distribution functions for all intervals
*/
chi_part_res_struct* ChiSquareManager::calc_expected_prob_all_valid_dist(chi_part_res_struct* distrib_func_res)
{
	//collect arrays with results of distribution function for all defined distributions
	int dist_res_arr_size = distrib_func_res->res_arr_size; //size of all arrays with distribution function results
	distribution_limit sel_distribution_limit = distrib_func_res->sel_distribution_limit;
	std::vector<double> uniform_dist_res_arr = distrib_func_res->uniform_res_arr;
	std::vector<double> normal_dist_res_arr = distrib_func_res->normal_res_arr;
	std::vector<double> exponential_dist_res_arr = distrib_func_res->exponential_res_arr;
	std::vector<double> poisson_dist_res_arr = distrib_func_res->poisson_res_arr;

	//create struct which will contain expected probability results for all relevant distribution functions
	chi_part_res_struct* exp_prob_res = new chi_part_res_struct;
	exp_prob_res->res_arr_size = dist_res_arr_size; //size of each array with expected probabilities will be the same as size of arrays with distribution function results (= interval count)
	exp_prob_res->sel_distribution_limit = sel_distribution_limit;

	exp_prob_res->uniform_res_arr = this->calc_expected_prob_bulk(uniform_dist_res_arr); //uniform distribution has no restrictions - always calculate expected probability
	exp_prob_res->normal_res_arr = this->calc_expected_prob_bulk(normal_dist_res_arr); //-||- normal distribution
	switch (sel_distribution_limit) {
	case POSITIVE_INTEGER:
		exp_prob_res->exponential_res_arr = this->calc_expected_prob_bulk(exponential_dist_res_arr); //res for exponential distribution present => only positive numbers in dataset
		exp_prob_res->poisson_res_arr = poisson_dist_res_arr; //res for Poisson distribution present => positive + integer numbers in dataset; Poisson values of expected probability are equal to distribution function values
		break;
	case POSITIVE_DECIMAL:
		exp_prob_res->exponential_res_arr = this->calc_expected_prob_bulk(exponential_dist_res_arr);
		break;
	case NEGATIVE:
		break;
	default:
		std::cout << "ERROR - unknown distribution characteristics" << std::endl;
	}

	return exp_prob_res;
}

/*
Function calculates expected frequencies for every distribution which has defined valid expected probability in exp_prob_res structure.
Ie. pointer in mentioned structure is not set to NULL for particular distribution (NULL could be exponential and Poisson).
chi_part_res_struct* exp_prob_res = pointer to structure with expected probability results
*/
chi_part_res_struct* ChiSquareManager::calc_expected_freq_all_valid_dist(chi_part_res_struct* exp_prob_res)
{
	//collect expected probability results from input structure
	int exp_prob_arr_size = exp_prob_res->res_arr_size;
	distribution_limit sel_distribution_limit = exp_prob_res->sel_distribution_limit;
	std::vector<double> uniform_exp_prob_arr = exp_prob_res->uniform_res_arr;
	std::vector<double> normal_exp_prob_arr = exp_prob_res->normal_res_arr;
	std::vector<double> exponential_exp_prob_arr = exp_prob_res->exponential_res_arr;
	std::vector<double> poisson_exp_prob_arr = exp_prob_res->poisson_res_arr;

	//return structure will contain results of expected frequency for all distributions defined in input structure
	chi_part_res_struct* exp_freq_res = new chi_part_res_struct;
	exp_freq_res->res_arr_size = exp_prob_arr_size;
	exp_freq_res->sel_distribution_limit = sel_distribution_limit;

	exp_freq_res->uniform_res_arr = this->calc_expected_freq_bulk(uniform_exp_prob_arr); //always calculate uniform distribution - no restrictions
	exp_freq_res->normal_res_arr = this->calc_expected_freq_bulk(normal_exp_prob_arr); //-||- normal distribution
	switch (sel_distribution_limit) {
	case POSITIVE_INTEGER:
		exp_freq_res->exponential_res_arr = this->calc_expected_freq_bulk(exponential_exp_prob_arr); //calc exponential distribution if present in input structure (only positive nums in dataset)
		exp_freq_res->poisson_res_arr = this->calc_expected_freq_bulk(poisson_exp_prob_arr); //-||- Poisson distribution (only positive + integer numbers in dataset)
		break;
	case POSITIVE_DECIMAL:
		exp_freq_res->exponential_res_arr = this->calc_expected_freq_bulk(exponential_exp_prob_arr);
		break;
	case NEGATIVE:
		break;
	default:
		std::cout << "ERROR - unknown distribution characteristics" << std::endl;
	}

	return exp_freq_res;
}

/*
Function performs calculation of chi-square specific formula for valid distributions which have defined expected frequencies in input structure.
Ie. calculation is performed for distributions which do not have pointer set to NULL in the exp_freq_res structure (could be exponential / Poisson).
IntervalManager* intervalManager = functions which are responsible for managing content of intervals into which are numbers sorted
chi_part_res_struct* exp_freq_res = pointer to structure with expected frequency results
*/
chi_part_res_struct* ChiSquareManager::calc_chi_formula_all_valid_dist(IntervalManager* intervalManager, chi_part_res_struct* exp_freq_res)
{
	std::vector<int> interval_counter = intervalManager->get_interval_counter(); //get array with counter for each interval, mandatory for chi-square formula calc

	//retrieve expected frequency results from input structure
	int exp_freq_arr_size = exp_freq_res->res_arr_size;
	distribution_limit sel_distribution_limit = exp_freq_res->sel_distribution_limit;
	std::vector<double> uniform_exp_freq_arr = exp_freq_res->uniform_res_arr;
	std::vector<double> normal_exp_freq_arr = exp_freq_res->normal_res_arr;
	std::vector<double> exponential_exp_freq_arr = exp_freq_res->exponential_res_arr;
	std::vector<double> poisson_exp_freq_arr = exp_freq_res->poisson_res_arr;

	//output structure will contain calculated chi-square formula for all distributions which have expected frequencies defined in exp_freq_res
	chi_part_res_struct* chi_formula_res = new chi_part_res_struct;
	chi_formula_res->res_arr_size = exp_freq_arr_size;
	chi_formula_res->sel_distribution_limit = sel_distribution_limit;

	chi_formula_res->uniform_res_arr = this->calc_chi_square_formula_bulk(interval_counter, uniform_exp_freq_arr); //uniform - always
	chi_formula_res->normal_res_arr = this->calc_chi_square_formula_bulk(interval_counter, normal_exp_freq_arr); //normal - always
	switch (sel_distribution_limit) {
	case POSITIVE_INTEGER:
		chi_formula_res->exponential_res_arr = this->calc_chi_square_formula_bulk(interval_counter, exponential_exp_freq_arr); //exponential - if present in input (positive numbers)
		chi_formula_res->poisson_res_arr = this->calc_chi_square_formula_bulk(interval_counter, poisson_exp_freq_arr); //Poisson - if present in input (positive + integer numbers)
		break;
	case POSITIVE_DECIMAL:
		chi_formula_res->exponential_res_arr = this->calc_chi_square_formula_bulk(interval_counter, exponential_exp_freq_arr);
		break;
	case NEGATIVE:
		break;
	default:
		std::cout << "ERROR - unknown distribution characteristics" << std::endl;
	}

	return chi_formula_res;
}

/*
Function calculates distribution function for distributions, which input numbers could potencially represent.
Ie. if only positive integer numbers found in file, calculate all distribution functions (uniform, normal, exponential, Poisson)
- if decimal point numbers found, ommit Possion => calculate uniform, normal, exponential
- if decimal point + negative numbers found, ommit Poisson + exponential => calculate uniform, normal
IntervalManager* intervalManager = functions which are responsible for managing content of intervals into which are numbers sorted
DecisionDist* decisionDist = functions which help to decide which distribution is closest
*/
chi_part_res_struct* ChiSquareManager::calc_distrib_func(IntervalManager* intervalManager, DecisionDist* decisionDist) {
	long interval_count = intervalManager->get_interval_count(); //count of all intervals into which were numbers sorted
	std::vector<double> interval_bound_low = intervalManager->get_intervals_bound_low(); //lower boundaries of all intervals
	std::vector<double> interval_bound_up = intervalManager->get_intervals_bound_up(); //upper boundaries of all intervals

	//collect dataset values which are mandatory for calculating distribution functions
	double min_value_dataset = decisionDist->get_min_value();
	double max_value_dataset = decisionDist->get_max_value();
	double avg_dataset = decisionDist->get_avg();
	double std_dev_dataset = decisionDist->get_std_dev();
	long count_dataset = decisionDist->get_count();

	//create struct which will contain results for all relevant distribution functions
	chi_part_res_struct* distrib_func_res = new chi_part_res_struct;
	distrib_func_res->res_arr_size = interval_count;

	//uniform, calculate always
	UniformDistrib* uniformDistrib = new UniformDistrib(min_value_dataset / decisionDist->get_max_value(), max_value_dataset / decisionDist->get_max_value()); //normalize to get lower values (Debian iso fix)
	std::vector<double> uniform_res_arr(interval_count); //create array for uniform distribution function results (size is the same as array with counter for each interval)
	const int loop_end = this->interval_count;
	for (int i = 0; i < loop_end; i++) { //calculate uniform distrib. func. for all created intervals
		uniform_res_arr[i] = uniformDistrib->calc_distrib_fun(interval_bound_up[i] / decisionDist->get_max_value()); //normalize to get lower values (Debian iso fix)
	}
	distrib_func_res->uniform_res_arr = uniform_res_arr; //add uniform results to struct

	//normal, calculate always
	NormalDistrib* normalDistrib = new NormalDistrib(avg_dataset / decisionDist->get_max_value(), std_dev_dataset / decisionDist->get_max_value()); //normalize to get lower values (Debian iso fix)

	std::vector<double> standard_val_res(interval_count);
	for (int i = 0; i < loop_end; i++) { //standardize value for each interval
		standard_val_res[i] = normalDistrib->standardize_interval(interval_bound_up[i] / decisionDist->get_max_value()); //normalize to get lower values (Debian iso fix)
	}

	std::vector<double> normal_res_arr(interval_count); //array for calculated results of normal distribution function (must be standardized before..)
	for (int i = 0; i < loop_end; i++) { //find result of distribution function in lookup table - for each interval
		normal_res_arr[i] = normalDistrib->find_distrib_func(standard_val_res[i]);
	}
	distrib_func_res->normal_res_arr = normal_res_arr; //add normal results to struct

	if (decisionDist->get_negative_num() == false) { //only positive numbers present in dataset, calc exponential distribution function
		ExponentialDistrib* exponentialDistrib = new ExponentialDistrib(avg_dataset);
		std::vector<double> exponential_res_arr(interval_count); //array for exponential distrib. function results
		for (int i = 0; i < loop_end; i++) {
			exponential_res_arr[i] = exponentialDistrib->calc_distrib_func(interval_bound_up[i]);
		}
		distrib_func_res->exponential_res_arr = exponential_res_arr; //add exponential results to struct

		if (decisionDist->get_dec_point_num() == false) { //only positive + integer number present in dataset, calc Poisson distribution function
			PoissonDistrib* poissonDistrib = new PoissonDistrib(avg_dataset);
			std::vector<double> poisson_res_arr(interval_count); //array for results of poisson distribution function

			int last_up_int = -1; //init - Poisson cannot ever be -1
			int bound_low_int; //integer lower boundary of interval
			int bound_up_int; //integer upper boundary interval
			for (int i = 0; i < loop_end; i++) { //calculate Poisson for each interval
				//interval boundaries for Poisson must be integer
				bound_low_int = (int)interval_bound_low[i];
				bound_up_int = (int)interval_bound_up[i];

				if (last_up_int == bound_low_int) { //upper boundary of last processed integer is equal with following lower boundary, increase by 1
					bound_low_int += 1;

					if (bound_low_int > bound_up_int) { //skip current interval, no integer numbers present
						continue;
					}
				}

				poisson_res_arr[i] = poissonDistrib->calc_distrib_func_interval(bound_low_int, bound_up_int);
				last_up_int = bound_up_int;
			}
			distrib_func_res->poisson_res_arr = poisson_res_arr; //add Poisson results to struct

			distrib_func_res->sel_distribution_limit = distribution_limit::POSITIVE_INTEGER;
		}
		else { //decimal point num present in dataset
			distrib_func_res->sel_distribution_limit = distribution_limit::POSITIVE_DECIMAL;
		}
	}
	else { //negative num present in dataset
		distrib_func_res->sel_distribution_limit = distribution_limit::NEGATIVE;
	}
	return distrib_func_res;
}
