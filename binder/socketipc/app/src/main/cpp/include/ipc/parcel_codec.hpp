#pragma once

#include "ipc/types.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace ipc {

// 模拟 Android Parcel 的 4 字节对齐写入规则。
class ParcelWriter {
public:
    void write_int32(std::int32_t value);
    void write_int64(std::int64_t value);
    void write_double(double value);
    void write_string(std::string_view value);
    void write_int32_array(std::span<const std::int32_t> values);

    [[nodiscard]] std::vector<std::uint8_t> take();

private:
    void pad_to_4();
    void append_raw(const void* data, std::size_t size);

    std::vector<std::uint8_t> bytes_;
};

class ParcelReader {
public:
    explicit ParcelReader(std::span<const std::uint8_t> data);

    [[nodiscard]] std::int32_t read_int32();
    [[nodiscard]] std::int64_t read_int64();
    [[nodiscard]] double read_double();
    [[nodiscard]] std::string read_string();
    [[nodiscard]] std::vector<std::int32_t> read_int32_array();

private:
    void ensure(std::size_t size);
    void align_to_4();

    std::span<const std::uint8_t> data_;
    std::size_t offset_{0};
};

class ParcelCodec {
public:
    [[nodiscard]] static std::vector<std::uint8_t> encode(const UserProfile& profile);
    [[nodiscard]] static UserProfile decode(std::span<const std::uint8_t> data);
    [[nodiscard]] static std::vector<std::uint8_t> process(std::span<const std::uint8_t> data);
};

}  // namespace ipc
