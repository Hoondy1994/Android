#include "audio_player.h"
#include <android/log.h>
#include <fstream>
#include <cstring>
#include <algorithm>

#define TAG "AudioPlayer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace android::audio {

AudioPlayer::AudioPlayer(AudioConfig cfg) : cfg_(cfg) {}

AudioPlayer::~AudioPlayer() {
    stop();
}

AudioResult<> AudioPlayer::load(const std::string& filePath) {
    stop();
    pcmData_.clear();

    std::ifstream f(filePath, std::ios::binary);
    if (!f.is_open()) return std::unexpected(AudioError::FILE_OPEN);

    WavFileHeader hdr{};
    f.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (!f || hdr.audioFormat != 1) return std::unexpected(AudioError::INVALID_FMT);

    fileCfg_.sampleRate    = static_cast<int32_t>(hdr.sampleRate);
    fileCfg_.channelCount  = static_cast<int32_t>(hdr.numChannels);
    fileCfg_.bitsPerSample = static_cast<int32_t>(hdr.bitsPerSample);

    std::size_t numSamples = hdr.dataSize / sizeof(Sample);
    pcmData_.resize(numSamples);
    f.read(reinterpret_cast<char*>(pcmData_.data()),
           static_cast<std::streamsize>(hdr.dataSize));
    if (!f && !f.eof()) return std::unexpected(AudioError::FILE_READ);

    playPos_.store(0);
    state_.store(AudioState::IDLE);
    LOGI("Loaded %zu samples, %lldms", numSamples, durationMs());
    return {};
}

AudioResult<> AudioPlayer::play() {
    if (pcmData_.empty())                   return std::unexpected(AudioError::INVALID_STATE);
    if (state_.load() == AudioState::PLAYING) return {};

    closeStream();
    playPos_.store(0);

    AAudioStreamBuilder* builder = nullptr;
    AAudio_createStreamBuilder(&builder);
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setSampleRate(builder, fileCfg_.sampleRate);
    AAudioStreamBuilder_setChannelCount(builder, fileCfg_.channelCount);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setDataCallback(builder, onAudioReady, this);
    AAudioStreamBuilder_setErrorCallback(builder, onError, this);

    aaudio_result_t res = AAudioStreamBuilder_openStream(builder, &stream_);
    AAudioStreamBuilder_delete(builder);
    if (res != AAUDIO_OK) return std::unexpected(AudioError::STREAM_OPEN);

    state_.store(AudioState::PLAYING);
    res = AAudioStream_requestStart(stream_);
    if (res != AAUDIO_OK) { closeStream(); return std::unexpected(AudioError::STREAM_START); }
    return {};
}

AudioResult<> AudioPlayer::pause() {
    if (state_.load() != AudioState::PLAYING) return std::unexpected(AudioError::INVALID_STATE);
    state_.store(AudioState::PAUSED);
    if (stream_) AAudioStream_requestPause(stream_);
    return {};
}

AudioResult<> AudioPlayer::resume() {
    if (state_.load() != AudioState::PAUSED) return std::unexpected(AudioError::INVALID_STATE);
    state_.store(AudioState::PLAYING);
    if (stream_) AAudioStream_requestStart(stream_);
    return {};
}

AudioResult<> AudioPlayer::stop() {
    auto s = state_.load();
    if (s == AudioState::IDLE) return {};
    state_.store(AudioState::IDLE);
    closeStream();
    playPos_.store(0);
    return {};
}

AudioResult<> AudioPlayer::seekTo(int64_t posMs) {
    if (pcmData_.empty()) return std::unexpected(AudioError::INVALID_STATE);
    int64_t sample = posMs * fileCfg_.sampleRate * fileCfg_.channelCount / 1000;
    const int64_t maxSample = static_cast<int64_t>(pcmData_.size());
    sample = std::clamp(sample, int64_t{0}, maxSample);
    playPos_.store(sample);
    return {};
}

int64_t AudioPlayer::durationMs() const noexcept {
    if (pcmData_.empty()) return 0;
    return fileCfg_.samplesToMs(static_cast<int64_t>(pcmData_.size()));
}

int64_t AudioPlayer::positionMs() const noexcept {
    return fileCfg_.samplesToMs(playPos_.load());
}

float AudioPlayer::progress() const noexcept {
    if (pcmData_.empty()) return 0.f;
    return static_cast<float>(playPos_.load()) / static_cast<float>(pcmData_.size());
}

aaudio_data_callback_result_t AudioPlayer::onAudioReady(
    AAudioStream*, void* userData, void* audioData, int32_t numFrames)
{
    auto* self  = static_cast<AudioPlayer*>(userData);
    if (self->state_.load() != AudioState::PLAYING) {
        std::memset(audioData, 0, static_cast<std::size_t>(numFrames) * sizeof(Sample) * self->fileCfg_.channelCount);
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    int64_t pos  = self->playPos_.load(std::memory_order_relaxed);
    int64_t need = static_cast<int64_t>(numFrames) * self->fileCfg_.channelCount;
    int64_t avail= static_cast<int64_t>(self->pcmData_.size()) - pos;
    int64_t copy = std::min(need, avail);

    auto* out = static_cast<Sample*>(audioData);
    if (copy > 0) {
        std::copy_n(self->pcmData_.data() + pos, copy, out);
        self->playPos_.store(pos + copy, std::memory_order_relaxed);
    }
    if (copy < need) {
        std::memset(out + copy, 0, static_cast<std::size_t>(need - copy) * sizeof(Sample));
        self->state_.store(AudioState::IDLE);
        if (self->onComplete_) self->onComplete_();
        return AAUDIO_CALLBACK_RESULT_STOP;
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void AudioPlayer::onError(AAudioStream*, void* userData, aaudio_result_t error) {
    auto* self = static_cast<AudioPlayer*>(userData);
    LOGE("error: %s", AAudio_convertResultToText(error));
    self->state_.store(AudioState::ERROR);
}

void AudioPlayer::closeStream() {
    if (stream_) {
        AAudioStream_requestStop(stream_);
        AAudioStream_close(stream_);
        stream_ = nullptr;
    }
}

} // namespace android::audio
