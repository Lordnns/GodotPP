#pragma once

#ifdef QNS_WITH_GODOT

#include "qns_serializer.hpp"
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    class QNSGodotUtils {
    public:
        static void write_variant(qns_core::QNSSerializer& s, const Variant& v) {
            PackedByteArray bytes = UtilityFunctions::var_to_bytes(v);
            s.write<uint32_t>(bytes.size());
            s.write_bytes(bytes.ptr(), bytes.size());
        }

        static Variant read_variant(qns_core::QNSDeserializer& d) {
            uint32_t len = d.read<uint32_t>();
            PackedByteArray bytes;
            bytes.resize(len);
            std::memcpy(bytes.ptrw(), d.read_bytes(len), len);
            return UtilityFunctions::bytes_to_var(bytes);
        }
    };
}

#endif // QNS_WITH_GODOT