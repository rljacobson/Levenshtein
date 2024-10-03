// algorithms_list.hpp
#ifndef ALGORITHMS_LIST_HPP
#define ALGORITHMS_LIST_HPP

#include <string>
#include <vector>
#include <iostream>

// Structure to hold algorithm name and argument count
struct TestAlgorithm {
    std::string name;
    int arg_count;
};

// List of available Levenshtein algorithms
static const std::vector<TestAlgorithm> TEST_ALGORITHMS = {
        { "damlev", 2 },
        { "damlevconst", 3 },
        { "damlevconstmin", 3 },
        { "damlevlim", 3 },
        { "damlevp", 3 },
};

#endif // ALGORITHMS_LIST_HPP
