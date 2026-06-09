#pragma once
#include "audio_types.h"
#include "audio_recorder.h"
#include "audio_player.h"
#include <mutex>
#include <vector>
#include <string>
#include <filesystem>

namespace android::audio {

// 顶层引擎：管理录音列表、协调 Recorder / Player
class AudioEngine {
public:
    explicit AudioEngine(std::string storageDir, AudioConfig cfg = kDefaultConfig);
    ~AudioEngine() = default;

    AudioEngine(const AudioEngine&)            = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    // 录音
    [[nodiscard]] AudioResult<std::string> startRecording();  // 返回文件路径
    [[nodiscard]] AudioResult<>            stopRecording();
    [[nodiscard]] int64_t recordingElapsedMs() const noexcept;

    // 播放
    [[nodiscard]] AudioResult<> startPlayback(const std::string& filePath);
    [[nodiscard]] AudioResult<> pausePlayback();
    [[nodiscard]] AudioResult<> resumePlayback();
    [[nodiscard]] AudioResult<> stopPlayback();
    [[nodiscard]] AudioResult<> seekTo(int64_t posMs);

    // 状态查询
    [[nodiscard]] AudioState engineState() const noexcept;
    [[nodiscard]] int64_t    playPositionMs() const noexcept;
    [[nodiscard]] int64_t    playDurationMs() const noexcept;
    [[nodiscard]] float      playProgress()   const noexcept;

    // 录音列表（从磁盘扫描）
    [[nodiscard]] std::vector<RecordingInfo> listRecordings() const;
    [[nodiscard]] bool deleteRecording(const std::string& filePath);

    void setOnPlaybackComplete(AudioPlayer::CompletionCallback cb);

private:
    [[nodiscard]] std::string makeFilePath() const;
    [[nodiscard]] RecordingInfo buildInfo(const std::filesystem::path& p) const;

    AudioConfig      cfg_;
    std::string      storageDir_;
    AudioRecorder    recorder_;
    AudioPlayer      player_;
    mutable std::mutex mtx_;
};

} // namespace android::audio
