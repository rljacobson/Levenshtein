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
#include <chrono>  // For precise timing
#include <cctype>  // For character classification
#include <cstring> // For memset
#include <map>     // For storing sample results

#include "testharness.hpp"  // Include the test harness for LEV_FUNCTION macros
#include "benchtime.hpp"
#include "edit_operations.hpp"

int benchmark_on_list(std::vector<std::string_view> &subject_words, std::vector<std::string_view> &mangled_words);
std::vector<std::string> mangle_word_list(std::vector<std::string> &words, int max_edits);

// Levenshtein
UDF_SIGNATURES(edit_dist)
UDF_SIGNATURES(bounded_edit_dist)
UDF_SIGNATURES(min_edit_dist)
UDF_SIGNATURES(postgres)

// Damerau–Levenshtein
// UDF_SIGNATURES(edit_dist_t_2d)
UDF_SIGNATURES(edit_dist_t)
UDF_SIGNATURES(bounded_edit_dist_t)
UDF_SIGNATURES(min_edit_dist_t)

// Benchmarking only
UDF_SIGNATURES(noop)

// The next two are special, as they return a `double` instead of a `long long`.
UDF_SIGNATURES_TYPE(similarity_t, double)
UDF_SIGNATURES_TYPE(min_similarity_t, double)

// Structure to hold function pointers and their metadata
struct UDF_Function {
    int (*init)(UDF_INIT*, UDF_ARGS*, char*);                // Pointer to init function
    long long (*func)(UDF_INIT*, UDF_ARGS*, char*  , char*); // Pointer to main function
    void (*deinit)(UDF_INIT*);  // Pointer to deinit function
    std::string name;           // Name of the function (e.g., "damlev")
    int arg_count;              // Number of arguments
    bool has_max_distance;      // Flag indicating if max_distance is required
};

#define INITIALIZE_ALGORITHM_ARGS(algorithm, arg_count_x) \
{                                                      \
    UDF_Function udf;                                  \
    udf.name      = AS_STRING(algorithm);              \
    udf.arg_count =  arg_count_x;                      \
    udf.has_max_distance = (udf.arg_count == 3);       \
    udf.init      = &MACRO_CONCAT(algorithm, _init);   \
    udf.func      = &algorithm;                        \
    udf.deinit    = &MACRO_CONCAT(algorithm, _deinit); \
    udf_functions.push_back(udf);                      \
}


// Function to initialize the list of UDF_Functions
std::vector<UDF_Function> get_udf_functions() {
    std::vector<UDF_Function> udf_functions;

    // If you don't want a benchmark to run, comment it out here.

    INITIALIZE_ALGORITHM_ARGS(bounded_edit_dist, 3)
    INITIALIZE_ALGORITHM_ARGS(bounded_edit_dist_t, 3)
    INITIALIZE_ALGORITHM_ARGS(edit_dist, 2)
    INITIALIZE_ALGORITHM_ARGS(edit_dist_t, 2)
    // INITIALIZE_ALGORITHM_ARGS(edit_dist_t_2d, 2)
    INITIALIZE_ALGORITHM_ARGS(min_edit_dist, 3)
    INITIALIZE_ALGORITHM_ARGS(min_edit_dist_t, 3)
    // INITIALIZE_ALGORITHM_ARGS(min_similarity_t, 3)
    INITIALIZE_ALGORITHM_ARGS(noop, 3)
    INITIALIZE_ALGORITHM_ARGS(postgres, 3)
    // INITIALIZE_ALGORITHM_ARGS(similarity_t, 3)

    return udf_functions;
}

#ifndef CAPTURE_METRICS
// Custom function to format integers with comma separators
std::string formatWithCommas(uint64_t value);
#endif

// region Benchmark with words from a file

std::string_view trim(std::string_view str) {
    auto start = std::find_if_not(str.begin(), str.end(), ::isspace);
    auto end = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
    return (start < end) ? str.substr(start - str.begin(), end - start) : std::string_view{};
}

// Helper function to remove internal control characters except spaces
std::string remove_internal_control_chars(const std::string_view s) {
    std::string result(s);
    result.erase(std::remove_if(result.begin(), result.end(),
                                [](unsigned char c) { return std::iscntrl(c) && c != ' '; }), result.end());
    return result;
}

