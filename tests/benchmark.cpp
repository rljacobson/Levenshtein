#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
// The macros below are set in the CMakeLists.txt file. We give sensible
// defaults here in case one does not want to use CMake.



#ifndef WORD_COUNT
#define WORD_COUNT 10000ul
#endif

#ifndef BENCH_FUNCTION
    #define LEV_FUNCTION BENCH_FUNCTION
#else
    #define LEV_FUNCTION damlev
#endif
#ifndef WORDS_PATH
#define WORDS_PATH "/usr/share/dict/words"
#endif
#include "testharness.hpp"


#include "benchtime.hpp"


extern "C" size_t lasm(const char *a, size_t alen, const char * b, size_t blen);

// Magic from SE:
// https://stackoverflow.com/questions/52699244/c-fast-way-to-load-large-txt-file-in-vectorstring
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range_core.hpp>

class LineIterator:
        public boost::iterator_facade<
            LineIterator,
            boost::iterator_range<char const*>,
            boost::iterators::forward_traversal_tag,
            boost::iterator_range<char const*>
        >{
public:
    LineIterator(char const* begin, char const* end)
    : p_(begin), q_(end) {
        // pass
    }

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
inline std::ostream& operator<<(std::ostream& s, boost::iterator_range<char const*> const& line) {
    return s.write(line.begin(), line.size());
}

long long calculateDamLevDistance(const std::string& S1, const std::string& S2) {
    int n = S1.size();
    int m = S2.size();

    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));

    for (int i = 0; i <= n; i++) {
        dp[i][0] = i;
    }
    for (int j = 0; j <= m; j++) {
        dp[0][j] = j;
    }

    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= m; j++) {
            int cost = (S1[i - 1] == S2[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({ dp[i - 1][j] + 1, // Deletion
                                  dp[i][j - 1] + 1, // Insertion
                                  dp[i - 1][j - 1] + cost }); // Substitution

            if (i > 1 && j > 1 && S1[i - 1] == S2[j - 2] && S1[i - 2] == S2[j - 1]) {
                dp[i][j] = std::min(dp[i][j], dp[i - 2][j - 2] + cost); // Transposition
            }
        }
    }

    return dp[n][m];
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    std::string primaryFilePath = "tests/taxanames";  // Primary file path (e.g., "tests/taxanames")
    std::string fallbackFilePath = WORDS_PATH;  // Default fallback file path
    unsigned maximum_size = WORD_COUNT;
    boost::interprocess::file_mapping text_file;
    std::string openedFilePath;

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

    // Benchmark for damlev
    damlev_setup();

    for (auto a : crange(text_file_buffer)) {
        for (auto b : crange(text_file_buffer)) {
            if (a == b) continue;
            ++line_no;
            damlev_call((char *)b.begin(), b.size(), (char *)a.begin(), a.size(), 1);
            if (line_no > maximum_size) break;
        }
        if (line_no > maximum_size) break;
    }
    damlev_teardown();
    double time_damlev = timer.elapsed();
    std::cout << "DAMLEV: Time elapsed: " << time_damlev << "s, Number of words: " << line_no << std::endl;

    // Benchmark for calculateDamLevDistance
    line_no = 0;
    timer.reset();
    Timer timer2;  // Assuming timer starts here

    for (auto a : crange(text_file_buffer)) {
        for (auto b : crange(text_file_buffer)) {
            if (a == b) continue;
            ++line_no;
            std::string strA(a.begin(), a.end());
            std::string strB(b.begin(), b.end());
            calculateDamLevDistance(strA, strB);
            if (line_no > maximum_size) break;
        }
        if (line_no > maximum_size) break;
    }
    double time_full_matrix = timer2.elapsed();
    std::cout << "Full Matrix Approach: Time elapsed: " << time_full_matrix << "s, Number of words: " << line_no << std::endl;

    std::cout << "Time difference: " << (time_damlev - time_full_matrix) << "s\n";

    return 0;
}
