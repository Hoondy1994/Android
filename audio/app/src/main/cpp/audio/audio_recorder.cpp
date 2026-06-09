#include "audio_recorder.h"
#include <android/log.h>
#include <array>
#include <cstring>
#include <chrono>
#include <thread>

#define TAG "AudioRecorder"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace android::audio {

AudioRecorder::AudioRecorder(AudioConfig cfg) : cfg_(cfg) {}

AudioRecorder::~AudioRecorder() {
    if (state_.load() == AudioState::RECORDING) {
        stop();
    }
    if (stream_) {
        AAudioStream_close(stream_);
        stream_ = nullptr;
    }
}

AudioResult<> AudioRecorder::start(std::string filePath) {
    if (state_.load() != AudioState::IDLE) {
        return std::unexpected(AudioError::INVALID_STATE);
    }

    // 打开 WAV 文件
    filePath_ = std::move(filePath);
    file_.open(filePath_, std::ios::binary | std::ios::trunc);
    if (!file_.is_open()) return std::unexpected(AudioError::FILE_OPEN);

    // 写占位 WAV 头（stop 时回填）
    WavFileHeader hdr;
    hdr.configure(cfg_);
    file_.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    bytesWritten_ = 0;
    totalFrames_.store(0);
    ring_.reset();

    // 构建 AAudio 输入流
    AAudioStreamBuilder* builder = nullptr;
    AAudio_createStreamBuilder(&builder);
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
    AAudioStreamBuilder_setInputPreset(builder, AAUDIO_INPUT_PRESET_VOICE_RECOGNITION);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setSampleRate(builder, cfg_.sampleRate);
    AAudioStreamBuilder_setChannelCount(builder, cfg_.channelCount);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setDataCallback(builder, onAudioReady, this);
    AAudioStreamBuilder_setErrorCallback(builder, onError, this);

    aaudio_result_t res = AAudioStreamBuilder_openStream(builder, &stream_);
    AAudioStreamBuilder_delete(builder);
    if (res != AAUDIO_OK) {
        LOGE("openStream failed: %s", AAudio_convertResultToText(res));
        file_.close();
        return std::unexpected(AudioError::STREAM_OPEN);
    }

    // 启动写盘线程（NDK libc++ 在 API 28 上无 std::jthread）
    if (writerThread_.joinable()) writerThread_.join();
    writerThread_ = std::thread([this] { writerLoop(); });

    state_.store(AudioState::RECORDING);
    res = AAudioStream_requestStart(stream_);
    if (res != AAUDIO_OK) {
        LOGE("requestStart failed: %s", AAudio_convertResultToText(res));
        state_.store(AudioState::IDLE);
        if (writerThread_.joinable()) writerThread_.join();
        return std::unexpected(AudioError::STREAM_START);
    }

    LOGI("Recording started → %s", filePath_.c_str());
    return {};
}

AudioResult<> AudioRecorder::stop() {
    if (state_.load() != AudioState::RECORDING) {
        return std::unexpected(AudioError::INVALID_STATE);
    }
    state_.store(AudioState::IDLE);

    if (stream_) {
        AAudioStream_requestStop(stream_);
        AAudioStream_close(stream_);
        stream_ = nullptr;
    }

    // 等待写线程把 ring 中剩余数据 flush 完毕
    if (writerThread_.joinable()) writerThread_.join();

    // 回填 WAV 头
    if (file_.is_open()) {
        WavFileHeader hdr;
        hdr.configure(cfg_);
        hdr.finalize(bytesWritten_);
        file_.seekp(0);
        file_.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
        file_.close();
    }

    LOGI("Recording stopped. bytes=%u, ms=%lld", bytesWritten_, recordedMs());
    return {};
}

int64_t AudioRecorder::recordedMs() const noexcept {
    return cfg_.samplesToMs(totalFrames_.load());
}

aaudio_data_callback_result_t AudioRecorder::onAudioReady(
    AAudioStream*, void* userData, void* audioData, int32_t numFrames)
{
    auto* self = static_cast<AudioRecorder*>(userData);
    if (self->state_.load() != AudioState::RECORDING) {
        return AAUDIO_CALLBACK_RESULT_STOP;
    }
    std::span<const Sample> src(static_cast<const Sample*>(audioData),
                                static_cast<std::size_t>(numFrames * self->cfg_.channelCount));
    self->ring_.write(src);
    self->totalFrames_.fetch_add(numFrames, std::memory_order_relaxed);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void AudioRecorder::onError(AAudioStream*, void* userData, aaudio_result_t error) {
    auto* self = static_cast<AudioRecorder*>(userData);
    LOGE("AAudio error: %s", AAudio_convertResultToText(error));
    self->state_.store(AudioState::ERROR);
}

void AudioRecorder::writerLoop() {
    constexpr std::size_t kChunk = 4096;
    std::array<Sample, kChunk> buf{};

    while (true) {
        std::size_t n = ring_.read(std::span{buf});
        if (n > 0) {
            uint32_t byteCount = static_cast<uint32_t>(n * sizeof(Sample));
            file_.write(reinterpret_cast<const char*>(buf.data()), byteCount);
            bytesWritten_ += byteCount;
        } else {
            // 仅当录音已停止且 ring 为空时退出
            if (state_.load() != AudioState::RECORDING) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    // 最后一次 flush
    while (!ring_.empty()) {
        std::size_t n = ring_.read(std::span{buf});
        if (n > 0) {
            uint32_t byteCount = static_cast<uint32_t>(n * sizeof(Sample));
            file_.write(reinterpret_cast<const char*>(buf.data()), byteCount);
            bytesWritten_ += byteCount;
        }
    }
    file_.flush();
}

} // namespace android::audio
