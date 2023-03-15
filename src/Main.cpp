#include <fstream>
#include <cmath>
#include "Main.h"
#include "Initializer.h"
#include "const.h"
#include "Watchdog.h"
#include "OpenCLManager.h"

/*
Function main is serves as entrypoint of application. Function expectes >= 3 arguments: program name + path to file + computing type.
Instead of last mentioned parameter (computing type), individual names of available OpenCL computing devices can be used.
char argc = count of arguments
char** argv = array with given arguments
*/
int main(int argc, char** argv)
{
    OpenCLManager* openCLMan = new OpenCLManager();
    Initializer* initializer = new Initializer(argc, argv, openCLMan); //Initializer class contains utils for program initialization (sets selected computing type etc.)
    bool initRes = initializer->init_via_args(); //init program using user given arguments
    if (initRes == false) { //ERROR - init failed, end
        return -1;
    }

    //input arguments valid, continue with program execution
    initializer->print_init_info();
    
    FileHelper* fileHelper = new FileHelper(initializer->get_input_file_name()); //contains utils for working with file specified by user (reading, obtaining filesize etc.)
    DecisionDist* decisionDist = new DecisionDist();
    Farmer* farmer = new Farmer(initializer->get_sel_comp_type(), openCLMan->get_compute_cl_devices());

    Watchdog::get_instance()->start_watchdog(); //start watchdog
    perf_first_pass(fileHelper, decisionDist, farmer); //perform first pass of algo and print results
    print_first_pass_info(fileHelper, decisionDist);

    //second pass of algo setup - START
    double min_value_dataset = decisionDist->get_min_value();
    double max_value_dataset = decisionDist->get_max_value();
    long count_dataset = decisionDist->get_count();
    
    IntervalManager* intervalManager = new IntervalManager(min_value_dataset, max_value_dataset, count_dataset); //dataset stays the same
    openCLMan->alloc_add_nums_to_intervals_buffers(intervalManager->get_interval_count());
    decisionDist->enable_avg_var_normalization(max_value_dataset);

    decisionDist->reset_count();
    perf_second_pass(fileHelper, intervalManager, decisionDist, farmer);
    decisionDist->calc_std_dev();
    decisionDist->finalize_avg_std_dev_normalization();
    intervalManager->merge_intervals();
    print_second_pass_info(intervalManager, decisionDist);
    //second pass of algo setup - END

    //perform chi-square goodness of fit calculations
    ChiSquareManager* chiSquareMan = new ChiSquareManager(count_dataset, decisionDist->get_avg(), intervalManager->get_interval_count());
    perform_chi_square_calc(intervalManager, decisionDist, chiSquareMan);
    Watchdog::get_instance()->stop_watchdog(); //stop watchdog
}

/*
Function reads whole file and determines numeric values which can be acquired in first round of algorithm, namely:
- dataset minimum number
- dataset maximum number
- total count of valid numbers in dataset (std::fpclassify(num) returns FP_NORMAL or FP_ZERO)
- checks if atleast one number in dataset has decimal point
- checks if atleast one number in dataset is negative
FileHelper* fileHelper = contains functions regarding to files
DecisionDist* decisionDist = functions which help to decide which distribution is closest
Farmer* farmer = farmer (farmer-worker model) which keeps track of availability of workers, assigns work
*/
void perf_first_pass(FileHelper* fileHelper, DecisionDist* decisionDist, Farmer* farmer) {
    std::cout << "Performing first round of algorithm, please wait..." << std::endl;
    Watchdog::get_instance()->reset_timer();

    uintmax_t file_size = fileHelper->deter_file_size();
    uintmax_t cur_file_offset = 0; //current offset in traversed file
    std::vector<double> valid_nums;

    fileHelper->open_file_read();
    double first_num = fileHelper->read_part_file(cur_file_offset, 1)[0];

    farmer->prep_devs_min_max_dec_point_neg_num(first_num);
    while ((cur_file_offset + DOUBLE_READ_COUNT_ONCE * sizeof(double)) < file_size) { //read file, update offset
        std::vector<double> file_nums = fileHelper->read_part_file(cur_file_offset, DOUBLE_READ_COUNT_ONCE); //read doubles from file into array
        Watchdog::get_instance()->reset_timer();

        for (int i = 0; i < DOUBLE_READ_COUNT_ONCE; i++) {
            if (fileHelper->is_valid_num(file_nums[i])) { //only update if valid number
                valid_nums.push_back(file_nums[i]);
            }
        }

        if (valid_nums.size() > 0) {
            farmer->assign_min_max_dec_point_neg_num(valid_nums); //check for min, max, dec.point, negative numbers
            Watchdog::get_instance()->reset_timer();
            decisionDist->update_count(static_cast<int>(valid_nums.size())); //update count of valid numbers
            valid_nums.clear();
        }

        cur_file_offset += DOUBLE_READ_COUNT_ONCE * sizeof(double);
    }

    uintmax_t remaining_bytes = file_size - cur_file_offset;
    if (remaining_bytes > 0) { //check if any unread numbers in file exist
        std::vector<double> file_nums = fileHelper->read_part_file(cur_file_offset, remaining_bytes / sizeof(double));
        Watchdog::get_instance()->reset_timer();

        for (int i = 0; i < remaining_bytes / sizeof(double); i++) {
            if (fileHelper->is_valid_num(file_nums[i])) {
                valid_nums.push_back(file_nums[i]);
            }
        }

        if (valid_nums.size() > 0) {
            farmer->assign_min_max_dec_point_neg_num(valid_nums);
            Watchdog::get_instance()->reset_timer();
            decisionDist->update_count(static_cast<int>(valid_nums.size())); //update count of valid numbers
            valid_nums.clear();
        }
    }

    fileHelper->close_file_read();

    //get results from each device, summarize
    double min_value = 0;
    double max_value = 0;
    bool dec_point_num = false;
    bool negative_num = false;

    farmer->retr_min_max_dec_point_neg_num_res(&min_value, &max_value, &dec_point_num, &negative_num);
    decisionDist->set_min_value(min_value);
    decisionDist->set_max_value(max_value);
    decisionDist->set_dec_point_num(dec_point_num);
    decisionDist->set_negative_num(negative_num);
}

