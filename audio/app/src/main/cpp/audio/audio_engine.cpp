#include "audio_engine.h"
#include <android/log.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <fstream>

#define TAG "AudioEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)

namespace android::audio {

namespace fs = std::filesystem;

AudioEngine::AudioEngine(std::string storageDir, AudioConfig cfg)
    : cfg_(cfg)
    , storageDir_(std::move(storageDir))
    , recorder_(cfg)
    , player_(cfg)
{
    fs::create_directories(storageDir_);
}

// ─── 录音 ─────────────────────────────────────────────────────────────────────

AudioResult<std::string> AudioEngine::startRecording() {
    std::lock_guard lk(mtx_);
    if (player_.state() == AudioState::PLAYING || player_.state() == AudioState::PAUSED) {
        player_.stop();
    }
    std::string path = makeFilePath();
    auto res = recorder_.start(path);
    if (!res) return std::unexpected(res.error());
    return path;
}

AudioResult<> AudioEngine::stopRecording() {
    std::lock_guard lk(mtx_);
    return recorder_.stop();
}

int64_t AudioEngine::recordingElapsedMs() const noexcept {
    return recorder_.recordedMs();
}

// ─── 播放 ─────────────────────────────────────────────────────────────────────

AudioResult<> AudioEngine::startPlayback(const std::string& filePath) {
    std::lock_guard lk(mtx_);
    if (recorder_.state() == AudioState::RECORDING) return std::unexpected(AudioError::INVALID_STATE);
    auto res = player_.load(filePath);
    if (!res) return res;
    return player_.play();
}

AudioResult<> AudioEngine::pausePlayback()  { return player_.pause();  }
AudioResult<> AudioEngine::resumePlayback() { return player_.resume(); }
AudioResult<> AudioEngine::stopPlayback()   { return player_.stop();   }
AudioResult<> AudioEngine::seekTo(int64_t posMs) { return player_.seekTo(posMs); }

AudioState AudioEngine::engineState() const noexcept {
    if (recorder_.state() == AudioState::RECORDING) return AudioState::RECORDING;
    return player_.state();
}
int64_t AudioEngine::playPositionMs() const noexcept { return player_.positionMs(); }
int64_t AudioEngine::playDurationMs() const noexcept { return player_.durationMs(); }
float   AudioEngine::playProgress()   const noexcept { return player_.progress(); }

void AudioEngine::setOnPlaybackComplete(AudioPlayer::CompletionCallback cb) {
    player_.setOnComplete(std::move(cb));
}

// ─── 列表管理 ─────────────────────────────────────────────────────────────────

std::vector<RecordingInfo> AudioEngine::listRecordings() const {
    std::vector<RecordingInfo> result;
    std::error_code ec;
    for (auto& entry : fs::directory_iterator(storageDir_, ec)) {
        if (entry.path().extension() == ".wav") {
            result.push_back(buildInfo(entry.path()));
        }
    }
    // 按文件名排序（含时间戳，即时间倒序需 reverse）
    std::sort(result.begin(), result.end(), [](const RecordingInfo& a, const RecordingInfo& b){
        return a.filePath > b.filePath;
    });
    return result;
}

bool AudioEngine::deleteRecording(const std::string& filePath) {
    std::error_code ec;
    return fs::remove(filePath, ec);
}

// ─── 私有 ─────────────────────────────────────────────────────────────────────

std::string AudioEngine::makeFilePath() const {
    auto now  = std::chrono::system_clock::now();
    auto tt   = std::chrono::system_clock::to_time_t(now);
    std::tm   tm{};
    localtime_r(&tt, &tm);
    std::ostringstream ss;
    ss << storageDir_ << "/REC_"
       << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".wav";
    return ss.str();
}

RecordingInfo AudioEngine::buildInfo(const fs::path& p) const {
    RecordingInfo info;
    info.filePath    = p.string();
    info.displayName = p.filename().string();
    info.sizeBytes   = static_cast<int64_t>(fs::file_size(p));

    // 解析 WAV 头获取时长
    std::ifstream f(p, std::ios::binary);
    if (f) {
        WavFileHeader hdr{};
        f.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
        if (f && hdr.byteRate > 0) {
            info.durationMs = static_cast<int64_t>(hdr.dataSize) * 1000 / hdr.byteRate;
        }
    }
    return info;
}

} // namespace android::audio
