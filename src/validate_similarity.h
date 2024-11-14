/*

 Code shared by all algorithms that use similarity.

*/

    if (args->args[2]){
        similarity = std::max(
                        *(reinterpret_cast<double *>(args->args[2])),
                        similarity
                    );
        if (similarity > 1.0 || similarity < 0.0)  {
            set_error(error, "Similarity must be in the interval (0.0, 1.0).");
            // We can either return 0 or return null if given a bad similarity. This policy is configurable
            // in CMakeLists.txt.
#ifdef RETURN_ZERO_ON_BAD_MAX
            return 0;
#else
            *is_null = 1;
            return 1;
#endif
        }
    } else {
        // This case should never happen.
        set_error(error, "Similarity doesn't exist. This is a bug and should never happen.");
#ifdef RETURN_ZERO_ON_BAD_MAX
        return 0;
#else
        *is_null = 1;
        return 1;
#endif
    }
