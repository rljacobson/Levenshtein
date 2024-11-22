/*

The implementation from postgres modified to fit my test harness.

*/

#include "common.h"

UDF_SIGNATURES(postgres)

#define Max(x, y)  std::max(x, y)
//((x) > (y) ? (x) : (y))
#define Min(x, y)  std::min(x, y)
//((x) < (y) ? (x) : (y))


[[maybe_unused]]
int postgres_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {
    // We require 3 arguments:
    if (args->arg_count != 3) {
        return 1;
    }
        // The arguments need to be of the right type.
    else if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT || args->arg_type[2] != INT_RESULT) {
        return 1;
    }

    // Attempt to preallocate a buffer.
    initid->ptr = (char *)new(std::nothrow) int[DAMLEV_MAX_EDIT_DIST];
    if (initid->ptr == nullptr) {
        return 1;
    }

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
void postgres_deinit(UDF_INIT *initid) {
    delete[] reinterpret_cast<int*>(initid->ptr);
}


[[maybe_unused]]
long long postgres(UDF_INIT *initid, UDF_ARGS *args, [[maybe_unused]] char *is_null, char *error) {

#ifdef PRINT_DEBUG
    std::cout << "postgres" << "\n";
#endif
#ifdef CAPTURE_METRICS
    PerformanceMetrics &metrics = performance_metrics[8];
#endif

    // postgres allocates on every call. We'll have to test it both ways.
    int *buffer = reinterpret_cast<int *>(initid->ptr);
    int max_d;
    {
        int max = Min(*(reinterpret_cast<long long *>(args->args[2])), DAMLEV_MAX_EDIT_DIST);

#include "validate_max.h"

        max_d = static_cast<int>(max);
    }

    // Taken from prealgorithm //

#ifdef CAPTURE_METRICS
    metrics.call_count++;
    Timer call_timer;
    call_timer.start();
#endif

    // Handle null strings
    if (!args->args[0] || !args->args[1]) {
#ifdef CAPTURE_METRICS
        metrics.total_time += call_timer.elapsed();
        metrics.exit_length_difference++;
#endif
        return static_cast<long long>(Max(args->lengths[0], args->lengths[1]));
    }

    // Let's make some string views so we can use the STL.
    // std::string_view source{args->args[1], args->lengths[1]};
    // std::string_view target{args->args[0], args->lengths[0]};
    char *source = args->args[0];
    char *target = args->args[1];

    int m = static_cast<int>(args->lengths[0]);
    int n = static_cast<int>(args->lengths[1]);

    // Ensure 'target' is the smaller string for efficiency
    if (m < n) {
        std::swap(target, source);
        std::swap(m, n);
    }



#ifdef CAPTURE_METRICS
    Timer algorithm_timer;
    algorithm_timer.start();
#endif
#ifdef PRINT_DEBUG
    std::cout << "target: " << target << '\n';
    std::cout << "source:   " << source << '\n';
#ifndef SUPPRESS_MAX_CHECK
    std::cout << "effective_max: " << max_d << '\n';
#endif
#endif

    // End prealgorithm //


    // Check if buffer size required exceeds available buffer size. This algorithm needs
    // a buffer of size 2*(m+1). Because of trimming, m may be smaller than the length
    // of the longest string.
    if( 2*(m+1) > DAMLEV_BUFFER_SIZE ) {
#ifdef CAPTURE_METRICS
        metrics.buffer_exceeded++;
        metrics.total_time += call_timer.elapsed();
#endif
        return 0;
    }


    // Start (modified) postgres code

    // Parameters in postgres
#define ins_c 1
#define del_c 1
#define sub_c 1
//     int sub_c = 1;

    int		   *prev;
    int		   *curr;
    // int		   *s_char_len = NULL;
    int			i;
    const char *y;

    int			start_column,
                stop_column;

    /*
     * We can transform an empty s into t with n insertions, or a non-empty t
     * into an empty s with m deletions.
    if (!m) {
#ifdef CAPTURE_METRICS
        metrics.exit_length_difference++;
        metrics.total_time += call_timer.elapsed();
#endif
        return n * ins_c;
    }
    if (!n) {
#ifdef CAPTURE_METRICS
        metrics.exit_length_difference++;
        metrics.total_time += call_timer.elapsed();
#endif
        return m * del_c;
    }
     */

    /* Initialize start and stop columns. */
    start_column = 0;
    stop_column = m + 1;

    /*
     * If max_d >= 0, determine whether the bound is impossibly tight.  If so,
     * return max_d + 1 immediately.  Otherwise, determine whether it's tight
     * enough to limit the computation we must perform.  If so, figure out
     * initial stop column.
     */
    // Always true by this point
    // if (max_d >= 0)
    { //preserve scope
        int			min_theo_d; /* Theoretical minimum distance. */
        // int			max_theo_d; /* Theoretical maximum distance. */
        int			net_inserts = n - m;

        min_theo_d = net_inserts < 0 ?
                     -net_inserts * del_c : net_inserts * ins_c;
        if (min_theo_d > max_d) {
#ifdef CAPTURE_METRICS
            metrics.exit_length_difference++;
            metrics.total_time += call_timer.elapsed();
#endif
            return max_d + 1;
        }
        // if (ins_c + del_c < sub_c)
        //     sub_c = ins_c + del_c;
        // max_theo_d = min_theo_d + sub_c * Min(m, n);
        // if (max_d >= max_theo_d)
        //     max_d = -1;
        else if (ins_c + del_c > 0)
        {
            /*
             * Figure out how much of the first i of the notional matrix we
             * need to fill in.  If the string is growing, the theoretical
             * minimum distance already incorporates the cost of deleting the
             * number of characters necessary to make the two strings equal in
             * length.  Each additional deletion forces another insertion, so
             * the best-case total cost increases by ins_c + del_c. If the
             * string is shrinking, the minimum theoretical cost assumes no
             * excess deletions; that is, we're starting no further right than
             * column n - m.  If we do start further right, the best-case
             * total cost increases by ins_c + del_c for each move right.
             */
            int			slack_d = max_d - min_theo_d;
            int			best_column = net_inserts < 0 ? -net_inserts : 0;

            stop_column = best_column + (slack_d / (ins_c + del_c)) + 1;
            if (stop_column > m)
                stop_column = m + 1;
        }
    }

#ifdef PRINT_DEBUG
    // Print the matrix header
    std::cout << "     ";
    for(int k = 0; k < m; k++) std::cout << " " << source[k] << " ";
    std::cout << "\n  ";
    for(int k = 0; k <= m; k++) std::cout << (k < 10? " ": "") << k << " ";
    std::cout << "\n";
#endif

    // postgres matches on UTF-8 characters, while we match on bytes.
    // It stores the length of 'source' character i in s_char_len[i].
    // We omit that here.

    /* One more cell for initialization column and i. */
    ++m;
    ++n;

    /* Previous and current rows of notional array. */
    prev = buffer;//new int[2 * m * sizeof(int)];//palloc(2 * m * sizeof(int));
    curr = prev + m;

    /*
	 * To transform the first i characters of s into the first 0 characters of
	 * t, we must perform i deletions.
	 */
    for (int j = start_column; j < stop_column; j++)
        prev[j] = j * del_c;

    /* Loop through rows of the notional array */
    for (y = target, i = 1; i < n; i++) {

        int *temp;
        const char *x = source;
        const int y_char_len = 1;
        // int			y_char_len = n != tlen + 1 ? pg_mblen(y) : 1;
        int j;


        /*
		 * In the best case, values percolate down the diagonal unchanged, so
		 * we must increment stop_column unless it's already on the right end
		 * of the array.  The inner loop will read prev[stop_column], so we
		 * have to initialize it even though it shouldn't affect the result.
		 */
        if (stop_column < m) {
            prev[stop_column] = max_d + 1;
            ++stop_column;
        }

        /*
         * The main loop fills in curr, but curr[0] needs a special case: to
         * transform the first 0 characters of s into the first i characters
         * of t, we must perform i insertions.  However, if start_column > 0,
         * this special case does not apply.
         */
        if (start_column == 0) {
            curr[0] = i * ins_c;
            j = 1;
        } else
            j = start_column;

#ifdef PRINT_DEBUG
        // Print column header
        std::cout << target[i - 1] << (i < 10 ? "  " : " ") << i << " ";
        for(int k = 1; k <= start_column-2; k++) std::cout << " . ";
        if (start_column > 1) std::cout << (max_d < 9? " " : "") << max_d + 1 << " ";
#endif

        // We include only postgres' "fast path" case of single byte characters.
        // The comment below is preserved from postgres.
        /*
		 * This inner loop is critical to performance, so we include a
		 * fast-path to handle the (fairly common) case where no multibyte
		 * characters are in the mix.  The fast-path is entitled to assume
		 * that if s_char_len is not initialized then BOTH strings contain
		 * only single-byte characters.
		 */
        for (; j < stop_column; j++) {
            int ins;
            int del;
            int sub;

            /* Calculate costs for insertion, deletion, and substitution. */
            ins = prev[j] + ins_c;
            del = curr[j - 1] + del_c;
            sub = prev[j - 1] + ((*x == *y) ? 0 : sub_c);

            /* Take the one with minimum cost. */
            curr[j] = Min(ins, del);
            curr[j] = Min(curr[j], sub);

            /* Point to next character. */
            x++;
#ifdef PRINT_DEBUG
            std::cout << (curr[j] < 10 ? " " : "") << curr[j] << " ";
#endif
#ifdef CAPTURE_METRICS
            metrics.cells_computed++;
#endif
        }
#ifdef PRINT_DEBUG
        if (stop_column < m) std::cout << (max_d < 9? " " : "") << max_d + 1 << " ";
        for(int k = stop_column+2; k <= m; k++) std::cout << " . ";
        std::cout << "   " << (start_column==0?1:start_column) << " <= i <= " << stop_column - 1 << "\n";
#endif

        /* Swap current i with previous i. */
        temp = curr;
        curr = prev;
        prev = temp;

        /* Point to next character. */
        y += y_char_len;

        // Again, by this point its guaranteed `max_d > 0`.

        /*
         * This chunk of code represents a significant performance hit if used
         * in the case where there is no max_d bound.  This is probably not
         * because the max_d >= 0 test itself is expensive, but rather because
         * the possibility of needing to execute this code prevents tight
         * optimization of the loop as a whole.
         */
        // Always true
        // if (max_d >= 0)
        { // preserve scope
            /*
             * The "zero point" is the column of the current i where the
             * remaining portions of the strings are of equal length.  There
             * are (n - 1) characters in the target string, of which i have
             * been transformed.  There are (m - 1) characters in the source
             * string, so we want to find the value for zp where (n - 1) - i =
             * (m - 1) - zp.
             */
            int			zp = i - (n - m);

            /* Check whether the stop column can slide left. */
            while (stop_column > 0)
            {
                int			ii = stop_column - 1;
                int			net_inserts = ii - zp;

                if (prev[ii] + (net_inserts > 0 ? net_inserts * ins_c :
                                -net_inserts * del_c) <= max_d)
                    break;
                stop_column--;
            }

            /* Check whether the start column can slide right. */
            while (start_column < stop_column)
            {
                int			net_inserts = start_column - zp;

                if (prev[start_column] +
                    (net_inserts > 0 ? net_inserts * ins_c :
                     -net_inserts * del_c) <= max_d)
                    break;

                /*
                 * We'll never again update these values, so we must make sure
                 * there's nothing here that could confuse any future
                 * iteration of the outer loop.
                 */
                prev[start_column] = max_d + 1;
                curr[start_column] = max_d + 1;
                if (start_column != 0)
                    source +=  1;
                start_column++;
            }

            /* If they cross, we're going to exceed the bound. */
            if (start_column >= stop_column) {
#ifdef CAPTURE_METRICS
                metrics.early_exit++;
                metrics.algorithm_time += algorithm_timer.elapsed();
                metrics.total_time += call_timer.elapsed();
#endif
#ifdef PRINT_DEBUG
                std::cout << "Bailing early" << '\n';
#endif
                return max_d + 1;
            }
        }

    }
    /*
     * Because the final value was swapped from the previous i to the
     * current i, that's where we'll find it.
     */
#ifdef CAPTURE_METRICS
    metrics.algorithm_time += algorithm_timer.elapsed();
    metrics.total_time += call_timer.elapsed();
#endif

    return Min(prev[m - 1], max_d);
}
