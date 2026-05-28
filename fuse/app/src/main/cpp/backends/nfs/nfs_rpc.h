#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace filefuse {

class XdrWriter {
public:
    void writeUint32(uint32_t value);
    void writeUint64(uint64_t value);
    void writeString(const std::string& value);
    void writeOpaque(const uint8_t* data, size_t size);
    void writeOpaque(const std::vector<uint8_t>& data);
    void appendRaw(const std::vector<uint8_t>& bytes);

    std::vector<uint8_t>& data() { return buffer_; }
    const std::vector<uint8_t>& data() const { return buffer_; }

private:
    std::vector<uint8_t> buffer_;
};

class XdrReader {
public:
    explicit XdrReader(const uint8_t* data, size_t size);

    bool readUint32(uint32_t& value);
    bool readUint64(uint64_t& value);
    bool readString(std::string& value);
    bool readOpaque(std::vector<uint8_t>& value, size_t fixedSize = 0);
    bool readRaw(uint8_t* out, size_t count);
    bool skip(size_t count);

    bool eof() const { return offset_ >= size_; }

private:
    const uint8_t* data_;
    size_t size_;
    size_t offset_ = 0;

    bool readBytes(uint8_t* out, size_t count);
};

class RpcClient {
public:
    bool connect(const std::string& host, uint16_t port);
    void close();

    bool call(uint32_t program, uint32_t version, uint32_t procedure,
              const std::vector<uint8_t>& args, std::vector<uint8_t>& replyBody);

private:
    int socketFd_ = -1;
    uint32_t xid_ = 1;

    bool sendAll(const uint8_t* data, size_t size);
    bool recvAll(uint8_t* data, size_t size);
};

}  // namespace filefuse
