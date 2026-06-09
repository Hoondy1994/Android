#include "ipc/custom_codec.hpp"

#include "ipc/binary_buffer.hpp"

#include <android/log.h>
#include <stdexcept>
#include <unistd.h>

#define LOG_TAG "SocketIPC"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace ipc {

std::vector<std::uint8_t> CustomCodec::encode(const UserProfile& profile) {
    ByteWriter writer;
    writer.write_u32(kCustomMagic);
    writer.write_i64(profile.id);
    writer.write_string(profile.name);
    writer.write_i32_array(profile.scores);
    writer.write_double(profile.rating);
    writer.write_i32(profile.checksum());
    return writer.take();
}

UserProfile CustomCodec::decode(const std::span<const std::uint8_t> data) {
    ByteReader reader(data);
    if (reader.read_u32() != kCustomMagic) {
        throw std::runtime_error("invalid custom payload magic");
    }

    UserProfile profile;
    profile.id = reader.read_i64();
    profile.name = reader.read_string();
    profile.scores = reader.read_i32_array();
    profile.rating = reader.read_double();
    const auto checksum = reader.read_i32();
    if (checksum != profile.checksum()) {
        throw std::runtime_error("custom payload checksum mismatch");
    }
    return profile;
}

std::vector<std::uint8_t> CustomCodec::process(const std::span<const std::uint8_t> data) {
    LOGI("[Flow:Codec] CustomCodec::process size=%zu, pid=%d", data.size(), getpid());
    return encode(decode(data).transformed());
}

}  // namespace ipc