/*
Prints information retrieved from first pass of algorithm.
- dataset min + max + count of valid numbers in dataset (std::fpclassify(num) returns FP_NORMAL or FP_ZERO)
- decimal point / negative number present
FileHelper* fileHelper = contains functions regarding to files
DecisionDist* decisionDist = functions which help to decide which distribution is closest
*/
void print_first_pass_info(FileHelper* fileHelper, DecisionDist* decisionDist) {
    std::cout << "****FIRST PASS INFO*** START" << std::endl;
    std::cout << "minimum number: " << decisionDist->get_min_value() << std::endl;
    std::cout << "maximum number: " << decisionDist->get_max_value() << std::endl;
    std::cout << "valid number count: " << decisionDist->get_count() << std::endl;
    std::cout << "negative value present (ommit Poisson + exponential): " << std::boolalpha << decisionDist->get_negative_num() << std::endl;
    std::cout << "decimal point value present (ommit Poisson): " << std::boolalpha << decisionDist->get_dec_point_num() << std::endl;
    std::cout << "****FIRST PASS INFO*** END" << std::endl;
}

/*
Prints info gathered during second pass of algorithm.
IntervalManager* intervalManager = functions which are responsible for managing content of intervals into which are numbers sorted
DecisionDist* decisionDist = functions which help to decide which distribution is closest
*/
void print_second_pass_info(IntervalManager* intervalManager, DecisionDist* decisionDist) {
    std::cout << "****SECOND PASS INFO*** START" << std::endl;
    std::cout << "interval count (Sturges rule): " << intervalManager->get_interval_count() << std::endl;
    std::cout << "interval size: " << intervalManager->get_interval_size() << std::endl;
    std::cout << "average: " << decisionDist->get_avg() << std::endl;
    std::cout << "standard deviation: " << decisionDist->get_std_dev() << std::endl;
    std::cout << "first interval boundaries: " << intervalManager->get_first_interval_bound_low() << " - " << intervalManager->get_first_interval_bound_up() << std::endl;
    std::cout << "last interval boundaries: " << intervalManager->get_last_interval_bound_low() << " - " << intervalManager->get_last_interval_bound_up() << std::endl;
    intervalManager->print_interval_cont_debug();
    std::cout << "****SECOND PASS INFO*** END" << std::endl;
}

