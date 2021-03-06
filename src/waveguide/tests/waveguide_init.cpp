#include "waveguide/make_transparent.h"
#include "waveguide/mesh.h"
#include "waveguide/postprocessor/node.h"
#include "waveguide/preprocessor/soft_source.h"
#include "waveguide/waveguide.h"

#include "core/callback_accumulator.h"
#include "core/spatial_division/voxelised_scene_data.h"

#include "utilities/progress_bar.h"

#include "gtest/gtest.h"

#include <algorithm>

using namespace wayverb::waveguide;
using namespace wayverb::core;

TEST(waveguide_init, waveguide_init) {
    const compute_context cc{};

    auto scene_data =
            geo::get_scene_data(geo::box{glm::vec3{-1}, glm::vec3{1}},
                                make_surface<simulation_bands>(0.001, 0));

    const auto voxelised = make_voxelised_scene_data(scene_data, 5, 0.1f);

    constexpr glm::vec3 centre{0, 0, 0};

    const util::aligned::vector<float> input(20, 1);
    auto transparent =
            make_transparent(input.data(), input.data() + input.size());

    constexpr auto steps = 100;
    transparent.resize(steps, 0);

    constexpr auto speed_of_sound = 340.0;

    const auto model = compute_mesh(cc, voxelised, 0.04, speed_of_sound);
    const auto receiver_index = compute_index(model.get_descriptor(), centre);

    auto prep = preprocessor::make_soft_source(
            receiver_index, transparent.begin(), transparent.end());

    callback_accumulator<postprocessor::node> postprocessor{receiver_index};

    util::progress_bar pb;
    run(cc,
        model,
        [&](auto& queue, auto& buffer, auto step) {
            return prep(queue, buffer, step);
        },
        [&](auto& queue, const auto& buffer, auto step) {
            postprocessor(queue, buffer, step);
            set_progress(pb, step, steps);
        },
        true);

    ASSERT_EQ(transparent.size(), postprocessor.get_output().size());

    for (auto i = 0u; i != input.size(); ++i) {
        ASSERT_NEAR(postprocessor.get_output()[i], input[i], 0.0001) << i;
    }
}
