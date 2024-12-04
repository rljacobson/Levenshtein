#include <arm_neon.h>
#include <string_view>

/// Returns the number of bytes from the left that are the same.
inline size_t find_first_mismatch(uint8x16_t comparison_result) {
    uint64_t lower = vgetq_lane_u64(vreinterpretq_u64_u8(comparison_result), 0);
    uint64_t upper = vgetq_lane_u64(vreinterpretq_u64_u8(comparison_result), 1);

    // Process lower and upper parts separately
    if (lower != 0xFFFFFFFFFFFFFFFF) {
        // Detect mismatch in the lower half
        size_t mismatch = __builtin_ctzll(~lower) / 8; // Find the mismatch byte
        return mismatch; // Adjust as needed for position
    }

    if (upper != 0xFFFFFFFFFFFFFFFF) {
        // Detect mismatch in the upper half
        size_t mismatch = 8 + (__builtin_ctzll(~upper) / 8); // Offset by 8 bytes
        return mismatch; // Adjust as needed for position
    }

    // If no mismatches
    return 16; // All bytes match
}

/// Returns the number of bytes from the right that are the same
inline size_t find_last_mismatch(uint8x16_t comparison_result) {
    uint64_t lower = vgetq_lane_u64(vreinterpretq_u64_u8(comparison_result), 0);
    uint64_t upper = vgetq_lane_u64(vreinterpretq_u64_u8(comparison_result), 1);

    // Process lower and upper parts separately, upper first

    if (upper != 0xFFFFFFFFFFFFFFFF) {
        // Detect mismatch in the upper half
        size_t mismatch = (__builtin_clzll(~upper) / 8); // Offset by 8 bytes
        return mismatch; // Adjust as needed for position
    }

    if (lower != 0xFFFFFFFFFFFFFFFF) {
        // Detect mismatch in the lower half
        size_t mismatch = 8 + __builtin_clzll(~lower) / 8; // Find the mismatch byte
        return mismatch; // Adjust as needed for position
    }

    // If no mismatches
    return 16; // All bytes match



    uint64_t mask = vgetq_lane_u64(vreinterpretq_u64_u8(comparison_result), 0) |
                    (static_cast<uint64_t>(vgetq_lane_u64(vreinterpretq_u64_u8(comparison_result), 1)) << 32);

    // Reverse the bit order to find the last `0x00` position
    return __builtin_clzll(~mask) / 8;
}

// Strip common prefix and suffix using SIMD
inline
std::pair<std::string_view, std::string_view>
strip_common_prefix_suffix(const char *str1, size_t len1, const char *str2, size_t len2) {
    size_t min_length = std::min(len1, len2);
    size_t prefix_length = 0;
    size_t suffix_length = 0;

    // Step 1: Compare common prefix using SIMD
    while (prefix_length + 16 <= min_length) {
        uint8x16_t v1 = vld1q_u8(reinterpret_cast<const uint8_t *>(str1 + prefix_length));
        uint8x16_t v2 = vld1q_u8(reinterpret_cast<const uint8_t *>(str2 + prefix_length));

        uint8x16_t cmp_result = vceqq_u8(v1, v2);

        // Check if all bytes match
        if (vminvq_u8(cmp_result) != 0xFF) {
            // They don't all match, find first mismatch
            prefix_length += find_first_mismatch(cmp_result);
            break;
        }
        prefix_length += 16;
    }

    // Process remaining bytes for the prefix
    while (prefix_length < min_length &&
           str1[prefix_length] == str2[prefix_length]) {
        ++prefix_length;
    }

    // Step 2: Compare common suffix using SIMD
    while (suffix_length + 16 <= min_length - prefix_length) {
        size_t offset1 = len1 - suffix_length - 16;
        size_t offset2 = len2 - suffix_length - 16;

        uint8x16_t v1 = vld1q_u8(reinterpret_cast<const uint8_t *>(str1 + offset1));
        uint8x16_t v2 = vld1q_u8(reinterpret_cast<const uint8_t *>(str2 + offset2));

        uint8x16_t cmp_result = vceqq_u8(v1, v2);

        // Check if all bytes match
        if (vminvq_u8(cmp_result) != 0xFF) {
            // They don't all match, find first mismatch
            size_t mismatch = find_last_mismatch(cmp_result);
            suffix_length += mismatch;
            break;
        }
        suffix_length += 16;
    }

    // Process remaining bytes for the suffix
    while (suffix_length < min_length - prefix_length &&
           str1[len1 - suffix_length - 1] == str2[len2 - suffix_length - 1]) {
        ++suffix_length;
    }

    // Return the stripped string views
    return {
            std::string_view(str1 + prefix_length, len1 - prefix_length - suffix_length),
            std::string_view(str2 + prefix_length, len2 - prefix_length - suffix_length)
    };
}
