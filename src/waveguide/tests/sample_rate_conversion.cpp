#include "waveguide/config.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

std::vector<float> make_impulse(size_t size, size_t impulse_index) {
    std::vector<float> signal(size, 0.0f);
    signal.at(impulse_index) = 1.0f;
    return signal;
}

}  // namespace

TEST(sample_rate_conversion, impulse_resampling) {
    constexpr size_t impulse_index = 200;
    constexpr size_t input_size = 1000;
    constexpr double input_sr = 1000.0;

    const auto input = make_impulse(input_size, impulse_index);

    // Upsampling by a factor of two should double the frame count and keep
    // the impulse amplitude after gain correction.
    constexpr double upsampled_sr = 2000.0;
    const auto upsampled = wayverb::waveguide::adjust_sampling_rate(
            input, input_sr, upsampled_sr);

    const auto expected_up_size = static_cast<size_t>(input_size * 2);
    ASSERT_EQ(upsampled.size(), expected_up_size);

    const auto upsampled_max = std::abs(*std::max_element(
            upsampled.begin(),
            upsampled.end(),
            [](float lhs, float rhs) {
                return std::abs(lhs) < std::abs(rhs);
            }));
    const auto expected_up_amplitude = static_cast<float>(
            std::min(1.0, input_sr / upsampled_sr));
    EXPECT_NEAR(upsampled_max, expected_up_amplitude, 5e-2f);

    // Downsampling by a factor of two should halve the frame count and retain
    // the impulse amplitude.
    constexpr double downsampled_sr = 500.0;
    const auto downsampled = wayverb::waveguide::adjust_sampling_rate(
            input, input_sr, downsampled_sr);

    const auto expected_down_size = static_cast<size_t>(input_size / 2);
    ASSERT_EQ(downsampled.size(), expected_down_size);

    const auto downsampled_max = std::abs(*std::max_element(
            downsampled.begin(),
            downsampled.end(),
            [](float lhs, float rhs) {
                return std::abs(lhs) < std::abs(rhs);
            }));
    const auto expected_down_amplitude = static_cast<float>(
            std::min(1.0, input_sr / downsampled_sr));
    EXPECT_NEAR(downsampled_max, expected_down_amplitude, 5e-2f);
}
