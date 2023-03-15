#pragma once
#include "Structures.h"
#include <vector>
#include "IntervalManager.h"
#include "DecisionDist.h"
#include "NormalDistrib.h"
#include "ExponentialDistrib.h"
#include "UniformDistrib.h"
#include "PoissonDistrib.h"

//contains functions related to chi square goodness of fit test
class ChiSquareManager
{
private:
	//constructor variables - START
	long count; //total count of VALID numbers in dataset (std::fpclassify gives FP_NORMAL / FP_ZERO)
	double avg; //average of dataset
	int interval_count; //total count of available intervals
	//constructor variables - END

public:
	ChiSquareManager(long count, double avg, int interval_count); //constructor expects count of valid nums, average and count of avail. intervals into which numbers are sorted
	double calc_expected_prob(double d_func_res_prev, double d_func_res_next); //calculates expected probability for given interval
	double calc_expected_freq(double expec_prob); //calculates expected frequency for given interval
	double calc_chi_square_formula(int ni, double expect_freq); //calculates formula specific for chi-square goodness of fit test
	double calc_test_crit(std::vector<double> calc_chi_interval); //sums calculated chi-square formulas for all of the intervals and returns result

	std::vector<double> calc_expected_prob_bulk(std::vector<double> d_func_res); //bulk calculation of expected probabilities
	std::vector<double> calc_expected_freq_bulk(std::vector<double> expec_prob_res); //calc expected frequencies for more intervals
	std::vector<double> calc_chi_square_formula_bulk(std::vector<int> ni, std::vector<double> expect_freq); //calculates chi-square formula for every given interval
	chi_win_res_struct* pick_lowest_test_crit(chi_crit_res_struct* chi_test_crit_res); //picks distribution with lowest calculated test criterium
	void print_chi_crit_res(chi_crit_res_struct* chi_crit_res); //prints all results from chi_crit_res structure
	void print_chi_win_res(chi_win_res_struct* chi_win_res); //prints information regarding to winning distribution
	void print_chi_part_res(chi_part_res_struct* chi_part_res); //prints partial result of chi square test
	chi_part_res_struct* calc_distrib_func(IntervalManager* intervalManager, DecisionDist* decisionDist); //calculates distribution function for distributions, which input numbers could potencially represent
	chi_part_res_struct* calc_expected_prob_all_valid_dist(chi_part_res_struct* distrib_func_res); //calc expected probability for distributions for which distribution function results are defined in input struct
	chi_part_res_struct* calc_expected_freq_all_valid_dist(chi_part_res_struct* exp_prob_res); //calc expected frequencies for distributions for which distribution function results are defined in input struct
	chi_part_res_struct* calc_chi_formula_all_valid_dist(IntervalManager* intervalManager, chi_part_res_struct* exp_freq_res); //calc chi square formula for defined distributions
	chi_crit_res_struct* calc_chi_test_crit_all_valid_dist(chi_part_res_struct* chi_formula_res); //calc test criteria for defined distribution
};