/*
The pre-algorithm code is the same for all algorithm variants. It handles
    - basic setup & initialization
    - trimming of common prefix/suffix
*/

    if (!args->args[2]) {
        // Early exit if max distance is zero
        return 0;
    }

    // Handle null or empty strings
    if (!args->args[0] || args->lengths[0] == 0 || !args->args[1] || args->lengths[1] == 0) {
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
        return int(query.length()) - int(start_offset);
    } else if (query.length() == start_offset) {
        return int(subject.length()) - int(start_offset);
    }

    // Skip any common suffix.
    auto suffix_mismatch = std::mismatch(subject.rbegin(), std::next(subject.rend(), -int(start_offset)),
                                         query.rbegin(), std::next(query.rend(), -int(start_offset)));
    auto end_offset = std::distance(subject.rbegin(), suffix_mismatch.first);

    // Extract the different part if significant.
    if (start_offset + end_offset < subject.length()) {
        subject = subject.substr(start_offset, subject.length() - start_offset - end_offset);
        query = query.substr(start_offset, query.length() - start_offset - end_offset);
    }

    // Ensure 'subject' is the smaller string for efficiency
    if (query.length() < subject.length()) {
        std::swap(subject, query);
    }

    int n = static_cast<int>(subject.length()); // Cast size_type to int
    int m = static_cast<int>(query.length()); // Cast size_type to int

    // Determine the effective maximum edit distance
    int effective_max = std::min(static_cast<int>(max), n);

    // Re-initialize buffer before calculation
    std::iota(buffer, buffer + m + 1, 0);
