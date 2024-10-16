#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <sstream>
#include <boost/range/iterator_range_core.hpp>
#include <iomanip> // For output formatting
#include <random>
#include <chrono> // For precise timing
#include <cctype> // For character classification
#include <cassert> // For assertions
#include <cstring> // For memset
#include <map>     // For storing sample results

#include "testharness.hpp"  // Include the test harness for LEV_FUNCTION macros
#include "benchtime.hpp"

// Declare all UDF functions with C linkage
extern "C" {
// damlev functions
int damlev_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long damlev(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void damlev_deinit(UDF_INIT *initid);

// damlev1D functions
int damlev1D_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long damlev1D(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void damlev1D_deinit(UDF_INIT *initid);

// damlev2D functions
int damlev2D_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long damlev2D(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void damlev2D_deinit(UDF_INIT *initid);

// damlevmin functions
int damlevmin_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long damlevmin(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void damlevmin_deinit(UDF_INIT *initid);

// damlevlim functions
int damlevlim_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long damlevlim(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void damlevlim_deinit(UDF_INIT *initid);

// damlevp functions
int damlevp_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long damlevp(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void damlevp_deinit(UDF_INIT *initid);

// noop functions
int noop_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long noop(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void noop_deinit(UDF_INIT *initid);
}

// Structure to hold function pointers and their metadata
struct UDF_Function {
    std::string name;  // Name of the function (e.g., "damlev")
    int (*init)(UDF_INIT*, UDF_ARGS*, char*);      // Pointer to init function
    long long (*func)(UDF_INIT*, UDF_ARGS*, char*, char*); // Pointer to main function
    void (*deinit)(UDF_INIT*);                     // Pointer to deinit function
    int arg_count;         // Number of arguments
    bool has_max_distance; // Flag indicating if max_distance is required
};

// Function to initialize the list of UDF_Functions
std::vector<UDF_Function> get_udf_functions() {
    std::vector<std::string> algorithms = { "damlev", "damlev1D", "damlev2D", "damlevlim", "damlevmin", "damlevp", "noop" };
    std::vector<int> algorithm_args = { 2, 3, 2, 3, 3, 2, 3 };

    // List to hold UDF_Function structs
    std::vector<UDF_Function> udf_functions;

    for(size_t i = 0; i < algorithms.size(); ++i) {
        UDF_Function udf;
        udf.name = algorithms[i];
        udf.arg_count = algorithm_args[i];
        udf.has_max_distance = (udf.arg_count == 3); // Assuming arg_count==3 means max_distance is required

        // Assign function pointers based on the algorithm name
        if (udf.name == "damlev") {
            udf.init = &damlev_init;
            udf.func = &damlev;
            udf.deinit = &damlev_deinit;
        }
        else if(udf.name == "damlev1D") {
            udf.init = &damlev1D_init;
            udf.func = &damlev1D;
            udf.deinit = &damlev1D_deinit;
        }
        else if(udf.name == "damlev2D") {
            udf.init = &damlev2D_init;
            udf.func = &damlev2D;
            udf.deinit = &damlev2D_deinit;
        }
        else if(udf.name == "damlevmin") {
            udf.init = &damlevmin_init;
            udf.func = &damlevmin;
            udf.deinit = &damlevmin_deinit;
        }
        else if(udf.name == "damlevlim") {
            udf.init = &damlevlim_init;
            udf.func = &damlevlim;
            udf.deinit = &damlevlim_deinit;
        }
        else if(udf.name == "damlevp") {
            udf.init = &damlevp_init;
            udf.func = &damlevp;
            udf.deinit = &damlevp_deinit;
        }
        else if(udf.name == "noop") {
            udf.init = &noop_init;
            udf.func = &noop;
            udf.deinit = &noop_deinit;
        }
        else {
            std::cerr << "Unknown algorithm: " << udf.name << std::endl;
            continue; // Skip unknown algorithms
        }

        udf_functions.push_back(udf);
    }

    return udf_functions;
}

// Helper function to trim whitespace from both ends of a string
std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    auto end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return ""; // All whitespace
    return s.substr(start, end - start + 1);
}

// Helper function to remove internal control characters except spaces
std::string remove_internal_control_chars(const std::string& s) {
    std::string result = s;
    result.erase(std::remove_if(result.begin(), result.end(),
                                [](unsigned char c) { return std::iscntrl(c) && c != ' '; }), result.end());
    return result;
}

// Helper function to randomly insert a string into another string
std::string randomlyInsertString(const std::string& base, const std::string& to_insert, std::mt19937& gen) {
    if (base.empty()) return to_insert; // Handle empty base string
    std::uniform_int_distribution<> dis(0, base.size()); // Allow insertion at position 0 and base.size()
    int insert_pos = dis(gen);

    std::string modified_base = base;
    modified_base.insert(insert_pos, to_insert);

    // Assertion to ensure no newline or carriage return characters are present
    assert(modified_base.find('\n') == std::string::npos && "Modified base contains newline character!");
    assert(modified_base.find('\r') == std::string::npos && "Modified base contains carriage return!");

    return modified_base;
}

int main(int argc, char *argv[]) {
    // For reproducibility
    std::mt19937 gen(42);

    // Define file paths and configurations
    std::string primaryFilePath = "tests/taxanames";  // Primary file path
    std::string fallbackFilePath = "words.txt";        // Default fallback file path
    unsigned max_subject_words = 100;                 // Number of subject words to test
    unsigned max_total_words = 100000;                 // Total number of words to load
    boost::interprocess::file_mapping text_file;
    std::string openedFilePath;

    // Attempt to open the primary file
    try {
        text_file = boost::interprocess::file_mapping(primaryFilePath.c_str(), boost::interprocess::read_only);
        openedFilePath = primaryFilePath;
    } catch (const boost::interprocess::interprocess_exception& ePrimary) {
        try {
            text_file = boost::interprocess::file_mapping(fallbackFilePath.c_str(), boost::interprocess::read_only);
            openedFilePath = fallbackFilePath;
        } catch (const boost::interprocess::interprocess_exception& eFallback) {
            std::cerr << "Could not open fallback file " << fallbackFilePath << '.' << std::endl;
            return EXIT_FAILURE;
        }
    }

    // Create a mapped region for the file
    boost::interprocess::mapped_region text_file_buffer(text_file, boost::interprocess::read_only);

    std::cout << "Opened file: " << openedFilePath << "." << std::endl;

    // Preprocess: Load words into a vector
    std::vector<std::string> words;
    words.reserve(max_total_words); // Reserve space to avoid frequent reallocations

    // Load words
    size_t word_count = 0;
    std::istringstream iss(std::string(static_cast<const char*>(text_file_buffer.get_address()), text_file_buffer.get_size()));
    std::string line;
    while (std::getline(iss, line)) {
        std::string word = trim(line); // Remove leading and trailing whitespace
        word = remove_internal_control_chars(word); // Remove internal control characters

        if (!word.empty()) {
            words.push_back(word);
            if (++word_count >= max_total_words) break;
        }
    }

    std::cout << "Loaded " << words.size() << " words from the file." << std::endl;

    // Create mangled words from all loaded words
    std::vector<std::string> mangled_words;
    mangled_words.reserve(words.size());
    for (const auto& word : words) {
        std::string mangled_word = randomlyInsertString(word, "12", gen);
        mangled_words.push_back(mangled_word);
    }

    // Shuffle the mangled words list once
    std::shuffle(mangled_words.begin(), mangled_words.end(), gen);

    // Select a subset of words to use as subject words
    std::vector<std::string> subject_words;
    if (words.size() <= max_subject_words) {
        subject_words = words;
    } else {
        // Randomly sample max_subject_words from words
        subject_words.reserve(max_subject_words);
        std::sample(words.begin(), words.end(), std::back_inserter(subject_words),
                    max_subject_words, gen);
    }

    std::cout << "Selected " << subject_words.size() << " subject words for testing." << std::endl;

    // Get the list of UDF functions
    std::vector<UDF_Function> udf_functions = get_udf_functions();

    // Structure to hold benchmarking results
    struct SampleResult {
        std::string subject;
        std::string query;
        long long result;
        int count;
    };

    struct BenchmarkResult {
        std::string name;
        double time_elapsed;
        unsigned long long function_calls;
        std::vector<SampleResult> samples;
    };
    std::vector<BenchmarkResult> results;

    // Total number of function calls
    unsigned long long total_calls = subject_words.size() * mangled_words.size();

    // Iterate over each UDF function
    for(const auto& udf : udf_functions) {
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Benchmarking function: " << udf.name << std::endl;

        // Initialize UDF_INIT
        UDF_INIT initid;
        memset(&initid, 0, sizeof(UDF_INIT));

        // Initialize UDF_ARGS
        UDF_ARGS args;
        memset(&args, 0, sizeof(UDF_ARGS));

        // Define argument types and other parameters
        args.arg_count = udf.arg_count;  // Number of arguments for this UDF
        args.arg_type = new Item_result[args.arg_count];
        args.args = new char*[args.arg_count];
        args.lengths = new unsigned long[args.arg_count];

        // Assign argument types based on arg_count
        if(udf.arg_count == 3) {
            // Assuming the third argument is max_distance
            args.arg_type[0] = STRING_RESULT;  // Subject word
            args.arg_type[1] = STRING_RESULT;  // Query word
            args.arg_type[2] = INT_RESULT;     // Max distance
        }
        else if(udf.arg_count == 2) {
            args.arg_type[0] = STRING_RESULT;  // Subject word
            args.arg_type[1] = STRING_RESULT;  // Query word
        }
        else if(udf.arg_count == 1) {
            args.arg_type[0] = STRING_RESULT;  // Single argument
        }
        else {
            std::cerr << "Unsupported arg_count: " << udf.arg_count << " for function: " << udf.name << std::endl;
            // Clean up
            delete[] args.arg_type;
            delete[] args.args;
            delete[] args.lengths;
            continue; // Skip to the next function
        }

        // Initialize the UDF
        char message[512];
        int init_result = udf.init(&initid, &args, message);
        if (init_result != 0) {
            std::cerr << "Initialization failed for " << udf.name << ": " << message << std::endl;
            // Clean up allocated memory
            delete[] args.arg_type;
            delete[] args.args;
            delete[] args.lengths;
            continue; // Skip to the next function
        }

        // Begin Benchmarking
        auto start_time = std::chrono::high_resolution_clock::now();

        std::vector<SampleResult> sample_results; // To store up to 5 samples
        size_t sample_count = 0;

        unsigned long long call_count = 0;

        // Outer loop over subject words
        for (const auto& subject : subject_words) {
            long long min_result = -1;
            int count =0;
            int count_found = 0;
            std::string min_query;
            [[maybe_unused]] long long max_distance =100;
            // Inner loop over mangled words
            for (const auto& query : mangled_words) {
                count++;
                // Prepare arguments
                if (udf.arg_count == 3) {
                    args.args[0] = const_cast<char*>(subject.c_str());
                    args.lengths[0] = subject.size();

                    args.args[1] = const_cast<char*>(query.c_str());
                    args.lengths[1] = query.size();

                    args.args[2] = reinterpret_cast<char*>(&max_distance);
                    args.lengths[2] = sizeof(max_distance);
                }
                else if (udf.arg_count == 2) {
                    args.args[0] = const_cast<char*>(subject.c_str());
                    args.lengths[0] = subject.size();

                    args.args[1] = const_cast<char*>(query.c_str());
                    args.lengths[1] = query.size();
                }
                else if (udf.arg_count == 1) {
                    args.args[0] = const_cast<char*>(subject.c_str());
                    args.lengths[0] = subject.size();
                }

                // Variables to capture return and null status
                char is_null = 0;
                char error[512] = {0};

                // Call the UDF function directly
                long long result = udf.func(&initid, &args, &is_null, error);
                if (error[0] != '\0') {
                    std::cerr << "Error during UDF call (" << udf.name << "): " << error << std::endl;
                    // Optionally handle the error, e.g., continue or exit
                }

                // Update min_result and min_query
                if (min_result == -1 || result < min_result) {
                    min_result = result;
                    min_query = query;
                    count_found = count;
                }

                ++call_count;
            }

            // Collect up to 5 samples
            if (sample_count < 5) {
                sample_results.push_back({subject, min_query, min_result, count_found});
                ++sample_count;
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end_time - start_time;

        // De-initialize the UDF
        udf.deinit(&initid);

        // Clean up allocated memory
        delete[] args.arg_type;
        delete[] args.args;
        delete[] args.lengths;

        // Store the benchmarking result
        BenchmarkResult br;
        br.name = udf.name;
        br.time_elapsed = elapsed.count();
        br.function_calls = call_count;
        br.samples = sample_results;
        results.push_back(br);

        std::cout << "Benchmark completed for " << udf.name << ": Time elapsed: " << br.time_elapsed << "s, Number of function calls: " << br.function_calls << std::endl;
    }

    // After benchmarking all functions, print samples and summary
    for(const auto& br : results) {
        std::cout << "\nSamples for function: " << br.name << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        for(const auto& sample : br.samples) {
            std::cout << "Subject: " << sample.subject << ", Query: " << sample.query << ", Result: " << sample.result <<" Position: " << sample.count <<std::endl;
        }
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "Benchmark Summary:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // Define column headers with fixed widths
    std::cout << std::left << std::setw(20) << "Function"
              << std::right << std::setw(15) << "Time (s)"
              << std::right << std::setw(20) << "Function Calls" << std::endl;

    // Print a separator line
    std::cout << "----------------------------------------" << std::endl;

    // Set formatting for floating-point numbers
    std::cout << std::fixed << std::setprecision(6);

    // Iterate over the results and print each row with proper alignment
    for(const auto& br : results) {
        std::cout << std::left << std::setw(20) << br.name
                  << std::right << std::setw(15) << br.time_elapsed
                  << std::right << std::setw(20) << br.function_calls << std::endl;
    }

    // Print a closing line
    std::cout << "========================================" << std::endl;

    return 0;
}
