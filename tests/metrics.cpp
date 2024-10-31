#include "metrics.hpp"
#include <iostream>
#include <iomanip> // for std::setw

// Global array to hold performance metrics for each algorithm
PerformanceMetrics performance_metrics[ALGORITHM_COUNT];

// Algorithm names
const char *algorithm_names[ALGORITHM_COUNT] = {
        "levlim",       // 0
        "levlimopt",    // 1
        "damlev",       // 2
        "damlevmin",    // 3
        "damlevlim",    // 4
        "damlevlimopt", // 5
        "damlevp",      // 6
        "damlev2D",     // 7
        "noop"          // 8
};

// Function to initialize all performance metrics
void initialize_metrics() {
#ifdef CAPTURE_METRICS
    std::cout << "CAPTURE_METRICS defined" << "\n";
#endif
#ifdef PRINT_DEBUG
    std::cout << "PRINT_DEBUG defined" << "\n";
#endif
    for (int i = 0; i < ALGORITHM_COUNT; i++) {
        performance_metrics[i].cells_computed = 0;
        performance_metrics[i].early_exit = 0;
        performance_metrics[i].exit_length_difference = 0;
        performance_metrics[i].algorithm_time = 0;
        performance_metrics[i].total_time = 0;
        performance_metrics[i].call_count = 0;

        performance_metrics[i].algorithm_name = algorithm_names[i]; // Point directly to the string literal
    }
}
/*

void print_metrics() {
    // Print table header
    std::cout << "Algorithm Name\tCells Computed\tEarly Exit\tExit Length Difference\tAlgorithm Time\tTotal Time\tCall Count\n";
    std::cout << "----------------------------------------------------------------------------------------------------\n";

    // Iterate through the metrics and print details for called algorithms
    for (int i = 0; i < ALGORITHM_COUNT; i++) {
        if (performance_metrics[i].call_count > 0) { // Only print if the algorithm was called
            std::cout << performance_metrics[i].algorithm_name << "\t"
                      << performance_metrics[i].cells_computed << "\t\t"
                      << performance_metrics[i].early_exit << "\t\t"
                      << performance_metrics[i].exit_length_difference << "\t\t\t"
                      << performance_metrics[i].algorithm_time << "\t"
                      << performance_metrics[i].total_time << "\t"
                      << performance_metrics[i].call_count << "\n";
        }
    }
}
*/
void print_metrics() {
    // Print table header
    std::cout << std::left << std::setw(15) << "Algorithm Name"
              << std::setw(15) << "Call Count"
              << std::setw(15) << "Total Time"
              << std::setw(15) << "Algorithm Time"
              << std::setw(20) << "Exit Via Length"
              << std::setw(15) << "Early Exit"
              << std::setw(20) << "Cells Computed"
              << std::endl;

    std::cout << std::string(120, '-') << std::endl; // Adjusted to match the total width of the header

    // Iterate through the metrics and print details for called algorithms
    for (int i = 0; i < ALGORITHM_COUNT; i++) {
        if (performance_metrics[i].call_count > 0) { // Only print if the algorithm was called
            std::cout << std::left << std::setw(15) << performance_metrics[i].algorithm_name
                      << std::setw(15) << performance_metrics[i].call_count
                      << std::setw(15) << performance_metrics[i].total_time
                      << std::setw(15) << performance_metrics[i].algorithm_time
                      << std::setw(20) << performance_metrics[i].exit_length_difference
                      << std::setw(15) << performance_metrics[i].early_exit
                      << std::setw(20) << performance_metrics[i].cells_computed
                      << std::endl;
        }
    }
}
