/*
The pre-algorithm code is the same for all algorithm variants. It handles
    - basic setup & initialization
    - trimming of common prefix/suffix
*/

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
        return static_cast<long long>(std::max(args->lengths[0], args->lengths[1]));
    }

    const char* subject = args->args[0];
    const char* query = args->args[1];
    int n = args->lengths[0];
    int m = args->lengths[1];

    // Ensure 'subject' is the smaller string for efficiency
    if (m < n) {
        std::swap(subject, query);
        std::swap(m, n);
    }

    const int m_n = (m-n); // We use this a lot.


#ifndef SUPPRESS_MAX_CHECK
    // Distance is at least the difference in the lengths of the strings.
    if (m_n > static_cast<int>(max)) {
#ifdef CAPTURE_METRICS
        metrics.exit_length_difference++;
        metrics.total_time += call_timer.elapsed();
#endif
        return max + 1; // Return max+1 by convention.
    }
#endif
    // Re-initialize buffer before calculation
    std::iota(buffer, buffer + m + 1, 0);

#ifdef CAPTURE_METRICS
    Timer algorithm_timer;
    algorithm_timer.start();
#endif
#ifdef PRINT_DEBUG
    std::cout << "subject: " << subject << '\n';
    std::cout << "query:   " << query << '\n';
#ifndef SUPPRESS_MAX_CHECK
    std::cout << "max: " << max << '\n';
#endif
#endif
