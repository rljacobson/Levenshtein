/*
Validate max distance and update.
This code is common to algorithms with limits.
*/

    if (args->args[2]) {
        long long user_max = *(reinterpret_cast<long long *>(args->args[2]));
        if (user_max < 0) {
            set_error(error, "Maximum edit distance cannot be negative.");
            // We can either return 0 or return null if given a bad max edit distance. This policy is configurable
            // in CMakeLists.txt.
#ifdef RETURN_ZERO_ON_BAD_MAX
            return 0;
#else
            *is_null = 1;
            return 1;
#endif
        }
        max = std::min(user_max, max);
    }
