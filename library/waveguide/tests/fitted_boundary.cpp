#include "waveguide/fitted_boundary.h"

#include "utilities/apply.h"
#include "utilities/decibels.h"
#include "utilities/for_each.h"
#include "utilities/zip.h"

#include "gtest/gtest.h"

namespace {

template <typename T, size_t B, size_t A>
std::ostream &operator<<(
        std::ostream &os,
        const std::tuple<std::array<T, B>, std::array<T, A>> &tup) {
    os << "b:\n";
    for (const auto &b : std::get<0>(tup)) {
        os << "    " << b << '\n';
    }

    os << "a:\n";
    for (const auto &a : std::get<1>(tup)) {
        os << "    " << a << '\n';
    }

    return os;
}

}  // namespace

TEST(fitted_boundary, eqnerror) {
    const auto test = [](const auto &a, const auto &b) {
        for_each(
                [](const auto &tup) {
                    for_each(
                            [](const auto &tup) {
                                ASSERT_NEAR(std::get<0>(tup),
                                            std::get<1>(tup),
                                            0.00000001);
                            },
                            zip(std::get<0>(tup), std::get<1>(tup)));
                },
                zip(a, b));
    };

    {
        const auto frequencies = std::array<double, 3>{{0.2, 0.4, 0.6}};
        test(waveguide::eqnerror<2, 2>(
                     frequencies,
                     waveguide::make_response(
                             std::array<double, 3>{{1, 1, 1}}, frequencies, 1),
                     std::array<double, 3>{{1, 1, 1}}),

             std::make_tuple(std::array<double, 3>{{
                                     -2.6366e-16, 1.0000e+00, 2.0390e-15,
                             }},
                             std::array<double, 3>{{
                                     1.0000e+00, 3.2715e-15, -4.4409e-16,
                             }}));
    }

    {
        const auto frequencies =
                map([](const auto &i) { return M_PI * i; },
                    std::array<double, 5>{{0.1, 0.25, 0.5, 0.75, 0.9}});

        const auto amplitudes =
                map([](const auto &i) { return decibels::db2a(i); },
                    std::array<double, 5>{{-40, -6, -24, -3, -40}});

        std::cout << waveguide::eqnerror<7, 7>(
                             frequencies,
                             waveguide::make_response(
                                     amplitudes, frequencies, 1),
                             std::array<double, 5>{{1, 1, 1, 1, 1}})
                  << '\n';
    }
}

TEST(fitted_boundary, compute_boundary_filter) {
    constexpr std::array<double, 5> centres{{0.2, 0.4, 0.6, 0.8, 1.0}};
    constexpr std::array<double, 5> amplitudes{{0, 1, 0.5, 1, 0}};
    constexpr auto order = 6;
    constexpr auto delay = 0.0;
    constexpr auto iterations = 10;

    const auto coeffs = waveguide::arbitrary_magnitude_filter<order>(
            centres, amplitudes, delay, iterations);

    std::cout << coeffs << '\n';
}