/*
Function reads whole file and performs actions which are defined for second round of algorithm, namely:
- adds numbers present in file into respective intervals
- calculates dataset average + standard deviation
FileHelper* fileHelper = contains functions regarding to files
IntervalManager* intervalManager = functions which are responsible for managing content of intervals into which are numbers sorted
DecisionDist* decisionDist = functions which help to decide which distribution is closest
Farmer* farmer = farmer (farmer-worker model) which keeps track of availability of workers, assigns work
*/
void perf_second_pass(FileHelper* fileHelper, IntervalManager* intervalManager, DecisionDist* decisionDist, Farmer* farmer) {
    std::cout << "Performing second round of algorithm, please wait..." << std::endl;
    Watchdog::get_instance()->reset_timer();

    uintmax_t file_size = fileHelper->deter_file_size();
    uintmax_t cur_file_offset = 0; //current offset in traversed file
    std::vector<double> valid_nums;

    fileHelper->open_file_read();

    farmer->prep_devs_intervals(intervalManager->get_interval_count());
    while ((cur_file_offset + DOUBLE_READ_COUNT_ONCE * sizeof(double)) < file_size) { //read file, update offset
        std::vector<double> file_nums = fileHelper->read_part_file(cur_file_offset, DOUBLE_READ_COUNT_ONCE); //read doubles from file into array
        Watchdog::get_instance()->reset_timer();

        for (int i = 0; i < DOUBLE_READ_COUNT_ONCE; i++) {
            if (fileHelper->is_valid_num(file_nums[i])) { //only update if valid number
                decisionDist->update_avg_var(file_nums[i]);
                valid_nums.push_back(file_nums[i]);
            }
        }

        if (valid_nums.size() > 0) {
            farmer->assign_add_nums_to_intervals(valid_nums, intervalManager->get_interval_size(), decisionDist->get_min_value(), intervalManager->get_interval_count()); //add numbers into respective intervals
            Watchdog::get_instance()->reset_timer();
            valid_nums.clear();
        }

        cur_file_offset += DOUBLE_READ_COUNT_ONCE * sizeof(double);
    }

    uintmax_t remaining_bytes = file_size - cur_file_offset;
    if (remaining_bytes > 0) { //check if any unread numbers in file exist
        std::vector<double> file_nums = fileHelper->read_part_file(cur_file_offset, remaining_bytes / sizeof(double));
        Watchdog::get_instance()->reset_timer();

        for (int i = 0; i < remaining_bytes / sizeof(double); i++) {
            if (fileHelper->is_valid_num(file_nums[i])) { //only update if valid number
                decisionDist->update_avg_var(file_nums[i]);
                valid_nums.push_back(file_nums[i]);
            }
        }

        if (valid_nums.size() > 0) {
            farmer->assign_add_nums_to_intervals(valid_nums, intervalManager->get_interval_size(), decisionDist->get_min_value(), intervalManager->get_interval_count()); //add numbers into respective intervals
            Watchdog::get_instance()->reset_timer();
            valid_nums.clear();
        }
    }
    fileHelper->close_file_read();

    //get results from each device, summarize
    std::vector<int> output_intervals(intervalManager->get_interval_count(), 0); //output buffer

    farmer->retr_add_nums_to_intervals_res(&output_intervals, intervalManager->get_interval_count());
    intervalManager->set_interval_counter(output_intervals);
}

/*
Performs chi square goodness of fit test and picks closest distribution.
IntervalManager* intervalManager = functions which are responsible for managing content of intervals into which are numbers sorted
DecisionDist* decisionDist = functions which help to decide which distribution is closest
ChiSquareManager* chiSquareMan = functions for solving chi-square test
*/
void perform_chi_square_calc(IntervalManager* intervalManager, DecisionDist* decisionDist, ChiSquareManager* chiSquareMan) {
    Watchdog::get_instance()->reset_timer();
    chi_part_res_struct* dist_func_res = chiSquareMan->calc_distrib_func(intervalManager, decisionDist);
    std::cout << "****CALCULATED DISTRIBUTION FUNCTIONS*** START" << std::endl;
    chiSquareMan->print_chi_part_res(dist_func_res);
    std::cout << "****CALCULATED DISTRIBUTION FUNCTIONS*** END" << std::endl;
    Watchdog::get_instance()->reset_timer();
    chi_part_res_struct* exp_prob_res = chiSquareMan->calc_expected_prob_all_valid_dist(dist_func_res);
    std::cout << "****CALCULATED EXPECTED PROBABILITIES*** START" << std::endl;
    chiSquareMan->print_chi_part_res(exp_prob_res);
    std::cout << "****CALCULATED EXPECTED PROBABILITIES*** END" << std::endl;
    Watchdog::get_instance()->reset_timer();
    chi_part_res_struct* exp_freq_res = chiSquareMan->calc_expected_freq_all_valid_dist(exp_prob_res);
    std::cout << "****CALCULATED EXPECTED FREQUENCIES*** START" << std::endl;
    chiSquareMan->print_chi_part_res(exp_freq_res);
    std::cout << "****CALCULATED EXPECTED FREQUENCIES*** END" << std::endl;
    Watchdog::get_instance()->reset_timer();
    chi_part_res_struct* chi_formula_res = chiSquareMan->calc_chi_formula_all_valid_dist(intervalManager, exp_freq_res);
    std::cout << "****CALCULATED CHI-SQUARE FORMULAS*** START" << std::endl;
    chiSquareMan->print_chi_part_res(chi_formula_res);
    std::cout << "****CALCULATED CHI-SQUARE FORMULAS*** END" << std::endl;
    Watchdog::get_instance()->reset_timer();
    std::cout << "****CALCULATED CHI-SQUARE TEST CRITERIA*** START" << std::endl;
    chi_crit_res_struct* chi_crit_res = chiSquareMan->calc_chi_test_crit_all_valid_dist(chi_formula_res);
    chiSquareMan->print_chi_crit_res(chi_crit_res);
    std::cout << "****CALCULATED CHI-SQUARE TEST CRITERIA*** END" << std::endl;
    Watchdog::get_instance()->reset_timer();
    std::cout << "****CLOSEST DISTRIBUTION INFO*** START" << std::endl;
    chi_win_res_struct* chi_lowest_res = chiSquareMan->pick_lowest_test_crit(chi_crit_res);
    chiSquareMan->print_chi_win_res(chi_lowest_res);
    std::cout << "****CLOSEST DISTRIBUTION INFO*** END" << std::endl;
}