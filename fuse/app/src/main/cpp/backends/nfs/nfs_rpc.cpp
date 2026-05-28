#include "nfs_rpc.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>

namespace filefuse {

namespace {

constexpr uint32_t kRpcCall = 0;
constexpr uint32_t kRpcReply = 1;
constexpr uint32_t kRpcMsgAccepted = 0;
constexpr uint32_t kRpcSuccess = 0;

void appendPadding(std::vector<uint8_t>& buffer, size_t length) {
    const size_t remainder = length % 4;
    if (remainder != 0) {
        buffer.insert(buffer.end(), 4 - remainder, 0);
    }
}

}  // namespace

void XdrWriter::writeUint32(uint32_t value) {
    const uint32_t net = htonl(value);
    const auto* bytes = reinterpret_cast<const uint8_t*>(&net);
    buffer_.insert(buffer_.end(), bytes, bytes + 4);
}

void XdrWriter::writeUint64(uint64_t value) {
    const uint32_t high = static_cast<uint32_t>(value >> 32);
    const uint32_t low = static_cast<uint32_t>(value & 0xffffffffu);
    writeUint32(high);
    writeUint32(low);
}

void XdrWriter::writeString(const std::string& value) {
    writeUint32(static_cast<uint32_t>(value.size()));
    buffer_.insert(buffer_.end(), value.begin(), value.end());
    appendPadding(buffer_, value.size());
}

void XdrWriter::writeOpaque(const uint8_t* data, size_t size) {
    writeUint32(static_cast<uint32_t>(size));
    buffer_.insert(buffer_.end(), data, data + size);
    appendPadding(buffer_, size);
}

void XdrWriter::writeOpaque(const std::vector<uint8_t>& data) {
    writeOpaque(data.data(), data.size());
}

void XdrWriter::appendRaw(const std::vector<uint8_t>& bytes) {
    buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
}

XdrReader::XdrReader(const uint8_t* data, size_t size) : data_(data), size_(size) {}

bool XdrReader::readBytes(uint8_t* out, size_t count) {
    if (offset_ + count > size_) {
        return false;
    }
    if (out != nullptr) {
        std::memcpy(out, data_ + offset_, count);
    }
    offset_ += count;
    return true;
}

bool XdrReader::skip(size_t count) {
    return readBytes(nullptr, count);
}

bool XdrReader::readRaw(uint8_t* out, size_t count) {
    return readBytes(out, count);
}

bool XdrReader::readUint32(uint32_t& value) {
    uint32_t net = 0;
    if (!readBytes(reinterpret_cast<uint8_t*>(&net), 4)) {
        return false;
    }
    value = ntohl(net);
    return true;
}

bool XdrReader::readUint64(uint64_t& value) {
    uint32_t high = 0;
    uint32_t low = 0;
    if (!readUint32(high) || !readUint32(low)) {
        return false;
    }
    value = (static_cast<uint64_t>(high) << 32) | low;
    return true;
}

bool XdrReader::readString(std::string& value) {
    uint32_t length = 0;
    if (!readUint32(length) || offset_ + length > size_) {
        return false;
    }
    value.assign(reinterpret_cast<const char*>(data_ + offset_), length);
    offset_ += length;
    const size_t remainder = length % 4;
    if (remainder != 0) {
        offset_ += 4 - remainder;
    }
    return true;
}

bool XdrReader::readOpaque(std::vector<uint8_t>& value, size_t fixedSize) {
    uint32_t length = 0;
    if (fixedSize == 0) {
        if (!readUint32(length) || offset_ + length > size_) {
            return false;
        }
    } else {
        length = static_cast<uint32_t>(fixedSize);
        if (offset_ + length > size_) {
            return false;
        }
    }

    value.assign(data_ + offset_, data_ + offset_ + length);
    offset_ += length;
    const size_t remainder = length % 4;
    if (remainder != 0) {
        offset_ += 4 - remainder;
    }
    return true;
}

bool RpcClient::connect(const std::string& host, uint16_t port) {
    close();

    addrinfo hints {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
        return false;
    }

    for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        socketFd_ = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (socketFd_ < 0) {
            continue;
        }
        if (::connect(socketFd_, ptr->ai_addr, ptr->ai_addrlen) == 0) {
            break;
        }
        close();
    }

    freeaddrinfo(result);
    return socketFd_ >= 0;
}

void RpcClient::close() {
    if (socketFd_ >= 0) {
        ::close(socketFd_);
        socketFd_ = -1;
    }
}

bool RpcClient::sendAll(const uint8_t* data, size_t size) {
    size_t sent = 0;
    while (sent < size) {
        const ssize_t result = send(socketFd_, data + sent, size - sent, 0);
        if (result <= 0) {
            return false;
        }
        sent += static_cast<size_t>(result);
    }
    return true;
}

bool RpcClient::recvAll(uint8_t* data, size_t size) {
    size_t received = 0;
    while (received < size) {
        const ssize_t result = recv(socketFd_, data + received, size - received, 0);
        if (result <= 0) {
            return false;
        }
        received += static_cast<size_t>(result);
    }
    return true;
}

bool RpcClient::call(uint32_t program, uint32_t version, uint32_t procedure,
                     const std::vector<uint8_t>& args, std::vector<uint8_t>& replyBody) {
    if (socketFd_ < 0) {
        return false;
    }

    XdrWriter writer;
    writer.writeUint32(xid_++);
    writer.writeUint32(kRpcCall);
    writer.writeUint32(2);
    writer.writeUint32(program);
    writer.writeUint32(version);
    writer.writeUint32(procedure);
    writer.writeUint32(0);
    writer.writeUint32(0);
    writer.writeUint32(0);
    writer.writeUint32(0);
    writer.appendRaw(args);

    const auto& request = writer.data();
    const uint32_t recordLength = htonl(static_cast<uint32_t>(request.size()));
    if (!sendAll(reinterpret_cast<const uint8_t*>(&recordLength), 4)) {
        return false;
    }
    if (!sendAll(request.data(), request.size())) {
        return false;
    }

    uint32_t responseLengthNet = 0;
    if (!recvAll(reinterpret_cast<uint8_t*>(&responseLengthNet), 4)) {
        return false;
    }
    const uint32_t responseLength = ntohl(responseLengthNet);
    if (responseLength == 0) {
        return false;
    }

    std::vector<uint8_t> response(responseLength);
    if (!recvAll(response.data(), response.size())) {
        return false;
    }

    XdrReader reader(response.data(), response.size());
    uint32_t xid = 0;
    uint32_t msgType = 0;
    uint32_t replyStat = 0;
    uint32_t acceptStat = 0;
    if (!reader.readUint32(xid) || !reader.readUint32(msgType) || msgType != kRpcReply) {
        return false;
    }
    if (!reader.readUint32(replyStat) || replyStat != kRpcMsgAccepted) {
        return false;
    }
    if (!reader.readUint32(acceptStat) || acceptStat != kRpcSuccess) {
        return false;
    }

    uint32_t verfFlavor = 0;
    uint32_t verfLength = 0;
    if (!reader.readUint32(verfFlavor) || !reader.readUint32(verfLength)) {
        return false;
    }
    if (verfLength > 0 && !reader.skip(verfLength)) {
        return false;
    }

    replyBody.clear();
    while (!reader.eof()) {
        uint8_t byte = 0;
        if (!reader.readRaw(&byte, 1)) {
            break;
        }
        replyBody.push_back(byte);
    }
    return !replyBody.empty();
}

}  // namespace filefuse
