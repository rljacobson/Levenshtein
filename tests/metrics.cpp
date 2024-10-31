#include "metrics.hpp"
#include <iostream>
#include <iomanip> // for std::setw
#include <locale>
#include <sstream>

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
    std::cout << "CAPTURE_METRICS defined." << "\n";
#endif
#ifdef PRINT_DEBUG
    std::cout << "PRINT_DEBUG defined." << "\n";
#endif
    for (int i = 0; i < ALGORITHM_COUNT; i++) {
        performance_metrics[i].cells_computed = 0;
        performance_metrics[i].early_exit = 0;
        performance_metrics[i].exit_length_difference = 0;
        performance_metrics[i].algorithm_time = 0;
        performance_metrics[i].total_time = 0;
        performance_metrics[i].buffer_exceeded = 0;
        performance_metrics[i].call_count = 0;

        performance_metrics[i].algorithm_name = algorithm_names[i]; // Point directly to the string literal
    }
}

// Custom function to format integers with comma separators
std::string formatWithCommas(uint64_t value) {
    // std::locale locale("");
    std::ostringstream oss;
    oss.imbue(std::locale("en_US.UTF-8")); // Use the user's locale
    oss << std::fixed << value; // Convert the integer to a fixed format
    return oss.str();
}


void print_metrics() {
    // Print table header
    std::cout << std::right << std::setw(10) << "Algorithm"
              << std::setw(12) << "Call Count"
              << std::setw(14) << "Mem Exceeded"
              << std::setw(17) << "Total Time (ms)"
              << std::setw(15) << "Alg Time (ms)"
              << std::setw(14) << "Length Exit"
              << std::setw(12) << "Early Exit"
              << std::setw(16) << "Cells Computed"
              << std::endl;

    std::cout << std::string(110, '-') << std::endl; // Adjusted to match the total width of the header

    // Iterate through the metrics and print details for called algorithms
    for (int i = 0; i < ALGORITHM_COUNT; i++) {
        if (performance_metrics[i].call_count > 0) { // Only print if the algorithm was called
            std::cout << std::right << std::setw(10) << performance_metrics[i].algorithm_name
                      << std::setw(12) << formatWithCommas(performance_metrics[i].call_count)
                      << std::setw(14) << performance_metrics[i].buffer_exceeded
                      << std::setw(17) << std::round(performance_metrics[i].total_time*1000.0)
                      << std::setw(15) << std::round(performance_metrics[i].algorithm_time*1000.0)
                      << std::setw(14) << formatWithCommas(performance_metrics[i].exit_length_difference)
                      << std::setw(12) << formatWithCommas(performance_metrics[i].early_exit)
                      << std::setw(16) << formatWithCommas(performance_metrics[i].cells_computed)
                      << std::endl;
        }
    }
}
