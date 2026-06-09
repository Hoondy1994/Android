#pragma once

#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace ipc {

class ByteWriter {
public:
    void write_u8(std::uint8_t value) { bytes_.push_back(value); }

    void write_i32(std::int32_t value) { append_pod(value); }

    void write_i64(std::int64_t value) { append_pod(value); }

    void write_u32(std::uint32_t value) { append_pod(value); }

    void write_double(double value) { append_pod(value); }

    void write_bytes(std::span<const std::uint8_t> data) {
        write_u32(static_cast<std::uint32_t>(data.size()));
        bytes_.insert(bytes_.end(), data.begin(), data.end());
    }

    void write_string(std::string_view value) {
        write_bytes(std::span{
            reinterpret_cast<const std::uint8_t*>(value.data()),
            value.size(),
        });
    }

    void write_i32_array(std::span<const std::int32_t> values) {
        write_u32(static_cast<std::uint32_t>(values.size()));
        for (const std::int32_t value : values) {
            write_i32(value);
        }
    }

    [[nodiscard]] const std::vector<std::uint8_t>& bytes() const noexcept { return bytes_; }

    [[nodiscard]] std::vector<std::uint8_t> take() { return std::move(bytes_); }

private:
    template <typename T>
    void append_pod(T value) {
        const auto* ptr = reinterpret_cast<const std::uint8_t*>(&value);
        bytes_.insert(bytes_.end(), ptr, ptr + sizeof(T));
    }

    std::vector<std::uint8_t> bytes_;
};

class ByteReader {
public:
    explicit ByteReader(std::span<const std::uint8_t> data) : data_(data) {}

    [[nodiscard]] std::uint8_t read_u8() { return read_pod<std::uint8_t>(); }

    [[nodiscard]] std::int32_t read_i32() { return read_pod<std::int32_t>(); }

    [[nodiscard]] std::int64_t read_i64() { return read_pod<std::int64_t>(); }

    [[nodiscard]] std::uint32_t read_u32() { return read_pod<std::uint32_t>(); }

    [[nodiscard]] double read_double() { return read_pod<double>(); }

    [[nodiscard]] std::vector<std::uint8_t> read_bytes() {
        const auto size = read_u32();
        ensure(size);
        std::vector<std::uint8_t> out(size);
        std::memcpy(out.data(), data_.data() + offset_, size);
        offset_ += size;
        return out;
    }

    [[nodiscard]] std::string read_string() {
        const auto raw = read_bytes();
        return std::string(reinterpret_cast<const char*>(raw.data()), raw.size());
    }

    [[nodiscard]] std::vector<std::int32_t> read_i32_array() {
        const auto count = read_u32();
        std::vector<std::int32_t> out;
        out.reserve(count);
        for (std::uint32_t i = 0; i < count; ++i) {
            out.push_back(read_i32());
        }
        return out;
    }

    [[nodiscard]] bool empty() const noexcept { return offset_ >= data_.size(); }

private:
    template <typename T>
    [[nodiscard]] T read_pod() {
        ensure(sizeof(T));
        T value{};
        std::memcpy(&value, data_.data() + offset_, sizeof(T));
        offset_ += sizeof(T);
        return value;
    }

    void ensure(std::size_t size) {
        if (offset_ + size > data_.size()) {
            throw std::runtime_error("binary buffer underflow");
        }
    }

    std::span<const std::uint8_t> data_;
    std::size_t offset_{0};
};

}  // namespace ipc
