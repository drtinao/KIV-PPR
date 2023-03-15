#pragma once
#include<vector>
#include<future>
#include<atomic>
#include <CL/cl.h>
#if __has_include(<CL/opencl.hpp>)
# include <CL/opencl.hpp>
#else
# include "opencl.hpp"
#endif

/*
Defines type of devices on which will be chi-square goodness of fit test executed.
*/
enum compute_type {
    ALL, //run computation on all available davices
    SMP, //compute on more CPU threads
    OPENCL //use specific OpenCL devices
};

/*
Enum lists names of all distributions which can input dataset potentially represent.
*/
enum distribution_list {
    UNIFORM,
    NORMAL,
    EXPONENTIAL,
    POISSON
};

/*
Each item in enum describes characteristics of numbers present in input dataset.
Point is that some distributions cannot contain some type of numbers. Therefore we can ommit calculation of chi-square for some distributions if decimal point / negative numbers are present in dataset.
*/
enum distribution_limit {
    POSITIVE_INTEGER, //all numbers in dataset are positive integers => calc every distribution (uniform, normal, exponential, Poisson)
    POSITIVE_DECIMAL, //all numbers positive, but at least one decimal point number present => ommit Poisson (calc uniform, normal, exponential)
    NEGATIVE //at least one negative number present => calc uniform + normal
};

/* 
Structure carries result of particular step which must be performed in order to calc chi-square goodness of fit test.
Particular step can be: calculation of distribution function / expected probability / expected frequency and solving of chi-square formula.
Structure contains arrays, each array represents result for one distribution (obvious from name). Size of all arrays is the same and is stored in res_arr_size variable.
In some cases, calculation of exponential and Poisson distribution can be ommited => array refers to NULL pointer in that case.
*/
struct chi_part_res_struct {
    int res_arr_size = -1; //count of items present in each of following struct arrays
    distribution_limit sel_distribution_limit = distribution_limit::NEGATIVE; //detected distribution limits (enum distribution_limit)

    std::vector<double> uniform_res_arr; //arr with uniform distribution results
    std::vector<double> normal_res_arr; //-||- normal distribution
    std::vector<double> exponential_res_arr; //-||- exponential distribution, potencionally NULL
    std::vector<double> poisson_res_arr; //-||- Poisson distribution, potencionally NULL
};

/*
Structure contains calculated test criterium for each distribution. Test criterium is defined as sum of chi-square formula for all defined intervals.
*/
struct chi_crit_res_struct {
    distribution_limit sel_distribution_limit; //detected distribution limits (enum distribution_limit)

    double uniform_res; //result of test criteria (ie. sum of chi square formula for each interval) for uniform distribution
    double normal_res; //-||- normal distribution
    double exponential_res; //-||- exponential distribution
    double poisson_res; //-||- Poisson distribution
};

/*
Contains information regarding to distribution with lowest calculated test criterium among all distributions - ie. distribution to which is dataset closest.
*/
struct chi_win_res_struct {
    distribution_list win_distribution; //name of distribution with lowest test criteria

    double chi_crit_res; //result of test criteria
    double* charasteristic_dist; //values which characterize the specific distribution (could be different for every distribution)
};

/*
Information regarding to one OpenCL device which is allowed to compute.
*/
struct cl_dev_stuff_struct {
    cl::Device dev; //OpenCL device info
    cl::Context dev_context; //OpenCL device context
    cl::CommandQueue dev_queue; //command queue for the device

    std::shared_future<void> current_task; //task which is currently executed by device

    //programs built for specific OpenCL device
    cl::Program prog_min_max_dec_point_neg_num; //first round of algo
    cl::Program prog_add_nums_intervals; //second round of algo

    //kernels
    cl::Kernel ker_min_max_dec_point_neg_num; //first round of algo
    cl::Kernel ker_add_nums_intervals; //second round of algo

    //buffers min_max_dec_point_neg_num + add_nums_to_intervals
    cl::Buffer input_nums_buf; //min_max_dec_point_neg_num + add_nums_to_intervals
    cl::Buffer res_min_pos_buf; //minimum value + sign
    cl::Buffer res_max_pos_buf; //maximum value + sign
    cl::Buffer res_min_neg_buf; //maximum value - sign
    cl::Buffer res_max_neg_buf; //maximum value - sign

    //buffers - min_max_dec_point_neg_num specific
    cl::Buffer res_dec_point_num_buf; //decimal point number found by device flag

    //buffers - add_nums_to_intervals specific
    cl::Buffer output_intervals_buf; //output intervals into which numbers are sorted
};