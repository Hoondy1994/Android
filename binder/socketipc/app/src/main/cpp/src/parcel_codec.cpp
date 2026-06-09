#include "ipc/parcel_codec.hpp"

#include <android/log.h>
#include <cstring>
#include <stdexcept>
#include <unistd.h>

#define LOG_TAG "SocketIPC"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace ipc {

void ParcelWriter::pad_to_4() {
    const auto remainder = bytes_.size() % 4;
    if (remainder != 0) {
        bytes_.insert(bytes_.end(), 4 - remainder, 0);
    }
}

void ParcelWriter::append_raw(const void* data, const std::size_t size) {
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    bytes_.insert(bytes_.end(), bytes, bytes + size);
}

void ParcelWriter::write_int32(const std::int32_t value) {
    append_raw(&value, sizeof(value));
}

void ParcelWriter::write_int64(const std::int64_t value) {
    append_raw(&value, sizeof(value));
}

void ParcelWriter::write_double(const double value) {
    append_raw(&value, sizeof(value));
}

void ParcelWriter::write_string(const std::string_view value) {
    write_int32(static_cast<std::int32_t>(value.size()));
    if (!value.empty()) {
        append_raw(value.data(), value.size());
    }
    pad_to_4();
}

void ParcelWriter::write_int32_array(const std::span<const std::int32_t> values) {
    write_int32(static_cast<std::int32_t>(values.size()));
    if (!values.empty()) {
        append_raw(values.data(), values.size() * sizeof(std::int32_t));
    }
}

std::vector<std::uint8_t> ParcelWriter::take() { return std::move(bytes_); }

ParcelReader::ParcelReader(const std::span<const std::uint8_t> data) : data_(data) {}

void ParcelReader::ensure(const std::size_t size) {
    if (offset_ + size > data_.size()) {
        throw std::runtime_error("parcel buffer underflow");
    }
}

void ParcelReader::align_to_4() {
    const auto remainder = offset_ % 4;
    if (remainder != 0) {
        offset_ += 4 - remainder;
    }
}

std::int32_t ParcelReader::read_int32() {
    ensure(sizeof(std::int32_t));
    std::int32_t value{};
    std::memcpy(&value, data_.data() + offset_, sizeof(value));
    offset_ += sizeof(value);
    return value;
}

std::int64_t ParcelReader::read_int64() {
    ensure(sizeof(std::int64_t));
    std::int64_t value{};
    std::memcpy(&value, data_.data() + offset_, sizeof(value));
    offset_ += sizeof(value);
    return value;
}

double ParcelReader::read_double() {
    ensure(sizeof(double));
    double value{};
    std::memcpy(&value, data_.data() + offset_, sizeof(value));
    offset_ += sizeof(value);
    return value;
}

std::string ParcelReader::read_string() {
    const auto size = read_int32();
    if (size < 0) {
        throw std::runtime_error("invalid parcel string length");
    }
    ensure(static_cast<std::size_t>(size));
    std::string value(reinterpret_cast<const char*>(data_.data() + offset_),
                      static_cast<std::size_t>(size));
    offset_ += static_cast<std::size_t>(size);
    align_to_4();
    return value;
}

std::vector<std::int32_t> ParcelReader::read_int32_array() {
    const auto count = read_int32();
    if (count < 0) {
        throw std::runtime_error("invalid parcel array length");
    }
    std::vector<std::int32_t> values(static_cast<std::size_t>(count));
    if (count > 0) {
        ensure(static_cast<std::size_t>(count) * sizeof(std::int32_t));
        std::memcpy(values.data(), data_.data() + offset_,
                    static_cast<std::size_t>(count) * sizeof(std::int32_t));
        offset_ += static_cast<std::size_t>(count) * sizeof(std::int32_t);
    }
    return values;
}

std::vector<std::uint8_t> ParcelCodec::encode(const UserProfile& profile) {
    ParcelWriter writer;
    writer.write_int64(profile.id);
    writer.write_string(profile.name);
    writer.write_int32_array(profile.scores);
    writer.write_double(profile.rating);
    writer.write_int32(profile.checksum());
    return writer.take();
}

UserProfile ParcelCodec::decode(const std::span<const std::uint8_t> data) {
    ParcelReader reader(data);
    UserProfile profile;
    profile.id = reader.read_int64();
    profile.name = reader.read_string();
    profile.scores = reader.read_int32_array();
    profile.rating = reader.read_double();
    const auto checksum = reader.read_int32();
    if (checksum != profile.checksum()) {
        throw std::runtime_error("parcel payload checksum mismatch");
    }
    return profile;
}

std::vector<std::uint8_t> ParcelCodec::process(const std::span<const std::uint8_t> data) {
    LOGI("[Flow:Codec] ParcelCodec::process size=%zu, pid=%d", data.size(), getpid());
    return encode(decode(data).transformed());
}

}  // namespace ipc
