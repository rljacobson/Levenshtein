#include "common.h"
#include <memory>

// Error messages.
// MySQL error messages can be a maximum of MYSQL_ERRMSG_SIZE bytes long. In
// version 8.0, MYSQL_ERRMSG_SIZE == 512. However, the example says to "try to
// keep the error message less than 80 bytes long!" Rules were meant to be
// broken.
constexpr const char
        DAMLEVMIN_ARG_NUM_ERROR[] = "Wrong number of arguments. DAMLEVMIN() requires three arguments:\n"
                                    "\t1. A string\n"
                                    "\t2. A string\n"
                                    "\t3. A maximum distance (0 <= int < ${DAMLEV_MAX_EDIT_DIST}).";
constexpr const auto DAMLEVMIN_ARG_NUM_ERROR_LEN = std::size(DAMLEVMIN_ARG_NUM_ERROR) + 1;
constexpr const char DAMLEVMIN_MEM_ERROR[] = "Failed to allocate memory for DAMLEVMIN"
                                             " function.";
constexpr const auto DAMLEVMIN_MEM_ERROR_LEN = std::size(DAMLEVMIN_MEM_ERROR) + 1;
constexpr const char
        DAMLEVMIN_ARG_TYPE_ERROR[] = "Arguments have wrong type. DAMLEVMIN() requires three arguments:\n"
                                     "\t1. A string\n"
                                     "\t2. A string\n"
                                     "\t3. A maximum distance (0 <= int < ${DAMLEV_MAX_EDIT_DIST}).";
constexpr const auto DAMLEVMIN_ARG_TYPE_ERROR_LEN = std::size(DAMLEVMIN_ARG_TYPE_ERROR) + 1;


// Use a "C" calling convention for MySQL UDF functions.
extern "C" {
    [[maybe_unused]] int damlevmin_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
    [[maybe_unused]] long long damlevmin(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
    [[maybe_unused]] void damlevmin_deinit(UDF_INIT *initid);
}

struct DamLevMinPersistant {
    int max;
    int *buffer; // Takes ownership of this buffer

    DamLevMinPersistant(int max, int *buffer): max(max), buffer(buffer){}

    ~DamLevMinPersistant(){ delete this->buffer; }
};

[[maybe_unused]]
int damlevmin_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 3 arguments:
    if (args->arg_count != 3) {
        strncpy(message, DAMLEVMIN_ARG_NUM_ERROR, DAMLEVMIN_ARG_NUM_ERROR_LEN);
        return 1;
    }
    if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT || args->arg_type[2] != INT_RESULT) {
        strncpy(message, DAMLEVMIN_ARG_TYPE_ERROR, DAMLEVMIN_ARG_TYPE_ERROR_LEN);
        return 1;
    }

    // Initialize persistent data
    int* buffer = new (std::nothrow) int[DAMLEV_MAX_EDIT_DIST];
    DamLevMinPersistant *data = new (std::nothrow) DamLevMinPersistant(DAMLEV_MAX_EDIT_DIST, buffer);
    // If memory allocation failed
    if (!buffer || !data) {
        strncpy(message, DAMLEVMIN_MEM_ERROR, DAMLEVMIN_MEM_ERROR_LEN);
        return 1;
    }

    initid->ptr = reinterpret_cast<char*>(data);

    // There are two error states possible within the function itself:
    //    1. Negative max distance provided
    //    2. Buffer size required is greater than available buffer allocated.
    // The policy for how to handle these cases is selectable in the CMakeLists.txt file.
#if defined(RETURN_NULL_ON_BAD_MAX) || defined(RETURN_NULL_ON_BUFFER_EXCEEDED)
    initid->maybe_null = 1;
#else
    initid->maybe_null = 0;
#endif

    return 0;
}

[[maybe_unused]]
void damlevmin_deinit(UDF_INIT *initid) {
    // As `DamLevMinPersistant` owns its buffer, `~DamLevMinPersistant` handles buffer deallocation.
    delete reinterpret_cast<DamLevMinPersistant*>(initid->ptr);
}

[[maybe_unused]]
long long damlevmin(UDF_INIT *initid, UDF_ARGS *args, [[maybe_unused]] char *is_null, char *error) {
    // Fetch persistent data
    DamLevMinPersistant *data = reinterpret_cast<DamLevMinPersistant *>(initid->ptr);
    // This line and the line updating `data->max` right before the final `return` statement are the only differences
    // between damlevmin and damlevlim.
    long long max = data->max;
    int *buffer = data->buffer;

    // The pre-algorithm code is the same for all algorithm variants. It handles
    //     - basic setup & initialization
    //     - trimming of common prefix/suffix
    //     - computation of "effective" max edit distance
#include "prealgorithm.h"

    // Lambda function for 2D matrix indexing in the 1D buffer
    auto idx = [m](int i, int j) { return i * (m + 1) + j; };

    // Main loop to calculate the Damerau-Levenshtein distance
    for (int i = 1; i <= n; ++i) {
        // Set column_min to infinity
        int column_min = std::numeric_limits<int>::max();
        // Initialize first column
        buffer[idx(i, 0)] = i;

        for (int j = 1; j <= m; ++j) {
            int cost          = (subject[i - 1] == query[j - 1]) ? 0 : 1;
            buffer[idx(i, j)] = std::min({buffer[idx(i - 1, j)] + 1,
                                          buffer[idx(i, j - 1)] + 1,
                                          buffer[idx(i - 1, j - 1)] + cost});

            // Check for transpositions
            if (i > 1 && j > 1 && subject[i - 1] == query[j - 2] && subject[i - 2] == query[j - 1]) {
                buffer[idx(i, j)] = std::min(buffer[idx(i, j)], buffer[idx(i - 2, j - 2)] + cost);
            }

            column_min = std::min(column_min, buffer[idx(i, j)]);
        }

        // Early exit if the minimum edit distance exceeds the effective maximum
        if (column_min > static_cast<int>(effective_max)) {
            return max + 1;
        }
    }

    // This line and the line fetching `data->max` at the top of the function are the only differences
    // between damlevmin and damlevlim.
    data->max = std::min(buffer[idx(n, m)], static_cast<int>(max));

    // Return the final Damerau-Levenshtein distance
    return buffer[idx(n, m)];
}
