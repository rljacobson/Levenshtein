#pragma once

#include <cstdint> // for uint64_t

#define ALGORITHM_COUNT 9

typedef struct {
    uint64_t cells_computed;         // Number of cells computed in the matrix
    uint64_t early_exit;             // Number of early exit occurrences
    uint64_t exit_length_difference; // Difference in lengths at exit (if applicable)
    double algorithm_time;           // Time spent on the algorithm (in some unit, e.g., microseconds)
    double total_time;               // Total time for execution (in some unit)
    uint64_t buffer_exceeded;        // Number of times the algorithm was called with strings that couldn't fit within the buffer
    uint64_t call_count;             // Number of times the algorithm is called
    const char *algorithm_name;      // Name of the algorithm
} PerformanceMetrics;

// Global array to hold performance metrics for each algorithm
extern PerformanceMetrics performance_metrics[ALGORITHM_COUNT];

// Algorithm names
extern const char *algorithm_names[ALGORITHM_COUNT];

// Function to initialize all performance metrics
void initialize_metrics();

void print_metrics();