void get_subsample(std::vector<std::string> &list, u_int64_t sample_size, std::vector<std::string> &sample_list){
    // std::random_device rd;
    // std::mt19937 gen(rd());

    if( sample_size > list.size()) {
        std::cout << "Wrong sample size. sample_size: " << sample_size << "   list.size: " << list.size() << std::endl;
    }
    sample_size = std::min(sample_size, static_cast<u_int64_t>(list.size()));  // Prevent out-of-bounds access

    // sample_list.reserve(sample_size);
    for(u_int64_t i=0; i < sample_size; i++){
        sample_list.push_back(list[i]);
    }
    // std::sample(
    //         list.begin(),
    //         list.end(),
    //         std::back_inserter(sample_list),
    //         sample_size,
    //         gen
    // );
}


/// Reads `max_total_words` from the wordlist file, randomly selects `max_subject_words` of that total, shuffles, and
/// returns a vector of strings if successful. Returns empty vector on error.
std::vector<std::string>
generate_word_list_from_file(int max_total_words, std::string &primaryFilePath) {
    // Define file paths and configurations
    // std::string primaryFilePath = "taxonomic_names.csv";  // Primary file path
    std::string fallbackFilePath = WORDS_PATH;//"words.txt";        // Default fallback file path

    boost::interprocess::file_mapping text_file;
    std::string_view openedFilePath;

    // Attempt to open the primary file
    try {
        text_file = boost::interprocess::file_mapping(primaryFilePath.c_str(), boost::interprocess::read_only);
        openedFilePath = primaryFilePath;
    } catch (const boost::interprocess::interprocess_exception &ePrimary) {
        try {
            text_file = boost::interprocess::file_mapping(fallbackFilePath.c_str(), boost::interprocess::read_only);
            openedFilePath = fallbackFilePath;
        } catch (const boost::interprocess::interprocess_exception &eFallback) {
            std::cerr << "Could not open fallback file " << fallbackFilePath << '.' << std::endl;
            // Empty list indicates error state
            return {};
        }
    }

    // Create a mapped region for the file
    boost::interprocess::mapped_region text_file_buffer(text_file, boost::interprocess::read_only);

    std::cout << "Opened file: " << openedFilePath << "." << std::endl;

    // Preprocess: Load words into a vector
    std::vector<std::string> words;
    words.reserve(max_total_words); // Reserve space to avoid frequent reallocations

    // Load words
    std::istringstream iss(
            std::string(static_cast<const char *>(text_file_buffer.get_address()), text_file_buffer.get_size()));
    std::string line;
    for (int word_count=0; word_count < max_total_words && std::getline(iss, line); word_count++) {
        // std::string_view trimmed_word = trim(line); // Remove leading and trailing whitespace
        // std::string word = remove_internal_control_chars(trimmed_word); // Remove internal control characters

        if (!line.empty()) {
            words.push_back(line);
        } else {
            word_count--;
        }
    }

    std::cout << "Loaded " << words.size() << " words from the file." << std::endl;

    // std::vector<std::string> subject_words;
    //
    // // Randomly sample max_subject_words from words to use as subject words
    // if (words.size() > max_subject_words) {
    //     subject_words = get_subsample(words, max_subject_words);
    // } else {
    //     subject_words = words;
    // }
    return words;
}


// endregion Benchmark with words from a file

// region Benchmark with randomly generated strings
std::vector<std::string> generate_list_of_random_words(int word_count, int word_length, int lower_bound=-1) {
    std::vector<std::string> words;
    words.reserve(word_count);

    for(int i = 0; i < word_count; i++) {
        words.push_back(generateRandomString(word_length, lower_bound));
    }
    return words;
}

/// Creates a new list with copies of strings from `words` that have been given `max_edits` and shuffled.
std::vector<std::string> mangle_word_list(std::vector<std::string> &subject_words, int max_edits, int max_edits_lower_bound){
    std::vector<std::string> mangled_words;
    mangled_words.reserve(subject_words.size());

    for(const auto &subject : subject_words){
        mangled_words.push_back(apply_random_edits(subject, max_edits, max_edits_lower_bound));
    }

    // Shuffle the mangled words list once
    std::shuffle(mangled_words.begin(), mangled_words.end(), gen);

    return mangled_words;
}

struct BenchmarkResult {
    std::string name;
    double time_elapsed;
    unsigned long long function_calls;
};



