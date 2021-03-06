#pragma once

#include "raytracer/cl/structs.h"
#include "raytracer/iterative_builder.h"

#include "core/cl/common.h"
#include "core/environment.h"
#include "core/spatial_division/scene_buffers.h"

namespace wayverb {
namespace raytracer {
namespace reflection_processor {

class visual_group_processor final {
public:
    explicit visual_group_processor(size_t items);

    template <typename It>
    void process(It b,
                 It /*e*/,
                 const core::scene_buffers& /*buffers*/,
                 size_t /*step*/,
                 size_t /*total*/) {
        builder_.push(b, b + builder_.get_num_items());
    }

    auto get_results() const { return builder_.get_data(); }

private:
    iterative_builder<reflection> builder_;
};

////////////////////////////////////////////////////////////////////////////////

class visual_processor final {
public:
    explicit visual_processor(size_t items);

    visual_group_processor get_group_processor(size_t num_directions) const;
    void accumulate(const visual_group_processor& processor);

    util::aligned::vector<util::aligned::vector<reflection>> get_results();

private:
    size_t items_;

    util::aligned::vector<util::aligned::vector<reflection>> results_;
};

////////////////////////////////////////////////////////////////////////////////

class make_visual final {
public:
    explicit make_visual(size_t items);

    visual_processor get_processor(
            const core::compute_context& cc,
            const glm::vec3& source,
            const glm::vec3& receiver,
            const core::environment& environment,
            const core::voxelised_scene_data<
                    cl_float3,
                    core::surface<core::simulation_bands>>& voxelised) const;

private:
    size_t items_;
};

}  // namespace reflection_processor
}  // namespace raytracer
}  // namespace wayverb
