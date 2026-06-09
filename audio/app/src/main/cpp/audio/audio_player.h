#pragma once
#include "audio_types.h"
#include <aaudio/AAudio.h>
#include <atomic>
#include <functional>
#include <string>
#include <vector>

namespace android::audio {

// 播放器：将 WAV 文件加载到内存，通过 AAudio 输出流播放
class AudioPlayer {
public:
    explicit AudioPlayer(AudioConfig cfg = kDefaultConfig);
    ~AudioPlayer();

    AudioPlayer(const AudioPlayer&)            = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;

    [[nodiscard]] AudioResult<> load(const std::string& filePath);
    [[nodiscard]] AudioResult<> play();
    [[nodiscard]] AudioResult<> pause();
    [[nodiscard]] AudioResult<> resume();
    [[nodiscard]] AudioResult<> stop();
    [[nodiscard]] AudioResult<> seekTo(int64_t posMs);

    [[nodiscard]] AudioState state()      const noexcept { return state_.load(); }
    [[nodiscard]] int64_t    durationMs() const noexcept;
    [[nodiscard]] int64_t    positionMs() const noexcept;
    [[nodiscard]] float      progress()   const noexcept; // 0.0 ~ 1.0

    using CompletionCallback = std::function<void()>;
    void setOnComplete(CompletionCallback cb) { onComplete_ = std::move(cb); }

private:
    static aaudio_data_callback_result_t onAudioReady(
        AAudioStream*, void* userData, void* audioData, int32_t numFrames);
    static void onError(AAudioStream*, void* userData, aaudio_result_t error);

    void closeStream();

    AudioConfig             cfg_;
    AudioConfig             fileCfg_;       // WAV 文件实际配置
    SampleBuffer            pcmData_;       // 整个文件的 PCM 数据
    std::atomic<int64_t>    playPos_{0};    // 当前读取位置（sample index）
    std::atomic<AudioState> state_{AudioState::IDLE};
    AAudioStream*           stream_  = nullptr;
    CompletionCallback      onComplete_;
};

} // namespace android::audio
