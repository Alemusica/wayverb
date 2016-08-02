#include "waveguide/waveguide.h"
#include "waveguide/log_nan.h"
#include "waveguide/postprocessor/microphone.h"
#include "waveguide/postprocessor/visualiser.h"

#include "glog/logging.h"

#include <cassert>

namespace waveguide {

waveguide::~waveguide() noexcept = default;

waveguide::waveguide(const cl::Context& context,
                     const cl::Device& device,
                     const MeshBoundary& boundary,
                     const glm::vec3& anchor,
                     double sample_rate)
        : waveguide(context,
                    device,
                    mesh(boundary,
                         config::grid_spacing(SPEED_OF_SOUND, 1 / sample_rate),
                         anchor),
                    sample_rate,
                    program::to_filter_coefficients(boundary.get_surfaces(),
                                                    sample_rate)) {}

waveguide::waveguide(
        const cl::Context& context,
        const cl::Device& device,
        const mesh& mesh,
        double sample_rate,
        aligned::vector<program::CanonicalCoefficients> coefficients)
        : waveguide(context,
                    device,
                    mesh,
                    sample_rate,
                    mesh.get_condensed_nodes(),
                    coefficients) {}

waveguide::waveguide(
        const cl::Context& context,
        const cl::Device& device,
        const mesh& mesh,
        double sample_rate,
        aligned::vector<program::CondensedNodeStruct> nodes,
        aligned::vector<program::CanonicalCoefficients> coefficients)
        : queue(context, device)
        , program(context, device)
        , kernel(program.get_kernel())
        , lattice(mesh)
        , sample_rate(sample_rate)
        , previous(program.template get_info<CL_PROGRAM_CONTEXT>(),
                   CL_MEM_READ_WRITE,
                   nodes.size() * sizeof(cl_float))
        , current(program.template get_info<CL_PROGRAM_CONTEXT>(),
                  CL_MEM_READ_WRITE,
                  nodes.size() * sizeof(cl_float))
        , node_buffer(context, nodes.begin(), nodes.end(), true)
        , boundary_coefficients_buffer(
                  context, coefficients.begin(), coefficients.end(), true)
        , error_flag_buffer(context, CL_MEM_READ_WRITE, sizeof(cl_int)) {
    LOG(INFO) << "main memory node storage: "
              << (sizeof(program::NodeStruct) * mesh.get_nodes().size() >> 20)
              << " MB";
}

bool waveguide::init_and_run(
        const glm::vec3& excitation_location,
        const aligned::vector<float>& input,
        const aligned::vector<std::unique_ptr<step_postprocessor>>&
                postprocessors,
        const per_step_callback& callback,
        std::atomic_bool& keep_going) {
    //  init
    const auto context = program.get_info<CL_PROGRAM_CONTEXT>();

    auto boundary_buffer_1 =
            load_to_buffer(context, lattice.get_boundary_data<1>(), false);
    auto boundary_buffer_2 =
            load_to_buffer(context, lattice.get_boundary_data<2>(), false);
    auto boundary_buffer_3 =
            load_to_buffer(context, lattice.get_boundary_data<3>(), false);

    const auto input_node = get_index_for_coordinate(excitation_location);

    auto zero_mesh = [this](auto& buffer) {
        aligned::vector<cl_uchar> n(buffer.template getInfo<CL_MEM_SIZE>(), 0);
        cl::copy(queue, n.begin(), n.end(), buffer);
    };
    zero_mesh(previous);
    zero_mesh(current);

    //  run
    for (auto pressure : input) {
        if (!keep_going) {
            return false;
        }

        write_single_value(queue, error_flag_buffer, 0, program::id_success);

        //  add input pressure to current pressure at input node
        const auto current_pressure =
                read_single_value<cl_float>(queue, current, 0);
        const auto new_pressure = current_pressure + pressure;
        write_single_value(queue, current, input_node, new_pressure);

        //  run kernel
        kernel(cl::EnqueueArgs(queue, cl::NDRange(lattice.get_nodes().size())),
               previous,
               current,
               node_buffer,
               to_cl_int3(lattice.get_dim()),
               boundary_buffer_1,
               boundary_buffer_2,
               boundary_buffer_3,
               boundary_coefficients_buffer,
               error_flag_buffer);

        //  read out flag value
        auto flag = read_single_value<program::ErrorCode>(
                queue, error_flag_buffer, 0);
        if (flag & program::id_inf_error) {
            throw std::runtime_error(
                    "pressure value is inf, check filter coefficients");
        }

        if (flag & program::id_nan_error) {
            throw std::runtime_error(
                    "pressure value is nan, check filter coefficients");
        }

        if (flag & program::id_outside_mesh_error) {
            throw std::runtime_error("tried to read non-existant node");
        }

        if (flag & program::id_suspicious_boundary_error) {
            throw std::runtime_error("suspicious boundary read");
        }

        for (const auto& i : postprocessors) {
            i->process(queue, current);
        }

        std::swap(previous, current);
    }

    return true;
}

size_t waveguide::get_index_for_coordinate(const glm::vec3& v) const {
    return lattice.compute_index(lattice.compute_locator(v));
}

glm::vec3 waveguide::get_coordinate_for_index(size_t index) const {
    return to_vec3(lattice.get_nodes()[index].position);
}

const mesh& waveguide::get_mesh() const { return lattice; }
double waveguide::get_sample_rate() const { return sample_rate; }

bool waveguide::inside(size_t index) const {
    return lattice.get_nodes()[index].inside;
}

//----------------------------------------------------------------------------//

std::experimental::optional<aligned::vector<run_step_output>> init_and_run(
        waveguide& waveguide,
        const glm::vec3& e,
        const aligned::vector<float>& input,
        size_t output_node,
        size_t steps,
        std::atomic_bool& keep_going,
        const waveguide::per_step_callback& callback) {
    auto t = input;
    t.resize(steps, 0);

    aligned::vector<run_step_output> ret;
    ret.reserve(steps);
    aligned::vector<std::unique_ptr<waveguide::step_postprocessor>>
            postprocessors;
    postprocessors.push_back(
            std::make_unique<postprocessor::microphone>(
                    waveguide.get_mesh(),
                    output_node,
                    waveguide.get_sample_rate(),
                    [&ret](float pressure, const glm::vec3& intensity) {
                        ret.push_back(run_step_output{intensity, pressure});
                    }));

    if (waveguide.init_and_run(e, t, postprocessors, callback, keep_going)) {
        return ret;
    }

    return std::experimental::nullopt;
}

std::experimental::optional<aligned::vector<run_step_output>> init_and_run(
        waveguide& waveguide,
        const glm::vec3& e,
        const aligned::vector<float>& input,
        size_t output_node,
        size_t steps,
        std::atomic_bool& keep_going,
        const waveguide::per_step_callback& callback,
        const waveguide::visualiser_callback& visual_callback) {
    auto t = input;
    t.resize(steps, 0);

    aligned::vector<run_step_output> ret;
    ret.reserve(steps);
    aligned::vector<std::unique_ptr<waveguide::step_postprocessor>>
            postprocessors;
    postprocessors.push_back(
            std::make_unique<postprocessor::microphone>(
                    waveguide.get_mesh(),
                    output_node,
                    waveguide.get_sample_rate(),
                    [&ret](float pressure, const glm::vec3& intensity) {
                        ret.push_back(run_step_output{intensity, pressure});
                    }));
    postprocessors.push_back(
            std::make_unique<postprocessor::visualiser>(
                    waveguide.get_mesh().get_nodes().size(),
                    [&visual_callback](const auto& i) { visual_callback(i); }));

    if (waveguide.init_and_run(e, t, postprocessors, callback, keep_going)) {
        return ret;
    }

    return std::experimental::nullopt;
}

}  // namespace waveguide
