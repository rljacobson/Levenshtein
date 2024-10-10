#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <sstream>
#include <boost/range/iterator_range_core.hpp>


// #ifndef WORD_COUNT
// #define WORD_COUNT 10000ul
// #endif
// #ifdef BENCH_FUNCTION
// #define LEV_FUNCTION BENCH_FUNCTION
// #endif
// #ifndef WORDS_PATH
// #define WORDS_PATH "/usr/share/dict/words"
// #endif

#include "testharness.hpp"  // Include the test harness for LEV_FUNCTION macros
#include "benchtime.hpp"

// Declare all UDF functions with C linkage
extern "C" {
// damlev functions
int damlev_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
long long damlev(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
void damlev_deinit(UDF_INIT *initid);

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
    // Define the available algorithms and their argument counts
    // From CMake:
    // AVAILABLE_ALGORITHMS "damlev;damlevmin;damlevconst;damlevlim;damlevp;noop"
    // ALGORITHM_ARGS "2;3;3;3;3;1"

    std::vector<std::string> algorithms = { "damlev", "damlevmin", "damlevconst", "damlevlim", "damlevp", "noop" };
    std::vector<int> algorithm_args = { 2, 3, 3, 3, 2, 3 };

    // List to hold UDF_Function structs
    std::vector<UDF_Function> udf_functions;

    for(size_t i = 0; i < algorithms.size(); ++i) {
        UDF_Function udf;
        udf.name = algorithms[i];
        udf.arg_count = algorithm_args[i];
        udf.has_max_distance = (udf.arg_count == 3); // Assuming arg_count==3 means max_distance is required

        // Assign function pointers based on the algorithm name
        if(udf.name == "damlev") {
            udf.init = &damlev_init;
            udf.func = &damlev;
            udf.deinit = &damlev_deinit;
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

// Iterator to traverse lines in the mapped file
class LineIterator:
        public boost::iterator_facade<
            LineIterator,
            boost::iterator_range<char const*>,
            boost::iterators::forward_traversal_tag,
            boost::iterator_range<char const*>
        >{
public:
    LineIterator(char const *begin, char const *end) : p_(begin), q_(end) {}

private:
    char const *p_, *q_;

    // Returns the current line as a range
    boost::iterator_range<char const*> dereference() const {
        return {p_, this->next()};
    }

    // Checks equality based on current pointer
    bool equal(LineIterator b) const {
        return p_ == b.p_;
    }

    // Moves to the next line
    void increment() {
        p_ = this->next();
    }

    // Finds the next newline character
    char const* next() const {
        auto p = std::find(p_, q_, '\n');
        return p + (p != q_);  // Move past the newline if found
    }
    friend class boost::iterator_core_access;
};

// Creates a range of LineIterators over the mapped region
inline boost::iterator_range<LineIterator> crange(boost::interprocess::mapped_region const& r) {
    auto p = static_cast<char const*>(r.get_address());
    auto q = p + r.get_size();
    return {LineIterator{p, q}, LineIterator{q, q}};
}

int main(int argc, char *argv[]) {
    // Define file paths and configurations
    std::string primaryFilePath = "tests/taxanames";  // Primary file path
    std::string fallbackFilePath = WORDS_PATH;  // Default fallback file path
    unsigned maximum_size = WORD_COUNT;
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

    // Get the list of UDF functions
    std::vector<UDF_Function> udf_functions = get_udf_functions();

    // Structure to hold benchmarking results
    struct BenchmarkResult {
        std::string name;
        double time_elapsed;
        unsigned word_pairs;
    };
    std::vector<BenchmarkResult> results;

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
            // No max_distance
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

        // Reset counters for each function
    unsigned line_no = 0;
        unsigned print_count = 0;  // Counter for printed examples
        const unsigned max_print = 10;  // Maximum number of examples to print

        // Begin Benchmarking
        std::cout << "Starting benchmark for " << udf.name << "..." << std::endl;
        Timer timer;
        timer.start();  // Start the timer

        // Iterate over word pairs
    for (auto a : crange(text_file_buffer)) {
        for (auto b : crange(text_file_buffer)) {
                // Skip identical words if there are at least two arguments
                if(udf.arg_count >=2 && a == b) continue;

            ++line_no;

                // Assign arguments based on arg_count
                if(udf.arg_count == 3) {
                    // Subject word
                    args.args[0] = const_cast<char*>(a.begin());
                    args.lengths[0] = a.size();
                    // Query word
                    args.args[1] = const_cast<char*>(b.begin());
                    args.lengths[1] = b.size();
                    // Max distance
                    int max_distance = 1;
                    args.args[2] = reinterpret_cast<char*>(&max_distance);
                    args.lengths[2] = sizeof(max_distance);
                }
                else if(udf.arg_count == 2) {
                    // Subject word
                    args.args[0] = const_cast<char*>(a.begin());
                    args.lengths[0] = a.size();
                    // Query word
                    args.args[1] = const_cast<char*>(b.begin());
                    args.lengths[1] = b.size();
                }
                else if(udf.arg_count == 1) {
                    // Single argument
                    args.args[0] = const_cast<char*>(a.begin());
                    args.lengths[0] = a.size();
                }

                // Variables to capture return and null status
                char is_null = 0;
                char error[512] = {0};

                // Call the UDF function directly
                long long distance = udf.func(&initid, &args, &is_null, error);
                if (error[0] != '\0') {
                    std::cerr << "Error during UDF call (" << udf.name << "): " << error << std::endl;
                    // Optionally handle the error, e.g., continue or exit
                }


                if (line_no >= maximum_size) break;
            }
            if (line_no >= maximum_size) break;
        }

        timer.stop();  // Stop the timer

        // De-initialize the UDF
        udf.deinit(&initid);

        // Clean up allocated memory
        delete[] args.arg_type;
        delete[] args.args;
        delete[] args.lengths;

        // Store the benchmarking result
        BenchmarkResult br;
        br.name = udf.name;
        br.time_elapsed = timer.elapsed();
        br.word_pairs = line_no;
        results.push_back(br);

        std::cout << "Benchmark completed for " << udf.name << ": Time elapsed: " << br.time_elapsed << "s, Number of word pairs: " << br.word_pairs << std::endl;
    }

    // After benchmarking all functions, print a summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "Benchmark Summary:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // Define column headers with fixed widths
    std::cout << std::left << std::setw(20) << "Function"
              << std::right << std::setw(15) << "Time (s)"
              << std::right << std::setw(15) << "Word Pairs" << std::endl;

    // Print a separator line
    std::cout << "----------------------------------------" << std::endl;

    // Set formatting for floating-point numbers
    std::cout << std::fixed << std::setprecision(6);

    // Iterate over the results and print each row with proper alignment
    for(const auto& br : results) {
        std::cout << std::left << std::setw(20) << br.name
                  << std::right << std::setw(15) << br.time_elapsed
                  << std::right << std::setw(15) << br.word_pairs << std::endl;
    }

    // Print a closing line
    std::cout << "========================================" << std::endl;

    return 0;
}
