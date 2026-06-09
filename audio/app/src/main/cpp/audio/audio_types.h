#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <chrono>
#include <expected>
#include <span>
#include <vector>

namespace android::audio {

using Sample       = int16_t;
using SampleBuffer = std::vector<Sample>;

struct AudioConfig {
    int32_t sampleRate    = 44'100;
    int32_t channelCount  = 1;
    int32_t bitsPerSample = 16;
    [[nodiscard]] constexpr int32_t byteRate()   const noexcept { return sampleRate * channelCount * bitsPerSample / 8; }
    [[nodiscard]] constexpr int32_t blockAlign() const noexcept { return channelCount * bitsPerSample / 8; }
    [[nodiscard]] constexpr int64_t samplesToMs(int64_t n) const noexcept { return n * 1000 / (sampleRate * channelCount); }
};
inline constexpr AudioConfig kDefaultConfig{};

enum class AudioState : uint8_t { IDLE=0, RECORDING=1, PLAYING=2, PAUSED=3, ERROR=4 };
[[nodiscard]] constexpr std::string_view to_string(AudioState s) noexcept {
    switch(s){
        case AudioState::IDLE:      return "IDLE";
        case AudioState::RECORDING: return "RECORDING";
        case AudioState::PLAYING:   return "PLAYING";
        case AudioState::PAUSED:    return "PAUSED";
        case AudioState::ERROR:     return "ERROR";
        default:                    return "UNKNOWN";
    }
}

enum class AudioError : int32_t {
    NONE=0, STREAM_OPEN=-1, STREAM_START=-2, STREAM_STOP=-3,
    FILE_OPEN=-4, FILE_WRITE=-5, FILE_READ=-6, INVALID_STATE=-7, INVALID_FMT=-8
};
[[nodiscard]] constexpr std::string_view to_string(AudioError e) noexcept {
    switch(e){
        case AudioError::NONE:          return "OK";
        case AudioError::STREAM_OPEN:   return "STREAM_OPEN_FAILED";
        case AudioError::STREAM_START:  return "STREAM_START_FAILED";
        case AudioError::FILE_OPEN:     return "FILE_OPEN_FAILED";
        case AudioError::FILE_WRITE:    return "FILE_WRITE_FAILED";
        case AudioError::FILE_READ:     return "FILE_READ_FAILED";
        case AudioError::INVALID_STATE: return "INVALID_STATE";
        default:                        return "UNKNOWN";
    }
}
template<typename T = void>
using AudioResult = std::expected<T, AudioError>;

// WAV 文件头 44字节
#pragma pack(push, 1)
struct WavFileHeader {
    char     riffId[4]     = {'R','I','F','F'};
    uint32_t riffSize      = 0;
    char     waveId[4]     = {'W','A','V','E'};
    char     fmtId[4]      = {'f','m','t',' '};
    uint32_t fmtSize       = 16;
    uint16_t audioFormat   = 1;   // PCM
    uint16_t numChannels   = 1;
    uint32_t sampleRate    = 44100;
    uint32_t byteRate      = 88200;
    uint16_t blockAlign    = 2;
    uint16_t bitsPerSample = 16;
    char     dataId[4]     = {'d','a','t','a'};
    uint32_t dataSize      = 0;

    void configure(const AudioConfig& c) noexcept {
        numChannels   = static_cast<uint16_t>(c.channelCount);
        sampleRate    = static_cast<uint32_t>(c.sampleRate);
        byteRate      = static_cast<uint32_t>(c.byteRate());
        blockAlign    = static_cast<uint16_t>(c.blockAlign());
        bitsPerSample = static_cast<uint16_t>(c.bitsPerSample);
    }
    void finalize(uint32_t bytes) noexcept { dataSize = bytes; riffSize = 36 + bytes; }
};
#pragma pack(pop)
static_assert(sizeof(WavFileHeader) == 44);

struct RecordingInfo {
    std::string filePath;
    std::string displayName;
    int64_t     durationMs = 0;
    int64_t     sizeBytes  = 0;
};

[[nodiscard]] inline int64_t nowMs() noexcept {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
[[nodiscard]] inline std::string fmtDuration(int64_t ms) {
    int64_t s=ms/1000, m=s/60, h=m/60;
    char buf[16];
    if(h>0) std::snprintf(buf,sizeof(buf),"%lld:%02lld:%02lld",h,m%60,s%60);
    else     std::snprintf(buf,sizeof(buf),"%02lld:%02lld",m,s%60);
    return buf;
}

} // namespace android::audio
