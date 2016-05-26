#pragma once

#include "attenuation_configs_serialize.h"
#include "config.h"
#include "vec_serialize.h"

namespace config {
template <typename Archive>
void serialize(Archive& archive, App& m) {
    archive(cereal::make_nvp("source", m.source),
            cereal::make_nvp("mic", m.mic),
            cereal::make_nvp("sample rate", m.sample_rate),
            cereal::make_nvp("bit depth", m.bit_depth),
            cereal::make_nvp("attenuation model", m.receiver_config));
}
}  // namespace config
JSON_OSTREAM_OVERLOAD(config::App);