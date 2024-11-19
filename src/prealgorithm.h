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

    // Let's make some string views so we can use the STL.
    std::string_view query{args->args[1], args->lengths[1]};
    std::string_view subject{args->args[0], args->lengths[0]};

    // Skip any common prefix.
    auto prefix_mismatch = std::mismatch(subject.begin(), subject.end(), query.begin(), query.end());
    auto start_offset = static_cast<size_t>(std::distance(subject.begin(), prefix_mismatch.first));

    // If one of the strings is a prefix of the other, return the length difference.
    if (subject.length() == start_offset) {
#ifdef CAPTURE_METRICS
        metrics.total_time += call_timer.elapsed();
        metrics.exit_length_difference++;
#endif
        return int(query.length()) - int(start_offset);
    } else if (query.length() == start_offset) {
#ifdef CAPTURE_METRICS
        metrics.total_time += call_timer.elapsed();
        metrics.exit_length_difference++;
#endif
        return int(subject.length()) - int(start_offset);
    }

    // Skip any common suffix.
    auto suffix_mismatch = std::mismatch(subject.rbegin(), std::next(subject.rend(), -int(start_offset)),
                                         query.rbegin(), std::next(query.rend(), -int(start_offset)));
    auto end_offset = std::distance(subject.rbegin(), suffix_mismatch.first);

    // Extract the different part if significant.

    subject = subject.substr(start_offset, subject.length() - start_offset - end_offset);
    query = query.substr(start_offset, query.length() - start_offset - end_offset);


    // Ensure 'subject' is the smaller string for efficiency
    if (query.length() < subject.length()) {
        std::swap(subject, query);
    }

    const int n = static_cast<int>(subject.length()); // Cast size_type to int
    const int m = static_cast<int>(query.length()); // Cast size_type to int

    // It's possible we "trimmed" an entire string.
    if(n==0) {
#ifdef CAPTURE_METRICS
        metrics.exit_length_difference++;
        metrics.total_time += call_timer.elapsed();
#endif
        return m;
    }

#ifndef SUPPRESS_MAX_CHECK
    // Determine the effective maximum edit distance. The distance is at most length of the longest string.
    const int effective_max = std::min(static_cast<int>(max), m);
    // Distance is at least the difference in the lengths of the strings.
    if (m-n > static_cast<int>(max)) {
#ifdef CAPTURE_METRICS
        metrics.exit_length_difference++;
        metrics.total_time += call_timer.elapsed();
#endif
        return max + 1; // Return max+1 by convention.
    }
#endif
    // Re-initialize buffer before calculation
    std::iota(buffer, buffer + m + 1, 0);

    // ToDo: If difference in lengths is greater or equal than effective max, return effective max.


#ifdef CAPTURE_METRICS
    Timer algorithm_timer;
    algorithm_timer.start();
#endif
#ifdef PRINT_DEBUG
    std::cout << "subject: " << subject << '\n';
    std::cout << "query:   " << query << '\n';
#ifndef SUPPRESS_MAX_CHECK
    std::cout << "effective_max: " << effective_max << '\n';
#endif
#endif
