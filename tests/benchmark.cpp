#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <sstream>
#include <boost/range/iterator_range_core.hpp>

#ifndef WORD_COUNT
#define WORD_COUNT 10000ul
#endif
#ifdef BENCH_FUNCTION
#define LEV_FUNCTION BENCH_FUNCTION
#endif
#ifndef WORDS_PATH
#define WORDS_PATH "/usr/share/dict/words"
#endif

#include "testharness.hpp"  // Include the test harness for LEV_FUNCTION macros
#include "benchtime.hpp"


// Function to read lines from a mapped file (used for benchmarking)
inline std::vector<std::string> readWordsFromMappedFile(const boost::interprocess::mapped_region &region, unsigned maximumWords) {
    const char *begin = static_cast<const char *>(region.get_address());
    const char *end = begin + region.get_size();
    std::string fileContent(begin, end);

    std::istringstream iss(fileContent);
    std::vector<std::string> words;
    std::string word;
    while (iss >> word && words.size() < maximumWords) {
        words.push_back(word);
    }

    return words;
}

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
    boost::iterator_range<char const*> dereference() const {
        return {p_, this->next()};
    }
    bool equal(LineIterator b) const {
        return p_ == b.p_;
    }
    void increment() {
        p_ = this->next();
    }
    char const* next() const {
        auto p = std::find(p_, q_, '\n');
        return p + (p != q_);
    }
    friend class boost::iterator_core_access;
};
inline boost::iterator_range<LineIterator> crange(boost::interprocess::mapped_region const& r) {
    auto p = static_cast<char const*>(r.get_address());
    auto q = p + r.get_size();
    return {LineIterator{p, q}, LineIterator{q, q}};
}


int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
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
    boost::interprocess::mapped_region text_file_buffer(text_file, boost::interprocess::read_only);

    std::cout << "Opened file: " << openedFilePath << "." << std::endl;

    Timer timer;
    unsigned line_no = 0;

    // Benchmark for the specified LEV_FUNCTION
    LEV_SETUP();  // Use the macro for setup

    for (auto a : crange(text_file_buffer)) {
        for (auto b : crange(text_file_buffer)) {
            if (a == b) continue;
            ++line_no;
            LEV_CALL((char *)b.begin(), b.size(), (char *)a.begin(), a.size(), 1);  // Use the macro for the function call
            if (line_no > maximum_size) break;
        }
        if (line_no > maximum_size) break;
    }

    LEV_TEARDOWN();  // Use the macro for teardown

    double time_elapsed = timer.elapsed();
    std::cout << "Benchmark completed: Time elapsed: " << time_elapsed << "s, Number of words: " << line_no << std::endl;

    return 0;
}
