#pragma once
#include "audio_types.h"
#include "ring_buffer.h"
#include <aaudio/AAudio.h>
#include <atomic>
#include <thread>
#include <string>
#include <fstream>

namespace android::audio {

// 录音器：AAudio 输入流 + SPSC 环形缓冲区 + 后台线程落盘
class AudioRecorder {
public:
    static constexpr std::size_t kRingCapacity = 1 << 18; // 256K samples ≈ 2.9s @44100

    explicit AudioRecorder(AudioConfig cfg = kDefaultConfig);
    ~AudioRecorder();

    AudioRecorder(const AudioRecorder&)            = delete;
    AudioRecorder& operator=(const AudioRecorder&) = delete;

    // 开始录音，写入 filePath（WAV格式）
    [[nodiscard]] AudioResult<> start(std::string filePath);
    // 停止录音，flush 并完成 WAV 文件头
    [[nodiscard]] AudioResult<> stop();

    [[nodiscard]] AudioState state()      const noexcept { return state_.load(); }
    [[nodiscard]] int64_t    recordedMs() const noexcept;

private:
    static aaudio_data_callback_result_t onAudioReady(
        AAudioStream*, void* userData,
        void* audioData, int32_t numFrames);
    static void onError(AAudioStream*, void* userData, aaudio_result_t error);

    void writerLoop();   // 运行在后台写盘线程中

    AudioConfig                             cfg_;
    AAudioStream*                           stream_     = nullptr;
    std::atomic<AudioState>                 state_      {AudioState::IDLE};
    std::atomic<int64_t>                    totalFrames_{0};

    RingBuffer<Sample, kRingCapacity>       ring_;
    std::thread                             writerThread_;

    std::string                             filePath_;
    std::ofstream                           file_;
    uint32_t                                bytesWritten_{0};
};

} // namespace android::audio