int benchmark_on_list(std::vector<std::string> &subject_words, std::vector<std::string> &mangled_words, long long max_distance){
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

        std::vector<SampleResult> sample_results; // To store up to 5 samples
        size_t sample_count = 0;

        unsigned long long call_count = 0;

        // Begin Benchmarking
        auto start_time = std::chrono::high_resolution_clock::now();

        // Outer loop over subject words
        for (const auto& subject : subject_words) {
            // long long min_result = -1;
            // int count =0;
            // int count_found = 0;
            std::string min_query;


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

            // Inner loop over mangled words
            for (const auto& query : mangled_words) {
                // count++;
                // Prepare arguments
                // if(subject.size() == 0 || query.size() == 0) {
                //     std::cout << "FOUND EMPTY STRING" << '\n';
                // }
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
                // if (min_result == -1 || result < min_result) {
                //     min_result = result;
                //     min_query = query;
                //     count_found = count;
                // }

                ++call_count;
            }

            // Collect up to 5 samples
            // if (sample_count < 5) {
            //     sample_results.push_back({subject, min_query, min_result, count_found});
            //     ++sample_count;
            // }

            // De-initialize the UDF
            udf.deinit(&initid);

        }

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end_time - start_time;

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

        // std::cout << "Benchmark completed for " << udf.name << ": Time elapsed: " << br.time_elapsed << "s, Number of function calls: " << br.function_calls << std::endl;
    }

    // After benchmarking all functions, print samples and summary
    for(const auto& br : results) {
        if(!br.samples.empty()){
            std::cout << "\nSamples for function: " << br.name << std::endl;
            std::cout << "----------------------------------------" << std::endl;
            for (const auto &sample: br.samples) {
                std::cout << "Subject: " << sample.subject << ", Query: " << sample.query << ", Result: "
                          << sample.result << " Position: " << sample.count << std::endl;
            }
        }
    }

    //----------------------------------------------
    // Function             Time (ms) Function Calls

    if (true){
        std::cout << "\n=============================================" << std::endl;
        std::cout << "Time including call overhead" << std::endl;
        std::cout << "---------------------------------------------" << std::endl;

        // Define column headers with fixed widths
        std::cout << std::left << std::setw(20) << "Function"
                  << std::right << std::setw(10) << "Time (ms)"
                  << std::right << std::setw(15) << "Function Calls" << std::endl;

        // Print a separator line
        std::cout << "---------------------------------------------" << std::endl;

        // Set formatting for floating-point numbers
        std::cout << std::fixed << std::setprecision(6);

        // Iterate over the results and print each row with proper alignment
        for (const auto &br: results) {
            std::cout << std::left << std::setw(20) << br.name
                      << std::right << std::setw(10) << formatWithCommas(std::round(1000.0*(br.time_elapsed)))
                      << std::right << std::setw(15) << formatWithCommas(br.function_calls) << '\n';
        }

        // Print a closing line
        std::cout << "=============================================" << std::endl;
    }

#ifdef CAPTURE_METRICS
    print_metrics();
#endif
    return 0;
}


int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
#ifdef CAPTURE_METRICS
    initialize_metrics();
#endif

    int max_subject_words     = 2000; // Number of subject words to test
    int max_total_words       = 200000; // Total number of words to load from file and sample from.
    long long max_distance    = 5;    // The max given to the algorithms (NOT max number of edits)
    int max_edits             = 5;    // The maximum number of edits used when mangling the strings (NOT algorithm limit)
    int max_edits_lower_bound = 1;   // -1 is disabled = do exactly max_edits

    // For randomly generated strings
    int word_length             = 40;
    int word_length_lower_bound = -1; // -1 is disabled = all strings have length `word_length`

#ifndef USE_GENERATED_WORDS
    std::vector<std::string> subject_words =
            generate_word_list_from_file(max_subject_words, max_total_words);
#else
    std::vector<std::string> subject_words =
            generate_list_of_random_words(max_subject_words, word_length, word_length_lower_bound);
#endif
    std::vector<std::string> mangled_words =
            mangle_word_list(subject_words, max_edits, max_edits_lower_bound);

    std::cout << std::right << std::setw(20) << "max_subject_words: " << max_subject_words << '\n';
    std::cout << std::right << std::setw(20) << "max_total_words: "   << max_total_words   << '\n';
    std::cout << std::right << std::setw(20) << "max_distance: "      << max_distance      << '\n';
    std::cout << std::right << std::setw(20) << "max_edits: "         << max_edits         << '\n';
    std::cout << std::right << std::setw(20) << "word_length: "       << word_length       << std::endl;

    return benchmark_on_list(subject_words, mangled_words, max_distance);
}
