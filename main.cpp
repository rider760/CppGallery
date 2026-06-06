#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>
#include <cstdio>

#if defined(_WIN32)
#include <windows.h>
#include <dwmapi.h>
#include <objbase.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#endif

#include <GLFW/glfw3.h>
#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_PIXEL_UNPACK_BUFFER
#define GL_PIXEL_UNPACK_BUFFER 0x88EC
#endif
#ifndef GL_STREAM_DRAW
#define GL_STREAM_DRAW 0x88E0
#endif
#ifndef GL_MAP_WRITE_BIT
#define GL_MAP_WRITE_BIT 0x0002
#endif
#ifndef GL_MAP_INVALIDATE_BUFFER_BIT
#define GL_MAP_INVALIDATE_BUFFER_BIT 0x0008
#endif

namespace fs = std::filesystem;

namespace {

using GlGenerateMipmapProc = void (*)(unsigned int target);
using GlGenBuffersProc = void (*)(int n, unsigned int* buffers);
using GlBindBufferProc = void (*)(unsigned int target, unsigned int buffer);
using GlBufferDataProc = void (*)(unsigned int target, std::ptrdiff_t size, const void* data, unsigned int usage);
using GlBufferSubDataProc = void (*)(unsigned int target, std::ptrdiff_t offset, std::ptrdiff_t size, const void* data);
using GlMapBufferRangeProc = void* (*)(unsigned int target, std::ptrdiff_t offset, std::ptrdiff_t length, unsigned int access);
using GlUnmapBufferProc = unsigned char (*)(unsigned int target);
using GlDeleteBuffersProc = void (*)(int n, const unsigned int* buffers);
GlGenerateMipmapProc g_glGenerateMipmap = nullptr;
GlGenBuffersProc g_glGenBuffers = nullptr;
GlBindBufferProc g_glBindBuffer = nullptr;
GlBufferDataProc g_glBufferData = nullptr;
GlBufferSubDataProc g_glBufferSubData = nullptr;
GlMapBufferRangeProc g_glMapBufferRange = nullptr;
GlUnmapBufferProc g_glUnmapBuffer = nullptr;
GlDeleteBuffersProc g_glDeleteBuffers = nullptr;

constexpr float kOuterPadding = 0.0f;
constexpr float kTileSpacing = 0.0f;
constexpr float kTargetTileWidth = 300.0f;
constexpr float kMinTileWidth = 140.0f;
constexpr int kDefaultColumnCount = 12;
constexpr int kMaxColumnCount = 18;
constexpr float kBucketHeight = 1024.0f;
constexpr std::size_t kTextureUploadPboCount = 3;
constexpr float kWheelStep = 130.0f;
constexpr double kWheelScrollCatchupSeconds = 0.09;
constexpr double kWheelScrollMaxSpeedPixelsPerSecond = 6200.0;
constexpr double kWheelScrollAccelerationPixelsPerSecond2 = 48000.0;
constexpr double kWheelScrollSnapDistance = 0.5;
constexpr double kWheelScrollSnapVelocityPixelsPerSecond = 24.0;
constexpr float kZoomWheelLinearStep = 0.16f;
constexpr float kZoomScaleMin = 0.35f;
constexpr float kZoomScaleMax = 8.0f;
constexpr float kZoomDragThreshold = 6.0f;
constexpr int kVideoPreviewMaxFrames = 36;
constexpr int kVideoMaxDecodeDimension = 512;
constexpr double kVideoPreviewDurationSeconds = 2.5;
constexpr double kVideoMinPlaybackFps = 6.0;
constexpr double kVideoMaxPlaybackFps = 12.0;
constexpr DWORD kVideoProbeTimeoutMs = 4000;
constexpr DWORD kVideoDecodeTimeoutMs = 15000;
constexpr std::uintmax_t kProbeFileBytesMax = 64ULL * 1024ULL * 1024ULL;
constexpr std::uintmax_t kStaticImageFileBytesMax = 96ULL * 1024ULL * 1024ULL;
constexpr std::uintmax_t kGifFileBytesMax = 32ULL * 1024ULL * 1024ULL;
constexpr std::uintmax_t kDecodedPixelCountMax = 96ULL * 1024ULL * 1024ULL;
constexpr std::uintmax_t kDecodedImageBytesMax = 512ULL * 1024ULL * 1024ULL;
constexpr std::size_t kStbiProbeIoBudgetBytes = 64ULL * 1024ULL * 1024ULL;
constexpr std::size_t kStbiDecodeIoBudgetBytesMin = 64ULL * 1024ULL * 1024ULL;
constexpr std::size_t kStbiDecodeIoBudgetBytesMax = 512ULL * 1024ULL * 1024ULL;
constexpr int kGalleryDecodeDimensionMin = 768;
constexpr int kGalleryDecodeDimensionMax = 1280;
constexpr int kZoomDecodeDimensionMax = 4096;
#if defined(_WIN32)
constexpr int kUiThreadBasePriority = THREAD_PRIORITY_ABOVE_NORMAL;
constexpr int kUiThreadActivePriority = THREAD_PRIORITY_HIGHEST;
constexpr int kAppIconResourceId = 101;
constexpr DWORD kDwmwaUseImmersiveDarkModeBefore20 = 19;
constexpr DWORD kDwmwaUseImmersiveDarkMode = 20;
constexpr DWORD kDwmwaWindowCornerPreference = 33;
constexpr DWORD kDwmwaSystemBackdropType = 38;
constexpr int kDwmWindowCornerRound = 2;
constexpr int kDwmSystemBackdropMainWindow = 2;
constexpr int kAccentEnableBlurBehind = 3;
constexpr int kAccentEnableAcrylicBlurBehind = 4;
#endif
constexpr int kScrollPreviewDecodeDimensionMin = 384;
constexpr int kScrollPreviewDecodeDimensionMax = 768;
constexpr int kGalleryUpgradeSlackPixels = 96;
constexpr double kInteractiveScrollBoostVelocityPixelsPerSecond = 80.0;
constexpr double kFastScrollVelocityPixelsPerSecond = 1800.0;
constexpr int kInteractiveScrollGalleryDecodeConcurrency = 2;
constexpr int kScrollingGalleryDecodeConcurrency = 3;
constexpr int kIdleGalleryDecodeConcurrencyCeiling = 4;
constexpr float kFastScrollActiveBandScreens = 0.75f;
constexpr float kFastScrollPreloadBandScreens = 1.25f;
constexpr float kUploadScreens = 2.0f;
constexpr float kPreloadScreens = 4.0f;
constexpr float kEvictScreens = 24.0f;
constexpr std::size_t kMaxActiveTexturesSafety = 4096;
constexpr std::size_t kMaxGpuTextureBytes = 8ULL * 1024ULL * 1024ULL * 1024ULL;
constexpr std::size_t kMaxCpuImageCacheBytes = 8ULL * 1024ULL * 1024ULL * 1024ULL;
constexpr int kGpuScaleEligibleDimensionMax = 3072;
constexpr std::size_t kGpuScaleEligiblePixelBytesMax = 24ULL * 1024ULL * 1024ULL;
constexpr float kGpuScaleEligibleRatioMax = 2.4f;
constexpr float kFastScrollGpuScaleEligibleRatioMax = 4.0f;
constexpr int kCpuCacheRetainDimensionMax = 2048;
constexpr std::size_t kCpuCacheRetainPixelBytesMax = 12ULL * 1024ULL * 1024ULL;
constexpr int kMaxDecodedPumpPerFrame = 32;
constexpr int kMaxUploadsPerFrame = 6;
constexpr int kMaxQueuedDecodesPerFrame = 16;
constexpr std::size_t kMaxInflightDecodes = 48;
constexpr int kFastScrollMaxDecodedPumpPerFrame = 2;
constexpr int kFastScrollMaxUploadsPerFrame = 4;
constexpr int kFastScrollMaxQueuedDecodesPerFrame = 4;
constexpr std::size_t kFastScrollInflightDecodes = 8;
constexpr std::size_t kFastScrollQueueTrimSize = 4;

constexpr ImU32 kBackgroundTop = IM_COL32(7, 10, 16, 255);
constexpr ImU32 kBackgroundBottom = IM_COL32(18, 24, 34, 255);
constexpr ImU32 kBorderColor = IM_COL32(255, 255, 255, 0);
constexpr ImU32 kOverlayBackdrop = IM_COL32(0, 0, 0, 200);
constexpr ImU32 kScrollbarTrackColor = IM_COL32(255, 255, 255, 28);
constexpr ImU32 kScrollbarThumbColor = IM_COL32(255, 255, 255, 92);
constexpr ImU32 kScrollbarThumbHoveredColor = IM_COL32(255, 255, 255, 132);
constexpr ImU32 kScrollbarThumbActiveColor = IM_COL32(255, 255, 255, 188);

enum class MediaKind : std::uint8_t {
    StaticImage,
    AnimatedGif,
    Video,
};

enum class LoadState : std::uint8_t {
    Empty,
    Queued,
    Decoding,
    PendingUpload,
    Resident,
};

enum class DecodeMode : std::uint8_t {
    Gallery,
    Zoom,
};

const char* MediaKindBadgeText(MediaKind kind) {
    switch (kind) {
    case MediaKind::AnimatedGif:
        return "GIF";
    case MediaKind::Video:
        return "VIDEO";
    case MediaKind::StaticImage:
    default:
        return nullptr;
    }
}

struct ImageRecord {
    fs::path path;
    MediaKind kind = MediaKind::StaticImage;
    double sourceFrameRate = 0.0;
    int sourceWidth = 0;
    int sourceHeight = 0;

    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    GLuint texture = 0;
    int textureWidth = 0;
    int textureHeight = 0;
    std::size_t textureBytes = 0;
    bool textureHasMipmaps = false;
    std::uint64_t lastTouchedFrame = 0;
    std::size_t residentSlot = std::numeric_limits<std::size_t>::max();
    int galleryTextureMaxDimension = 0;
    bool galleryUpgradePending = false;
    int galleryUpgradeTargetDimension = 0;
    LoadState state = LoadState::Empty;
    bool removed = false;
    stbi_uc* cachedPixels = nullptr;
    std::size_t cachedPixelBytes = 0;
    int cachedWidth = 0;
    int cachedHeight = 0;
    std::uint64_t cachedLastTouchedFrame = 0;
    std::vector<stbi_uc> animationPixels;
    std::vector<int> animationFrameDelaysMs;
    int animationFrameCount = 0;
    int animationFrameIndex = 0;
    double animationFrameClockSeconds = 0.0;
};

struct ProbeResult {
    bool valid = false;
    MediaKind kind = MediaKind::StaticImage;
    double frameRate = 0.0;
    int width = 0;
    int height = 0;
};

struct DecodedImage {
    std::size_t index = std::numeric_limits<std::size_t>::max();
    MediaKind kind = MediaKind::StaticImage;
    DecodeMode mode = DecodeMode::Gallery;
    int width = 0;
    int height = 0;
    int frameCount = 1;
    int requestedMaxDimension = 0;
    bool replaceResidentTexture = false;
    stbi_uc* pixels = nullptr;
    std::vector<int> frameDelaysMs;

    DecodedImage() = default;
    DecodedImage(const DecodedImage&) = delete;
    DecodedImage& operator=(const DecodedImage&) = delete;

    DecodedImage(DecodedImage&& other) noexcept
        : index(other.index),
          kind(other.kind),
          mode(other.mode),
          width(other.width),
          height(other.height),
          frameCount(other.frameCount),
          requestedMaxDimension(other.requestedMaxDimension),
          replaceResidentTexture(other.replaceResidentTexture),
          pixels(other.pixels),
          frameDelaysMs(std::move(other.frameDelaysMs)) {
        other.index = std::numeric_limits<std::size_t>::max();
        other.kind = MediaKind::StaticImage;
        other.mode = DecodeMode::Gallery;
        other.width = 0;
        other.height = 0;
        other.frameCount = 1;
        other.requestedMaxDimension = 0;
        other.replaceResidentTexture = false;
        other.pixels = nullptr;
    }

    DecodedImage& operator=(DecodedImage&& other) noexcept {
        if (this != &other) {
            reset();
            index = other.index;
            kind = other.kind;
            mode = other.mode;
            width = other.width;
            height = other.height;
            frameCount = other.frameCount;
            requestedMaxDimension = other.requestedMaxDimension;
            replaceResidentTexture = other.replaceResidentTexture;
            pixels = other.pixels;
            frameDelaysMs = std::move(other.frameDelaysMs);
            other.index = std::numeric_limits<std::size_t>::max();
            other.kind = MediaKind::StaticImage;
            other.mode = DecodeMode::Gallery;
            other.width = 0;
            other.height = 0;
            other.frameCount = 1;
            other.requestedMaxDimension = 0;
            other.replaceResidentTexture = false;
            other.pixels = nullptr;
        }
        return *this;
    }

    ~DecodedImage() {
        reset();
    }

    void reset() {
        if (pixels != nullptr) {
            stbi_image_free(pixels);
            pixels = nullptr;
        }
        index = std::numeric_limits<std::size_t>::max();
        kind = MediaKind::StaticImage;
        mode = DecodeMode::Gallery;
        width = 0;
        height = 0;
        frameCount = 1;
        requestedMaxDimension = 0;
        replaceResidentTexture = false;
        frameDelaysMs.clear();
    }

    std::size_t pixelBytes() const {
        if (pixels == nullptr || width <= 0 || height <= 0) {
            return 0;
        }
        return static_cast<std::size_t>(width) *
            static_cast<std::size_t>(height) *
            4U *
            static_cast<std::size_t>(std::max(1, frameCount));
    }

    stbi_uc* releasePixels() {
        stbi_uc* released = pixels;
        pixels = nullptr;
        return released;
    }
};

struct PerfTraceSample {
    double totalMs = 0.0;
    double visibleMs = 0.0;
    double touchMs = 0.0;
    double collectDecodedMs = 0.0;
    double uploadMs = 0.0;
    double animationMs = 0.0;
    double evictMs = 0.0;
    double requestMs = 0.0;
};

struct DecodeJob {
    std::size_t index = 0;
    DecodeMode mode = DecodeMode::Gallery;
    int maxDimension = 0;
    std::uint64_t scrollRevision = 0;
    bool discardIfStale = false;
    bool replaceResidentTexture = false;
};

std::uintmax_t FileSizeBytes(const fs::path& path) {
    std::error_code ec;
    const std::uintmax_t size = fs::file_size(path, ec);
    return ec ? 0 : size;
}

bool FileExceedsSizeLimit(const fs::path& path, std::uintmax_t maxBytes, std::uintmax_t* sizeBytes = nullptr) {
    const std::uintmax_t fileBytes = FileSizeBytes(path);
    if (sizeBytes != nullptr) {
        *sizeBytes = fileBytes;
    }
    return fileBytes == 0 || fileBytes > maxBytes;
}

std::size_t ClampDecodeIoBudget(std::uintmax_t fileBytes) {
    const std::uintmax_t scaledBudget = fileBytes > 0
        ? std::min<std::uintmax_t>(kStbiDecodeIoBudgetBytesMax, fileBytes * 4ULL)
        : static_cast<std::uintmax_t>(kStbiDecodeIoBudgetBytesMin);
    return static_cast<std::size_t>(std::clamp<std::uintmax_t>(
        scaledBudget,
        static_cast<std::uintmax_t>(kStbiDecodeIoBudgetBytesMin),
        static_cast<std::uintmax_t>(kStbiDecodeIoBudgetBytesMax)));
}

bool ExceedsDecodedImageLimits(int width, int height, int frameCount = 1) {
    if (width <= 0 || height <= 0 || frameCount <= 0) {
        return true;
    }

    const std::uintmax_t framePixels =
        static_cast<std::uintmax_t>(width) *
        static_cast<std::uintmax_t>(height);
    if (framePixels == 0 || framePixels > kDecodedPixelCountMax) {
        return true;
    }

    const std::uintmax_t totalBytes =
        framePixels *
        4ULL *
        static_cast<std::uintmax_t>(frameCount);
    return totalBytes == 0 || totalBytes > kDecodedImageBytesMax;
}

struct StbiStreamContext {
    explicit StbiStreamContext(const fs::path& path, std::size_t ioBudgetBytes)
        : stream(path, std::ios::binary),
          ioBudgetBytes(ioBudgetBytes) {}

    bool allowIo(std::size_t byteCount) {
        if (budgetExceeded || ioBudgetBytes == 0) {
            return !budgetExceeded;
        }
        if (byteCount > ioBudgetBytes - std::min(ioBudgetBytes, ioBytesConsumed)) {
            budgetExceeded = true;
            return false;
        }
        ioBytesConsumed += byteCount;
        return true;
    }

    std::ifstream stream;
    std::size_t ioBudgetBytes = 0;
    std::size_t ioBytesConsumed = 0;
    bool budgetExceeded = false;
};

int StbiRead(void* user, char* data, int size) {
    auto* context = static_cast<StbiStreamContext*>(user);
    if (size <= 0 || !context->allowIo(static_cast<std::size_t>(size))) {
        return 0;
    }
    context->stream.read(data, static_cast<std::streamsize>(size));
    return static_cast<int>(context->stream.gcount());
}

void StbiSkip(void* user, int size) {
    auto* context = static_cast<StbiStreamContext*>(user);
    const std::size_t skipBytes = size >= 0
        ? static_cast<std::size_t>(size)
        : static_cast<std::size_t>(-static_cast<long long>(size));
    if (!context->allowIo(skipBytes)) {
        context->stream.clear();
        context->stream.seekg(0, std::ios::end);
        return;
    }
    context->stream.clear();
    context->stream.seekg(static_cast<std::streamoff>(size), std::ios_base::cur);
}

int StbiEof(void* user) {
    auto* context = static_cast<StbiStreamContext*>(user);
    return context->budgetExceeded || context->stream.eof() ? 1 : 0;
}

const stbi_io_callbacks kStbiCallbacks = {
    StbiRead,
    StbiSkip,
    StbiEof,
};

template <typename T>
class ThreadSafeQueue {
public:
    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push_back(std::move(item));
        }
        condition_.notify_one();
    }

    bool waitPop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return closed_ || !queue_.empty(); });
        if (queue_.empty()) {
            return false;
        }
        item = std::move(queue_.front());
        queue_.pop_front();
        return true;
    }

    bool tryPop(T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        item = std::move(queue_.front());
        queue_.pop_front();
        return true;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        condition_.notify_all();
    }

    std::vector<T> closeAndClear() {
        std::vector<T> removed;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
            removed.reserve(queue_.size());
            while (!queue_.empty()) {
                removed.push_back(std::move(queue_.front()));
                queue_.pop_front();
            }
        }
        condition_.notify_all();
        return removed;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<T> queue_;
    bool closed_ = false;
};

class DecodeRequestQueue {
public:
    void push(DecodeJob job, bool highPriority) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (closed_) {
                return;
            }
            if (highPriority) {
                queue_.push_front(job);
            } else {
                queue_.push_back(job);
            }
        }
        condition_.notify_one();
    }

    bool waitPop(DecodeJob& job) {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return closed_ || !queue_.empty(); });
        if (queue_.empty()) {
            return false;
        }
        job = queue_.front();
        queue_.pop_front();
        return true;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        condition_.notify_all();
    }

    std::vector<DecodeJob> closeAndClear() {
        std::vector<DecodeJob> removed;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
            removed.reserve(queue_.size());
            while (!queue_.empty()) {
                removed.push_back(queue_.front());
                queue_.pop_front();
            }
        }
        condition_.notify_all();
        return removed;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    std::vector<DecodeJob> trimToSize(std::size_t maxItems) {
        std::vector<DecodeJob> removed;
        std::lock_guard<std::mutex> lock(mutex_);
        while (queue_.size() > maxItems) {
            removed.push_back(queue_.back());
            queue_.pop_back();
        }
        return removed;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<DecodeJob> queue_;
    bool closed_ = false;
};

class VerticalBucketIndex {
public:
    void rebuild(const std::vector<ImageRecord>& items, float contentHeight, const std::vector<std::size_t>& orderedIndices) {
        const std::size_t bucketCount = std::max<std::size_t>(
            1,
            static_cast<std::size_t>(std::ceil(std::max(1.0f, contentHeight) / kBucketHeight)));

        buckets_.assign(bucketCount, {});
        stamps_.assign(items.size(), 0);

        for (const std::size_t index : orderedIndices) {
            const auto& item = items[index];
            const std::size_t firstBucket = clampBucket(item.y);
            const std::size_t lastBucket = clampBucket(item.y + item.h);
            for (std::size_t bucket = firstBucket; bucket <= lastBucket; ++bucket) {
                buckets_[bucket].push_back(index);
            }
        }
    }

    template <typename Fn>
    void query(float top, float bottom, Fn&& callback) {
        if (buckets_.empty() || stamps_.empty()) {
            return;
        }

        ++stamp_;
        if (stamp_ == 0) {
            std::fill(stamps_.begin(), stamps_.end(), 0U);
            stamp_ = 1;
        }

        const std::size_t firstBucket = clampBucket(std::max(0.0f, top));
        const std::size_t lastBucket = clampBucket(std::max(top, bottom));

        for (std::size_t bucket = firstBucket; bucket <= lastBucket; ++bucket) {
            for (const std::size_t index : buckets_[bucket]) {
                if (stamps_[index] == stamp_) {
                    continue;
                }
                stamps_[index] = stamp_;
                callback(index);
            }
        }
    }

private:
    std::size_t clampBucket(float position) const {
        if (buckets_.empty()) {
            return 0;
        }
        const float safePosition = std::max(0.0f, position);
        const auto bucket = static_cast<std::size_t>(safePosition / kBucketHeight);
        return std::min(bucket, buckets_.size() - 1);
    }

    std::vector<std::vector<std::size_t>> buckets_;
    std::vector<std::uint32_t> stamps_;
    std::uint32_t stamp_ = 0;
};

std::string LowercaseExtension(const fs::path& path) {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return extension;
}

std::string LowercaseAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool IsStaticImageExtension(const std::string& extension) {
    static const std::array<const char*, 9> kExtensions = {
        ".png", ".jpg", ".jpeg", ".bmp", ".tga",
        ".hdr", ".pic", ".ppm", ".pgm"
    };
    return std::find(kExtensions.begin(), kExtensions.end(), extension) != kExtensions.end();
}

bool IsAnimatedGifExtension(const std::string& extension) {
    return extension == ".gif";
}

bool IsVideoExtension(const std::string& extension) {
    static const std::array<const char*, 6> kExtensions = {
        ".mp4", ".mkv", ".webm", ".avi", ".mov", ".m4v"
    };
    return std::find(kExtensions.begin(), kExtensions.end(), extension) != kExtensions.end();
}

MediaKind DetectMediaKind(const fs::path& path) {
    const std::string extension = LowercaseExtension(path);
    if (IsAnimatedGifExtension(extension)) {
        return MediaKind::AnimatedGif;
    }
    if (IsVideoExtension(extension)) {
        return MediaKind::Video;
    }
    return MediaKind::StaticImage;
}

bool IsSupportedMediaFile(const fs::path& path) {
    const std::string extension = LowercaseExtension(path);
    return IsStaticImageExtension(extension) || IsAnimatedGifExtension(extension) || IsVideoExtension(extension);
}

std::vector<fs::path> CollectImagePaths(const fs::path& root) {
    std::vector<fs::path> paths;
    std::error_code ec;

    if (!fs::exists(root, ec)) {
        return paths;
    }

    if (fs::is_regular_file(root, ec)) {
        if (IsSupportedMediaFile(root)) {
            paths.push_back(root);
        }
        return paths;
    }

    const auto options = fs::directory_options::skip_permission_denied;
    for (fs::recursive_directory_iterator it(root, options, ec), end; it != end; it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }
        if (!it->is_regular_file(ec)) {
            ec.clear();
            continue;
        }
        if (IsSupportedMediaFile(it->path())) {
            paths.push_back(it->path());
        }
    }

    std::sort(paths.begin(), paths.end(), [](const fs::path& lhs, const fs::path& rhs) {
        return lhs.native() < rhs.native();
    });

    return paths;
}

bool DirectoryHintsMediaRoot(const fs::path& root) {
    std::error_code ec;
    if (root.empty() || !fs::exists(root, ec) || !fs::is_directory(root, ec)) {
        return false;
    }

    static const std::array<std::string, 4> kMediaDirectoryNames = {
        "images", "image", "picture", "video"
    };

    const auto options = fs::directory_options::skip_permission_denied;
    for (fs::directory_iterator it(root, options, ec), end; it != end; it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }

        if (it->is_regular_file(ec)) {
            if (!ec && IsSupportedMediaFile(it->path())) {
                return true;
            }
            ec.clear();
            continue;
        }
        ec.clear();

        if (!it->is_directory(ec)) {
            ec.clear();
            continue;
        }

        const std::string name = LowercaseAscii(it->path().filename().string());
        if (std::find(kMediaDirectoryNames.begin(), kMediaDirectoryNames.end(), name) != kMediaDirectoryNames.end()) {
            return true;
        }
    }

    return false;
}

bool ProbeImageInfo(const fs::path& path, int& width, int& height) {
    StbiStreamContext stream(path, kStbiProbeIoBudgetBytes);
    if (!stream.stream.is_open()) {
        return false;
    }

    int components = 0;
    return stbi_info_from_callbacks(&kStbiCallbacks, &stream, &width, &height, &components) != 0 &&
        !ExceedsDecodedImageLimits(width, height);
}

std::vector<stbi_uc> ReadBinaryFile(const fs::path& path, std::uintmax_t maxBytes) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream.is_open()) {
        return {};
    }

    const std::streamsize length = stream.tellg();
    if (length <= 0 || static_cast<std::uintmax_t>(length) > maxBytes) {
        return {};
    }

    stream.seekg(0, std::ios::beg);
    std::vector<stbi_uc> buffer(static_cast<std::size_t>(length));
    if (!stream.read(reinterpret_cast<char*>(buffer.data()), length)) {
        return {};
    }
    return buffer;
}

#if defined(_WIN32) && defined(CPPGALLERY_HAS_EMBEDDED_FFMPEG_TOOLS)
constexpr int kEmbeddedFfmpegResourceId = 41001;
constexpr int kEmbeddedFfprobeResourceId = 41002;
constexpr WORD kEmbeddedBinaryResourceTypeId = 10;
#endif

#if defined(_WIN32)
fs::path ExecutablePath() {
    std::array<wchar_t, 32768> buffer{};
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) {
        return {};
    }
    return fs::path(buffer.data());
}

fs::path ExecutableDirectory() {
    const fs::path executablePath = ExecutablePath();
    return executablePath.empty() ? fs::path{} : executablePath.parent_path();
}

#if defined(CPPGALLERY_HAS_EMBEDDED_FFMPEG_TOOLS)
int EmbeddedToolResourceId(const wchar_t* toolName) {
    if (toolName == nullptr) {
        return 0;
    }
    if (lstrcmpiW(toolName, L"ffmpeg.exe") == 0) {
        return kEmbeddedFfmpegResourceId;
    }
    if (lstrcmpiW(toolName, L"ffprobe.exe") == 0) {
        return kEmbeddedFfprobeResourceId;
    }
    return 0;
}

std::wstring EmbeddedToolCacheTag() {
    const fs::path executablePath = ExecutablePath();
    WIN32_FILE_ATTRIBUTE_DATA attributes{};
    if (executablePath.empty() ||
        !GetFileAttributesExW(executablePath.c_str(), GetFileExInfoStandard, &attributes)) {
        return L"default";
    }

    std::wostringstream stream;
    stream << std::hex
           << attributes.nFileSizeHigh
           << attributes.nFileSizeLow
           << L'-'
           << attributes.ftLastWriteTime.dwHighDateTime
           << attributes.ftLastWriteTime.dwLowDateTime;
    return stream.str();
}

std::optional<fs::path> ExtractEmbeddedTool(const wchar_t* toolName) {
    const int resourceId = EmbeddedToolResourceId(toolName);
    if (resourceId == 0) {
        return std::nullopt;
    }

    HMODULE module = GetModuleHandleW(nullptr);
    HRSRC resource = FindResourceW(
        module,
        MAKEINTRESOURCEW(static_cast<WORD>(resourceId)),
        MAKEINTRESOURCEW(kEmbeddedBinaryResourceTypeId));
    if (resource == nullptr) {
        return std::nullopt;
    }

    HGLOBAL loadedResource = LoadResource(module, resource);
    if (loadedResource == nullptr) {
        return std::nullopt;
    }

    const DWORD resourceSize = SizeofResource(module, resource);
    if (resourceSize == 0) {
        return std::nullopt;
    }

    const void* resourceBytes = LockResource(loadedResource);
    if (resourceBytes == nullptr) {
        return std::nullopt;
    }

    std::array<wchar_t, 32768> tempBuffer{};
    const DWORD tempLength = GetTempPathW(static_cast<DWORD>(tempBuffer.size()), tempBuffer.data());
    if (tempLength == 0 || tempLength >= tempBuffer.size()) {
        return std::nullopt;
    }

    std::error_code ec;
    const fs::path outputDir = fs::path(tempBuffer.data()) / L"CppGalleryEmbeddedTools" / EmbeddedToolCacheTag();
    fs::create_directories(outputDir, ec);
    if (ec) {
        return std::nullopt;
    }

    const fs::path outputPath = outputDir / toolName;
    const bool fileAlreadyMatches =
        fs::exists(outputPath, ec) &&
        !ec &&
        fs::is_regular_file(outputPath, ec) &&
        !ec &&
        fs::file_size(outputPath, ec) == static_cast<std::uintmax_t>(resourceSize);
    if (fileAlreadyMatches && !ec) {
        return outputPath;
    }

    std::wostringstream tempNameStream;
    tempNameStream << toolName << L'.' << GetCurrentProcessId() << L'.' << GetCurrentThreadId() << L".tmp";
    const fs::path tempPath = outputDir / tempNameStream.str();

    {
        std::ofstream stream(tempPath, std::ios::binary | std::ios::trunc);
        if (!stream.is_open()) {
            return std::nullopt;
        }
        stream.write(static_cast<const char*>(resourceBytes), static_cast<std::streamsize>(resourceSize));
        if (!stream.good()) {
            stream.close();
            fs::remove(tempPath, ec);
            return std::nullopt;
        }
    }

    fs::rename(tempPath, outputPath, ec);
    if (ec) {
        ec.clear();
        if (fs::exists(outputPath, ec) &&
            !ec &&
            fs::file_size(outputPath, ec) == static_cast<std::uintmax_t>(resourceSize)) {
            fs::remove(tempPath, ec);
            return outputPath;
        }

        ec.clear();
        fs::remove(outputPath, ec);
        ec.clear();
        fs::rename(tempPath, outputPath, ec);
        if (ec) {
            fs::remove(tempPath, ec);
            return std::nullopt;
        }
    }

    return outputPath;
}
#endif
#else
fs::path ExecutablePath() {
    return {};
}

fs::path ExecutableDirectory() {
    return {};
}
#endif

std::vector<fs::path> FfmpegToolCandidates(const wchar_t* toolName) {
    std::vector<fs::path> candidates;
    auto appendIfExists = [&](const fs::path& candidate) {
        if (candidate.empty()) {
            return;
        }
        std::error_code ec;
        if (fs::exists(candidate, ec) && !ec) {
            if (std::find(candidates.begin(), candidates.end(), candidate) == candidates.end()) {
                candidates.push_back(candidate);
            }
        }
    };

    std::error_code ec;
    const fs::path currentDir = fs::current_path(ec);
    const fs::path exeDir = ExecutableDirectory();
    std::vector<fs::path> roots;
    if (!currentDir.empty()) {
        roots.push_back(currentDir);
        roots.push_back(currentDir / "build" / "Release");
        roots.push_back(currentDir / "third_party" / "ffmpeg");
        roots.push_back(currentDir / "third_party" / "ffmpeg" / "bin");
    }
    if (!exeDir.empty()) {
        roots.push_back(exeDir);
        roots.push_back(exeDir.parent_path());
        roots.push_back(exeDir.parent_path() / "build" / "Release");
        roots.push_back(exeDir.parent_path().parent_path());
        roots.push_back(exeDir.parent_path().parent_path() / "build" / "Release");
        roots.push_back(exeDir.parent_path().parent_path() / "third_party" / "ffmpeg");
        roots.push_back(exeDir.parent_path().parent_path() / "third_party" / "ffmpeg" / "bin");
    }

#if defined(_WIN32)
    wchar_t envBuffer[32768];
    const DWORD envLength = GetEnvironmentVariableW(L"FFMPEG_ROOT", envBuffer, static_cast<DWORD>(std::size(envBuffer)));
    if (envLength > 0 && envLength < std::size(envBuffer)) {
        const fs::path envRoot(envBuffer);
        roots.push_back(envRoot);
        roots.push_back(envRoot / "bin");
    }
#endif

    for (const fs::path& root : roots) {
        appendIfExists(root / toolName);
    }
    return candidates;
}

std::optional<fs::path> ResolveFfmpegToolPath(const wchar_t* toolName) {
    const std::vector<fs::path> candidates = FfmpegToolCandidates(toolName);
    if (!candidates.empty()) {
        return candidates.front();
    }
#if defined(_WIN32) && defined(CPPGALLERY_HAS_EMBEDDED_FFMPEG_TOOLS)
    return ExtractEmbeddedTool(toolName);
#else
    return std::nullopt;
#endif
}

bool HasFfmpegRuntime() {
    static const bool hasRuntime =
        ResolveFfmpegToolPath(L"ffmpeg.exe").has_value() &&
        ResolveFfmpegToolPath(L"ffprobe.exe").has_value();
    return hasRuntime;
}

const char* VideoRuntimeStatusText() {
    return HasFfmpegRuntime()
        ? "Video runtime ready"
        : "Video runtime missing: put ffmpeg.exe + ffprobe.exe beside the exe or under third_party/ffmpeg/bin";
}

#if defined(_WIN32)
std::vector<unsigned char> RunCommandCaptureBinary(
    const std::wstring& commandLine,
    DWORD timeoutMs,
    int* exitCode = nullptr) {
    std::vector<unsigned char> output;
    SECURITY_ATTRIBUTES securityAttributes{};
    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.bInheritHandle = TRUE;

    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &securityAttributes, 0)) {
        if (exitCode != nullptr) {
            *exitCode = -1;
        }
        return output;
    }

    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

    HANDLE nullHandle = CreateFileW(
        L"NUL",
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        &securityAttributes,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESTDHANDLES;
    startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startupInfo.hStdOutput = writePipe;
    startupInfo.hStdError = nullHandle != INVALID_HANDLE_VALUE ? nullHandle : writePipe;

    PROCESS_INFORMATION processInfo{};
    std::wstring mutableCommandLine = commandLine;
    mutableCommandLine.push_back(L'\0');
    const BOOL created = CreateProcessW(
        nullptr,
        mutableCommandLine.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo);

    CloseHandle(writePipe);
    if (nullHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(nullHandle);
    }

    if (!created) {
        CloseHandle(readPipe);
        if (exitCode != nullptr) {
            *exitCode = -1;
        }
        return output;
    }

    std::array<unsigned char, 65536> chunk{};
    const ULONGLONG startTicks = GetTickCount64();
    bool timedOut = false;

    for (;;) {
        DWORD bytesAvailable = 0;
        if (!PeekNamedPipe(readPipe, nullptr, 0, nullptr, &bytesAvailable, nullptr)) {
            break;
        }

        if (bytesAvailable > 0) {
            DWORD bytesRead = 0;
            const DWORD readSize = std::min<DWORD>(bytesAvailable, static_cast<DWORD>(chunk.size()));
            if (!ReadFile(readPipe, chunk.data(), readSize, &bytesRead, nullptr) || bytesRead == 0) {
                break;
            }
            output.insert(output.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(bytesRead));
            continue;
        }

        const DWORD waitResult = WaitForSingleObject(processInfo.hProcess, 20);
        if (waitResult == WAIT_OBJECT_0) {
            break;
        }
        if (waitResult == WAIT_FAILED) {
            break;
        }
        if (timeoutMs > 0 && GetTickCount64() - startTicks >= timeoutMs) {
            timedOut = true;
            TerminateProcess(processInfo.hProcess, static_cast<UINT>(-2));
            WaitForSingleObject(processInfo.hProcess, 2000);
            break;
        }
    }

    for (;;) {
        DWORD bytesRead = 0;
        if (!ReadFile(readPipe, chunk.data(), static_cast<DWORD>(chunk.size()), &bytesRead, nullptr) || bytesRead == 0) {
            break;
        }
        output.insert(output.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(bytesRead));
    }

    DWORD status = 0;
    if (!GetExitCodeProcess(processInfo.hProcess, &status)) {
        status = static_cast<DWORD>(-1);
    } else if (timedOut) {
        status = static_cast<DWORD>(-2);
    }
    CloseHandle(readPipe);
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);

    if (exitCode != nullptr) {
        *exitCode = static_cast<int>(status);
    }
    return output;
}
#else
std::vector<unsigned char> RunCommandCaptureBinary(const std::wstring&, unsigned long, int* exitCode = nullptr) {
    if (exitCode != nullptr) {
        *exitCode = -1;
    }
    return {};
}
#endif

std::wstring QuoteCommandArgument(const fs::path& path) {
    return L"\"" + path.wstring() + L"\"";
}

double ParseFrameRateText(const std::string& text) {
    const std::size_t slash = text.find('/');
    if (slash == std::string::npos) {
        try {
            return std::stod(text);
        } catch (...) {
            return 0.0;
        }
    }

    try {
        const double numerator = std::stod(text.substr(0, slash));
        const double denominator = std::stod(text.substr(slash + 1));
        return denominator != 0.0 ? numerator / denominator : 0.0;
    } catch (...) {
        return 0.0;
    }
}

double ClampVideoPlaybackFps(double frameRate) {
    if (frameRate <= 0.0 || !std::isfinite(frameRate)) {
        frameRate = 12.0;
    }
    return std::clamp(frameRate, kVideoMinPlaybackFps, kVideoMaxPlaybackFps);
}

bool ProbeVideoInfo(const fs::path& path, int& width, int& height, double& frameRate) {
    width = 0;
    height = 0;
    frameRate = 0.0;

    const auto ffprobePath = ResolveFfmpegToolPath(L"ffprobe.exe");
    if (!ffprobePath.has_value()) {
        return false;
    }

    std::wostringstream command;
    command << QuoteCommandArgument(*ffprobePath)
            << L" -v error -select_streams v:0 -show_entries stream=width,height,avg_frame_rate"
            << L" -of default=noprint_wrappers=1:nokey=0 "
            << QuoteCommandArgument(path);

    int exitCode = 0;
    const std::vector<unsigned char> output = RunCommandCaptureBinary(command.str(), kVideoProbeTimeoutMs, &exitCode);
    if (exitCode != 0 || output.empty()) {
        return false;
    }

    std::string text(output.begin(), output.end());
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.rfind("width=", 0) == 0) {
            width = std::max(0, std::atoi(line.c_str() + 6));
        } else if (line.rfind("height=", 0) == 0) {
            height = std::max(0, std::atoi(line.c_str() + 7));
        } else if (line.rfind("avg_frame_rate=", 0) == 0) {
            frameRate = ParseFrameRateText(line.substr(15));
        }
    }

    return width > 0 && height > 0;
}

bool ProbeMediaInfo(const fs::path& path, MediaKind kind, int& width, int& height, double& frameRate) {
    frameRate = 0.0;
    if (kind == MediaKind::Video) {
        return ProbeVideoInfo(path, width, height, frameRate);
    }
    const std::uintmax_t maxProbeBytes = kind == MediaKind::AnimatedGif
        ? kGifFileBytesMax
        : kStaticImageFileBytesMax;
    if (FileExceedsSizeLimit(path, maxProbeBytes)) {
        return false;
    }
    return ProbeImageInfo(path, width, height);
}

DecodedImage DecodeImageFile(std::size_t index, const fs::path& path) {
    DecodedImage decoded;
    decoded.index = index;

    std::uintmax_t fileBytes = 0;
    if (FileExceedsSizeLimit(path, kStaticImageFileBytesMax, &fileBytes)) {
        return decoded;
    }

    StbiStreamContext stream(path, ClampDecodeIoBudget(fileBytes));
    if (!stream.stream.is_open()) {
        return decoded;
    }

    int components = 0;
    decoded.pixels = stbi_load_from_callbacks(
        &kStbiCallbacks,
        &stream,
        &decoded.width,
        &decoded.height,
        &components,
        4);

    if (decoded.pixels == nullptr || ExceedsDecodedImageLimits(decoded.width, decoded.height)) {
        decoded.reset();
    }

    return decoded;
}

DecodedImage DecodeVideoFile(std::size_t index, const fs::path& path, int sourceWidth, int sourceHeight, double sourceFrameRate) {
    DecodedImage decoded;
    decoded.index = index;
    decoded.kind = MediaKind::Video;

    const auto ffmpegPath = ResolveFfmpegToolPath(L"ffmpeg.exe");
    if (!ffmpegPath.has_value() ||
        sourceWidth <= 0 ||
        sourceHeight <= 0 ||
        ExceedsDecodedImageLimits(sourceWidth, sourceHeight)) {
        return decoded;
    }

    const float scale = std::min(
        1.0f,
        std::min(
            static_cast<float>(kVideoMaxDecodeDimension) / static_cast<float>(sourceWidth),
            static_cast<float>(kVideoMaxDecodeDimension) / static_cast<float>(sourceHeight)));
    const int targetWidth = std::max(2, static_cast<int>(std::floor((static_cast<float>(sourceWidth) * scale) / 2.0f)) * 2);
    const int targetHeight = std::max(2, static_cast<int>(std::floor((static_cast<float>(sourceHeight) * scale) / 2.0f)) * 2);
    const double playbackFps = ClampVideoPlaybackFps(sourceFrameRate);
    const int previewFrames = std::clamp(
        static_cast<int>(std::round(playbackFps * kVideoPreviewDurationSeconds)),
        12,
        kVideoPreviewMaxFrames);

    std::wostringstream command;
    command << QuoteCommandArgument(*ffmpegPath)
            << L" -v error -i "
            << QuoteCommandArgument(path)
            << L" -an -sn -vf \"fps="
            << playbackFps
            << L",scale="
            << targetWidth
            << L":"
            << targetHeight
            << L":flags=lanczos\" -frames:v "
            << previewFrames
            << L" -f rawvideo -pix_fmt rgba -";

    int exitCode = 0;
    const std::vector<unsigned char> rawFrames = RunCommandCaptureBinary(command.str(), kVideoDecodeTimeoutMs, &exitCode);
    const std::size_t frameStride =
        static_cast<std::size_t>(targetWidth) *
        static_cast<std::size_t>(targetHeight) *
        4U;
    if (exitCode != 0 || frameStride == 0 || rawFrames.size() < frameStride) {
        return decoded;
    }

    const int frameCount = static_cast<int>(rawFrames.size() / frameStride);
    if (frameCount <= 0 || ExceedsDecodedImageLimits(targetWidth, targetHeight, frameCount)) {
        return decoded;
    }

    decoded.pixels = static_cast<stbi_uc*>(std::malloc(frameStride * static_cast<std::size_t>(frameCount)));
    if (decoded.pixels == nullptr) {
        return decoded;
    }

    std::memcpy(decoded.pixels, rawFrames.data(), frameStride * static_cast<std::size_t>(frameCount));
    decoded.width = targetWidth;
    decoded.height = targetHeight;
    decoded.frameCount = frameCount;
    decoded.frameDelaysMs.assign(static_cast<std::size_t>(frameCount), static_cast<int>(std::round(1000.0 / playbackFps)));
    return decoded;
}

DecodedImage DecodeGalleryFile(
    std::size_t index,
    const fs::path& path,
    MediaKind kind,
    int sourceWidth,
    int sourceHeight,
    double sourceFrameRate) {
    if (kind == MediaKind::Video) {
        return DecodeVideoFile(index, path, sourceWidth, sourceHeight, sourceFrameRate);
    }
    if (kind != MediaKind::AnimatedGif) {
        return DecodeImageFile(index, path);
    }

    DecodedImage decoded;
    decoded.index = index;
    decoded.kind = MediaKind::AnimatedGif;

    if (sourceWidth <= 0 || sourceHeight <= 0 || ExceedsDecodedImageLimits(sourceWidth, sourceHeight)) {
        return decoded;
    }

    std::vector<stbi_uc> fileBytes = ReadBinaryFile(path, kGifFileBytesMax);
    if (fileBytes.empty()) {
        return decoded;
    }

    int components = 0;
    int frameCount = 0;
    int* frameDelays = nullptr;
    decoded.pixels = stbi_load_gif_from_memory(
        fileBytes.data(),
        static_cast<int>(fileBytes.size()),
        &frameDelays,
        &decoded.width,
        &decoded.height,
        &frameCount,
        &components,
        4);

    if (decoded.pixels == nullptr || decoded.width <= 0 || decoded.height <= 0) {
        if (frameDelays != nullptr) {
            STBI_FREE(frameDelays);
        }
        decoded.reset();
        return decoded;
    }

    decoded.frameCount = std::max(1, frameCount);
    if (frameDelays != nullptr && decoded.frameCount > 1) {
        decoded.frameDelaysMs.assign(frameDelays, frameDelays + decoded.frameCount);
    }
    if (frameDelays != nullptr) {
        STBI_FREE(frameDelays);
    }
    if (ExceedsDecodedImageLimits(decoded.width, decoded.height, std::max(1, decoded.frameCount))) {
        decoded.reset();
        return decoded;
    }
    if (decoded.frameCount <= 1) {
        decoded.kind = MediaKind::StaticImage;
    }
    return decoded;
}

bool DownscaleToFit(DecodedImage& decoded, int maxTextureSize) {
    if (decoded.pixels == nullptr || decoded.width <= maxTextureSize && decoded.height <= maxTextureSize) {
        return decoded.pixels != nullptr;
    }

    const float scale = std::min(
        static_cast<float>(maxTextureSize) / static_cast<float>(decoded.width),
        static_cast<float>(maxTextureSize) / static_cast<float>(decoded.height));

    const int newWidth = std::max(1, static_cast<int>(std::floor(decoded.width * scale)));
    const int newHeight = std::max(1, static_cast<int>(std::floor(decoded.height * scale)));
    const std::size_t framePixelCount = static_cast<std::size_t>(newWidth) * static_cast<std::size_t>(newHeight) * 4U;
    const std::size_t pixelCount = framePixelCount * static_cast<std::size_t>(std::max(1, decoded.frameCount));

    auto* resized = static_cast<stbi_uc*>(std::malloc(pixelCount));
    if (resized == nullptr) {
        return false;
    }

    const std::size_t sourceFrameStride =
        static_cast<std::size_t>(decoded.width) * static_cast<std::size_t>(decoded.height) * 4U;
    for (int frameIndex = 0; frameIndex < std::max(1, decoded.frameCount); ++frameIndex) {
        const stbi_uc* sourceFrame = decoded.pixels + sourceFrameStride * static_cast<std::size_t>(frameIndex);
        stbi_uc* targetFrame = resized + framePixelCount * static_cast<std::size_t>(frameIndex);
        for (int y = 0; y < newHeight; ++y) {
            const int sourceY = std::min(decoded.height - 1, static_cast<int>((static_cast<long long>(y) * decoded.height) / newHeight));
            for (int x = 0; x < newWidth; ++x) {
                const int sourceX = std::min(decoded.width - 1, static_cast<int>((static_cast<long long>(x) * decoded.width) / newWidth));
                const std::size_t sourceOffset =
                    (static_cast<std::size_t>(sourceY) * static_cast<std::size_t>(decoded.width) + static_cast<std::size_t>(sourceX)) * 4U;
                const std::size_t targetOffset =
                    (static_cast<std::size_t>(y) * static_cast<std::size_t>(newWidth) + static_cast<std::size_t>(x)) * 4U;
                targetFrame[targetOffset + 0] = sourceFrame[sourceOffset + 0];
                targetFrame[targetOffset + 1] = sourceFrame[sourceOffset + 1];
                targetFrame[targetOffset + 2] = sourceFrame[sourceOffset + 2];
                targetFrame[targetOffset + 3] = sourceFrame[sourceOffset + 3];
            }
        }
    }

    stbi_image_free(decoded.pixels);
    decoded.pixels = resized;
    decoded.width = newWidth;
    decoded.height = newHeight;
    return true;
}

bool ShouldUseMipmapsForDecodedImage(const DecodedImage& decoded) {
    return decoded.kind == MediaKind::StaticImage &&
        decoded.frameCount <= 1 &&
        decoded.mode == DecodeMode::Gallery;
}

GLuint CreateTexture(int width, int height, const stbi_uc* pixels, bool useMipmaps) {
    if (pixels == nullptr || width <= 0 || height <= 0) {
        return 0;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    if (texture == 0) {
        return 0;
    }

    const bool enableMipmaps = useMipmaps && g_glGenerateMipmap != nullptr;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, enableMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels);
    if (enableMipmaps) {
        g_glGenerateMipmap(GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

GLuint CreateTexture(const DecodedImage& decoded) {
    return CreateTexture(decoded.width, decoded.height, decoded.pixels, ShouldUseMipmapsForDecodedImage(decoded));
}

std::size_t TextureByteSize(int width, int height, bool includeMipmaps = false) {
    if (width <= 0 || height <= 0) {
        return 0;
    }
    const std::size_t baseBytes =
        static_cast<std::size_t>(width) *
        static_cast<std::size_t>(height) *
        4U;
    return includeMipmaps ? baseBytes + (baseBytes / 3U) : baseBytes;
}

int MaxDimension(int width, int height) {
    return std::max(width, height);
}

ImU32 PlaceholderTint(std::size_t index, int alpha) {
    const std::uint32_t hash = static_cast<std::uint32_t>((index + 1) * 2654435761u);
    const int red = 50 + static_cast<int>((hash >> 16) & 0x3F);
    const int green = 70 + static_cast<int>((hash >> 8) & 0x4F);
    const int blue = 95 + static_cast<int>(hash & 0x5F);
    return IM_COL32(red, green, blue, alpha);
}

bool PointInRect(const ImVec2& point, const ImVec2& min, const ImVec2& max) {
    return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
}

#if defined(_WIN32)
bool CopyImageFileToClipboard(const fs::path& path) {
    const MediaKind kind = DetectMediaKind(path);
    int sourceWidth = 0;
    int sourceHeight = 0;
    double sourceFrameRate = 0.0;
    ProbeMediaInfo(path, kind, sourceWidth, sourceHeight, sourceFrameRate);
    DecodedImage decoded = DecodeGalleryFile(
        std::numeric_limits<std::size_t>::max(),
        path,
        kind,
        sourceWidth,
        sourceHeight,
        sourceFrameRate);
    if (decoded.pixels == nullptr || decoded.width <= 0 || decoded.height <= 0) {
        return false;
    }

    const std::size_t pixelBytes =
        static_cast<std::size_t>(decoded.width) * static_cast<std::size_t>(decoded.height) * 4U;
    if (pixelBytes > static_cast<std::size_t>(std::numeric_limits<DWORD>::max())) {
        return false;
    }

    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = decoded.width;
    bitmapInfo.bmiHeader.biHeight = decoded.height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    bitmapInfo.bmiHeader.biSizeImage = static_cast<DWORD>(pixelBytes);

    const auto copyPixelsBgraBottomUp = [&](unsigned char* destinationPixels) {
        const std::size_t rowBytes = static_cast<std::size_t>(decoded.width) * 4U;
        for (int y = 0; y < decoded.height; ++y) {
            const auto* sourceRow = decoded.pixels + static_cast<std::size_t>(y) * rowBytes;
            auto* destinationRow = destinationPixels + static_cast<std::size_t>(decoded.height - 1 - y) * rowBytes;
            for (std::size_t x = 0; x < rowBytes; x += 4U) {
                destinationRow[x + 0] = sourceRow[x + 2];
                destinationRow[x + 1] = sourceRow[x + 1];
                destinationRow[x + 2] = sourceRow[x + 0];
                destinationRow[x + 3] = sourceRow[x + 3];
            }
        }
    };

    const std::size_t dibBytes = sizeof(BITMAPINFOHEADER) + pixelBytes;
    HGLOBAL dibHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, dibBytes);
    if (dibHandle == nullptr) {
        return false;
    }

    void* dibMemory = GlobalLock(dibHandle);
    if (dibMemory == nullptr) {
        GlobalFree(dibHandle);
        return false;
    }

    auto* header = static_cast<BITMAPINFOHEADER*>(dibMemory);
    *header = bitmapInfo.bmiHeader;
    auto* targetPixels = static_cast<unsigned char*>(dibMemory) + sizeof(BITMAPINFOHEADER);
    copyPixelsBgraBottomUp(targetPixels);
    GlobalUnlock(dibHandle);

    void* bitmapPixels = nullptr;
    HBITMAP bitmapHandle = CreateDIBSection(nullptr, &bitmapInfo, DIB_RGB_COLORS, &bitmapPixels, nullptr, 0);
    if (bitmapHandle != nullptr && bitmapPixels != nullptr) {
        copyPixelsBgraBottomUp(static_cast<unsigned char*>(bitmapPixels));
    }

    if (!OpenClipboard(nullptr)) {
        if (bitmapHandle != nullptr) {
            DeleteObject(bitmapHandle);
        }
        GlobalFree(dibHandle);
        return false;
    }

    bool success = false;
    if (EmptyClipboard()) {
        bool anyFormatSet = false;
        if (bitmapHandle != nullptr && SetClipboardData(CF_BITMAP, bitmapHandle) != nullptr) {
            bitmapHandle = nullptr;
            anyFormatSet = true;
        }
        if (SetClipboardData(CF_DIB, dibHandle) != nullptr) {
            dibHandle = nullptr;
            anyFormatSet = true;
        }
        success = anyFormatSet;
    }

    CloseClipboard();

    if (bitmapHandle != nullptr) {
        DeleteObject(bitmapHandle);
    }
    if (dibHandle != nullptr) {
        GlobalFree(dibHandle);
    }
    return success;
}
#else
bool CopyImageFileToClipboard(const fs::path&) {
    return false;
}
#endif

bool DeleteImageFile(const fs::path& path) {
    std::error_code error;
    return fs::remove(path, error);
}

std::size_t RecommendedWorkerCount() {
    const unsigned int hardware = std::thread::hardware_concurrency();
    if (hardware <= 2) {
        return 2;
    }
    return std::min<std::size_t>(4, static_cast<std::size_t>(hardware - 1));
}

#if defined(_WIN32)
DWORD_PTR LowestSetBit(DWORD_PTR mask) {
    return mask & (~mask + 1);
}

int FirstProcessorIndexFromMask(DWORD_PTR mask) {
    for (int bit = 0; bit < static_cast<int>(sizeof(DWORD_PTR) * 8); ++bit) {
        if ((mask & (static_cast<DWORD_PTR>(1) << bit)) != 0) {
            return bit;
        }
    }
    return -1;
}

std::vector<DWORD> ProcessorIndicesFromMask(DWORD_PTR mask) {
    std::vector<DWORD> indices;
    indices.reserve(sizeof(DWORD_PTR) * 8);
    for (DWORD bit = 0; bit < static_cast<DWORD>(sizeof(DWORD_PTR) * 8); ++bit) {
        if ((mask & (static_cast<DWORD_PTR>(1) << bit)) != 0) {
            indices.push_back(bit);
        }
    }
    return indices;
}
#endif

std::vector<ImageRecord> BuildImageDatabase(const std::vector<fs::path>& paths) {
    if (paths.empty()) {
        return {};
    }

    std::vector<MediaKind> mediaKinds(paths.size(), MediaKind::StaticImage);
    const std::size_t threadCount = std::max<std::size_t>(1, std::min<std::size_t>(RecommendedWorkerCount(), paths.size()));
    std::vector<ProbeResult> probeResults(paths.size());
    std::atomic<std::size_t> nextIndex{0};
    std::vector<std::thread> workers;
    workers.reserve(threadCount);

    for (std::size_t threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
        workers.emplace_back([&] {
            for (;;) {
                const std::size_t index = nextIndex.fetch_add(1, std::memory_order_relaxed);
                if (index >= paths.size()) {
                    return;
                }

                const MediaKind kind = DetectMediaKind(paths[index]);
                mediaKinds[index] = kind;
                int width = 0;
                int height = 0;
                double frameRate = 0.0;
                if (ProbeMediaInfo(paths[index], kind, width, height, frameRate) && width > 0 && height > 0) {
                    probeResults[index] = ProbeResult{true, kind, frameRate, width, height};
                }
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    for (std::size_t index = 0; index < paths.size(); ++index) {
        if (probeResults[index].valid || mediaKinds[index] != MediaKind::Video) {
            continue;
        }

        int width = 0;
        int height = 0;
        double frameRate = 0.0;
        if (ProbeVideoInfo(paths[index], width, height, frameRate) && width > 0 && height > 0) {
            probeResults[index] = ProbeResult{true, MediaKind::Video, frameRate, width, height};
        }
    }

    std::vector<ImageRecord> records;
    records.reserve(paths.size());

    for (std::size_t index = 0; index < paths.size(); ++index) {
        const auto& probe = probeResults[index];
        if (!probe.valid) {
            continue;
        }

        ImageRecord record;
        record.path = paths[index];
        record.kind = probe.kind;
        record.sourceFrameRate = probe.frameRate;
        record.sourceWidth = probe.width;
        record.sourceHeight = probe.height;
        records.push_back(std::move(record));
    }

    return records;
}

std::vector<ImageRecord> LoadGalleryRecords(const fs::path& rootDirectory) {
    return BuildImageDatabase(CollectImagePaths(rootDirectory));
}

std::string MakeWindowTitle(const fs::path& rootDirectory) {
    const std::string rootText = rootDirectory.filename().empty()
        ? rootDirectory.u8string()
        : rootDirectory.filename().u8string();
    return rootText.empty() ? "CppGallery" : "CppGallery - " + rootText;
}

bool HasCommandFlag(int argc, char** argv, const char* flag) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] != nullptr && std::strcmp(argv[i], flag) == 0) {
            return true;
        }
    }
    return false;
}

void PrintMediaDiagnostics(const fs::path& rootDirectory) {
    const std::vector<fs::path> paths = CollectImagePaths(rootDirectory);
    std::size_t staticCandidates = 0;
    std::size_t gifCandidates = 0;
    std::size_t videoCandidates = 0;
    std::size_t validStatic = 0;
    std::size_t validGif = 0;
    std::size_t validVideo = 0;

    std::cout << "Root: " << rootDirectory.u8string() << '\n';
    std::cout << "FFmpeg runtime: " << VideoRuntimeStatusText() << '\n';
    if (const auto ffmpegPath = ResolveFfmpegToolPath(L"ffmpeg.exe")) {
        std::cout << "ffmpeg: " << ffmpegPath->u8string() << '\n';
    }
    if (const auto ffprobePath = ResolveFfmpegToolPath(L"ffprobe.exe")) {
        std::cout << "ffprobe: " << ffprobePath->u8string() << '\n';
    }

    for (const fs::path& path : paths) {
        const MediaKind kind = DetectMediaKind(path);
        int width = 0;
        int height = 0;
        double frameRate = 0.0;

        switch (kind) {
        case MediaKind::AnimatedGif:
            ++gifCandidates;
            break;
        case MediaKind::Video:
            ++videoCandidates;
            break;
        case MediaKind::StaticImage:
        default:
            ++staticCandidates;
            break;
        }

        if (!ProbeMediaInfo(path, kind, width, height, frameRate) || width <= 0 || height <= 0) {
            if (kind == MediaKind::Video) {
                std::cout << "Video probe failed: " << path.u8string() << '\n';
            }
            continue;
        }

        switch (kind) {
        case MediaKind::AnimatedGif:
            ++validGif;
            break;
        case MediaKind::Video:
            ++validVideo;
            break;
        case MediaKind::StaticImage:
        default:
            ++validStatic;
            break;
        }
    }

    std::cout << "Supported paths: " << paths.size() << '\n';
    std::cout << "Candidates static=" << staticCandidates
              << " gif=" << gifCandidates
              << " video=" << videoCandidates << '\n';
    std::cout << "Valid static=" << validStatic
              << " gif=" << validGif
              << " video=" << validVideo << '\n';
}

#if defined(_WIN32)
std::optional<fs::path> PickFolderDialog(const fs::path& initialFolder) {
    IFileOpenDialog* dialog = nullptr;
    const HRESULT createResult = CoCreateInstance(
        CLSID_FileOpenDialog,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&dialog));
    if (FAILED(createResult) || dialog == nullptr) {
        return std::nullopt;
    }

    DWORD options = 0;
    if (SUCCEEDED(dialog->GetOptions(&options))) {
        dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    }

    if (!initialFolder.empty()) {
        IShellItem* initialItem = nullptr;
        const std::wstring initialFolderWide = initialFolder.wstring();
        if (SUCCEEDED(SHCreateItemFromParsingName(initialFolderWide.c_str(), nullptr, IID_PPV_ARGS(&initialItem))) &&
            initialItem != nullptr) {
            dialog->SetFolder(initialItem);
            initialItem->Release();
        }
    }

    const HRESULT showResult = dialog->Show(nullptr);
    if (showResult == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        dialog->Release();
        return std::nullopt;
    }
    if (FAILED(showResult)) {
        dialog->Release();
        return std::nullopt;
    }

    IShellItem* resultItem = nullptr;
    if (FAILED(dialog->GetResult(&resultItem)) || resultItem == nullptr) {
        dialog->Release();
        return std::nullopt;
    }

    PWSTR selectedPathWide = nullptr;
    if (FAILED(resultItem->GetDisplayName(SIGDN_FILESYSPATH, &selectedPathWide)) || selectedPathWide == nullptr) {
        resultItem->Release();
        dialog->Release();
        return std::nullopt;
    }

    const fs::path selectedPath(selectedPathWide);
    CoTaskMemFree(selectedPathWide);
    resultItem->Release();
    dialog->Release();
    return selectedPath;
}
#else
std::optional<fs::path> PickFolderDialog(const fs::path&) {
    return std::nullopt;
}
#endif

class GalleryApp {
    struct ScrollbarGeometry {
        bool visible = false;
        ImVec2 trackMin;
        ImVec2 trackMax;
        ImVec2 thumbMin;
        ImVec2 thumbMax;
        float thumbHeight = 0.0f;
        float thumbTravel = 0.0f;
    };

    struct TextureUploadPbo {
        GLuint buffer = 0;
        std::size_t capacityBytes = 0;
    };

public:
    GalleryApp(std::vector<ImageRecord> records, std::size_t workerCount, int maxTextureSize, bool perfTraceEnabled = false)
        : items_(std::move(records)),
          workerCount_(std::max<std::size_t>(1, workerCount)),
          maxTextureSize_(std::max(1024, maxTextureSize)),
          perfTraceEnabled_(perfTraceEnabled) {
        visibleIndices_.reserve(512);
        preloadIndices_.reserve(1536);
        residentIndices_.reserve(kMaxActiveTexturesSafety);
        if (perfTraceEnabled_) {
            perfTraceFile_.open("perf_trace.csv", std::ios::out | std::ios::trunc);
            if (perfTraceFile_.is_open()) {
                perfTraceFile_
                    << "frame,total_ms,visible_ms,touch_ms,decoded_ms,upload_ms,animation_ms,evict_ms,request_ms,"
                    << "visible,preload,resident,decode_queue,completed,ready,cached_upload,workers_busy,"
                    << "gpu_mb,cpu_mb,scroll_y,framerate,decoded_pumped,upload_budget,uploads,queued\n";
            } else {
                perfTraceEnabled_ = false;
            }
        }
        rebuildLayout(1920.0f, 1080.0f);
        galleryDecodeConcurrencyLimit_.store(
            std::max(1, std::min<int>(static_cast<int>(workerCount_), kIdleGalleryDecodeConcurrencyCeiling)),
            std::memory_order_relaxed);
#if defined(_WIN32)
        captureSchedulingDefaults();
        configureUiThreadScheduling();
#endif
        startWorkers();
    }

    ~GalleryApp() {
        stopWorkers();
#if defined(_WIN32)
        restoreSchedulingDefaults();
#endif
        releaseTextures();
        releaseTextureUploadPbos();
        if (perfTraceFile_.is_open()) {
            perfTraceFile_.flush();
            perfTraceFile_.close();
        }
    }

    void updateViewport(const ImVec2& size) {
        const float safeWidth = std::max(1.0f, size.x);
        const float safeHeight = std::max(1.0f, size.y);
        if (std::abs(safeWidth - viewportWidth_) > 0.5f || std::abs(safeHeight - viewportHeight_) > 0.5f) {
            rebuildLayout(safeWidth, safeHeight);
        }
        clampZoomPanOffset();
    }

    void tick(const ImGuiIO& io) {
        ++frameIndex_;
        lastFrameDeltaSeconds_ = io.DeltaTime;

        const auto nowMs = [] {
            return std::chrono::duration<double, std::milli>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        };

        const double tickStartMs = nowMs();
        handleScroll(io);
        if (zoomedIndex_.has_value() || lastFrameDeltaSeconds_ <= 0.0f) {
            lastScrollVelocityPixelsPerSecond_ = 0.0;
        } else {
            lastScrollVelocityPixelsPerSecond_ =
                std::abs(scrollY_ - lastScrollVelocitySampleY_) /
                static_cast<double>(lastFrameDeltaSeconds_);
        }
        if (!zoomedIndex_.has_value() && std::abs(scrollY_ - lastScrollRevisionY_) > 0.5) {
            scrollRevision_.fetch_add(1, std::memory_order_relaxed);
            lastScrollRevisionY_ = scrollY_;
        }
        lastScrollVelocitySampleY_ = scrollY_;
#if defined(_WIN32)
        updateSchedulingProfile(io);
#endif
        updateDecodeConcurrencyLimit(io);
        const double afterScrollMs = nowMs();
        collectVisibleSets();
        const double afterVisibleMs = nowMs();
        touchVisibleItems();
        const double afterTouchMs = nowMs();
        collectDecodedImages();
        const double afterDecodedMs = nowMs();
        processReadyUploads();
        const double afterUploadMs = nowMs();
        updateAnimatedMedia(io.DeltaTime);
        const double afterAnimationMs = nowMs();
        evictTextures();
        const double afterEvictMs = nowMs();
        requestVisibleDecodes();
        const double afterRequestMs = nowMs();

        if (perfTraceEnabled_) {
            const PerfTraceSample sample{
                afterRequestMs - tickStartMs,
                afterVisibleMs - afterScrollMs,
                afterTouchMs - afterVisibleMs,
                afterDecodedMs - afterTouchMs,
                afterUploadMs - afterDecodedMs,
                afterAnimationMs - afterUploadMs,
                afterEvictMs - afterAnimationMs,
                afterRequestMs - afterEvictMs,
            };
            tracePerfFrame(io, sample);
        }
    }

    bool handleScrollbar(
        const ImVec2& canvasOrigin,
        const ImVec2& canvasSize,
        const ImVec2& mousePosition,
        bool leftClicked,
        bool leftDown,
        bool leftReleased) {
        if (zoomedIndex_.has_value()) {
            scrollbarDragging_ = false;
            return false;
        }

        const ScrollbarGeometry scrollbar = computeScrollbarGeometry(canvasOrigin, canvasSize);
        if (!scrollbar.visible) {
            scrollbarDragging_ = false;
            return false;
        }

        if (leftReleased) {
            scrollbarDragging_ = false;
        }

        if (scrollbarDragging_) {
            if (!leftDown) {
                scrollbarDragging_ = false;
                return true;
            }

            const float thumbTop = std::clamp(
                mousePosition.y - scrollbarDragOffsetY_,
                scrollbar.trackMin.y,
                scrollbar.trackMax.y - scrollbar.thumbHeight);
            const float ratio = scrollbar.thumbTravel > 0.0f
                ? (thumbTop - scrollbar.trackMin.y) / scrollbar.thumbTravel
                : 0.0f;
            setScrollY(static_cast<double>(ratio) * maxScroll());
            return true;
        }

        if (!PointInRect(mousePosition, scrollbar.trackMin, scrollbar.trackMax) || !leftClicked) {
            return false;
        }

        if (PointInRect(mousePosition, scrollbar.thumbMin, scrollbar.thumbMax)) {
            scrollbarDragging_ = true;
            scrollbarDragOffsetY_ = mousePosition.y - scrollbar.thumbMin.y;
            return true;
        }

        const float thumbTop = std::clamp(
            mousePosition.y - (scrollbar.thumbHeight * 0.5f),
            scrollbar.trackMin.y,
            scrollbar.trackMax.y - scrollbar.thumbHeight);
        const float ratio = scrollbar.thumbTravel > 0.0f
            ? (thumbTop - scrollbar.trackMin.y) / scrollbar.thumbTravel
            : 0.0f;
        setScrollY(static_cast<double>(ratio) * maxScroll());
        scrollbarDragging_ = true;
        scrollbarDragOffsetY_ = mousePosition.y - thumbTop;
        return true;
    }

    std::optional<fs::path> handlePointer(
        const ImVec2& canvasOrigin,
        const ImVec2& mousePosition,
        bool leftClicked,
        bool leftDown,
        bool leftReleased,
        bool rightClicked) {
        hoveredIndex_.reset();

        if (zoomedIndex_.has_value()) {
            const std::size_t index = *zoomedIndex_;
            if (rightClicked) {
                return items_[index].path;
            }

            const ImVec2 displaySize = zoomDisplaySize();
            const ImVec2 imageSize = computeZoomImageSize(items_[index], displaySize);
            const bool canPan = zoomCanPan(imageSize, displaySize);

            if (leftClicked) {
                zoomDragPending_ = true;
                zoomDragging_ = false;
                zoomDragOriginMouse_ = mousePosition;
                zoomDragOriginPan_ = zoomPanOffset_;
            }

            if (zoomDragPending_ && leftDown) {
                const ImVec2 dragDelta = mousePosition - zoomDragOriginMouse_;
                if (canPan) {
                    zoomPanOffset_ = clampZoomPanOffset(zoomDragOriginPan_ + dragDelta, imageSize, displaySize);
                }
                if (std::abs(dragDelta.x) >= kZoomDragThreshold || std::abs(dragDelta.y) >= kZoomDragThreshold) {
                    if (canPan) {
                        zoomDragging_ = true;
                        suppressZoomClose_ = true;
                    }
                }
            }

            if (leftReleased) {
                const bool shouldClose = !zoomDragging_ && !suppressZoomClose_;
                zoomDragPending_ = false;
                zoomDragging_ = false;
                if (shouldClose) {
                    closeZoom();
                } else {
                    suppressZoomClose_ = false;
                }
            } else if (!leftDown && zoomDragPending_) {
                zoomDragPending_ = false;
                zoomDragging_ = false;
                suppressZoomClose_ = false;
            }

            return std::nullopt;
        }

        for (auto it = visibleIndices_.rbegin(); it != visibleIndices_.rend(); ++it) {
            const std::size_t index = *it;
            const auto& item = items_[index];
            const ImVec2 min = canvasOrigin + ImVec2(item.x, item.y - static_cast<float>(scrollY_));
            const ImVec2 max = min + ImVec2(item.w, item.h);
            if (!PointInRect(mousePosition, min, max)) {
                continue;
            }

            hoveredIndex_ = index;
            if (rightClicked) {
                return item.path;
            }
            if (leftClicked) {
                zoomedIndex_ = index;
                resetZoomInteraction();
                suppressZoomClose_ = true;
                items_[index].lastTouchedFrame = frameIndex_;
                requestZoomDecode(index);
                queueDecode(index, true);
            }
            break;
        }

        return std::nullopt;
    }

    void drawGallery(ImDrawList* drawList, const ImVec2& canvasOrigin, const ImVec2& canvasSize) {
        drawList->AddRectFilledMultiColor(
            canvasOrigin,
            canvasOrigin + canvasSize,
            kBackgroundTop,
            kBackgroundTop,
            kBackgroundBottom,
            kBackgroundBottom);

        if (layoutOrder_.empty()) {
            const ImVec2 textSize = ImGui::CalcTextSize("No supported images or videos were found.");
            const ImVec2 textPos = canvasOrigin + (canvasSize - textSize) * 0.5f;
            drawList->AddText(textPos, IM_COL32(230, 235, 240, 255), "No supported images or videos were found.");
            return;
        }

        for (const std::size_t index : visibleIndices_) {
            const auto& item = items_[index];
            const ImVec2 min = canvasOrigin + ImVec2(item.x, item.y - static_cast<float>(scrollY_));
            const ImVec2 max = min + ImVec2(item.w, item.h);

            if (kTileSpacing > 0.0f) {
                drawList->AddRectFilled(min + ImVec2(4.0f, 6.0f), max + ImVec2(4.0f, 6.0f), IM_COL32(0, 0, 0, 36));
            }

            if (item.texture != 0) {
                drawList->AddImage((ImTextureID)(intptr_t)item.texture, min, max);
            } else {
                const ImU32 base = PlaceholderTint(index, 210);
                const ImU32 accent = PlaceholderTint(index + 17, 160);
                drawList->AddRectFilledMultiColor(min, max, base, accent, accent, base);
                drawList->AddLine(min + ImVec2(0.0f, item.h * 0.22f), max - ImVec2(0.0f, item.h * 0.22f), IM_COL32(255, 255, 255, 16), 1.0f);
                drawList->AddLine(min + ImVec2(item.w * 0.18f, 0.0f), max - ImVec2(item.w * 0.18f, 0.0f), IM_COL32(255, 255, 255, 12), 1.0f);
            }

            ImU32 border = kBorderColor;
            if (hoveredIndex_.has_value() && *hoveredIndex_ == index) {
                border = IM_COL32(255, 255, 255, 92);
            }
            drawList->AddRect(min, max, border, 0.0f, 0, 1.0f);

            if (const char* badgeText = MediaKindBadgeText(item.kind)) {
                const ImVec2 textSize = ImGui::CalcTextSize(badgeText);
                const ImVec2 badgePadding(8.0f, 5.0f);
                const ImVec2 badgeMin = min + ImVec2(8.0f, 8.0f);
                const ImVec2 badgeMax = badgeMin + textSize + badgePadding * 2.0f;
                drawList->AddRectFilled(badgeMin, badgeMax, IM_COL32(0, 0, 0, 156), 6.0f);
                drawList->AddText(badgeMin + badgePadding, IM_COL32(245, 245, 245, 255), badgeText);
            }
        }
    }

    void drawScrollbar(ImDrawList* drawList, const ImVec2& canvasOrigin, const ImVec2& canvasSize) const {
        if (zoomedIndex_.has_value()) {
            return;
        }

        const ScrollbarGeometry scrollbar = computeScrollbarGeometry(canvasOrigin, canvasSize);
        if (!scrollbar.visible) {
            return;
        }

        const ImVec2 mousePosition = ImGui::GetIO().MousePos;
        const bool thumbHovered = PointInRect(mousePosition, scrollbar.thumbMin, scrollbar.thumbMax);
        const ImU32 thumbColor = scrollbarDragging_
            ? kScrollbarThumbActiveColor
            : (thumbHovered ? kScrollbarThumbHoveredColor : kScrollbarThumbColor);

        drawList->AddRectFilled(scrollbar.trackMin, scrollbar.trackMax, kScrollbarTrackColor, 7.0f);
        drawList->AddRect(scrollbar.trackMin, scrollbar.trackMax, IM_COL32(255, 255, 255, 20), 7.0f, 0, 1.0f);
        drawList->AddRectFilled(scrollbar.thumbMin, scrollbar.thumbMax, thumbColor, 7.0f);
    }

    void drawZoomOverlay(const ImVec2& displaySize) {
        if (!zoomedIndex_.has_value()) {
            return;
        }

        const std::size_t index = *zoomedIndex_;
        const auto& item = items_[index];
        const bool hasZoomTexture = zoomTextureIndex_.has_value() && *zoomTextureIndex_ == index && zoomTexture_ != 0;
        const GLuint overlayTexture = hasZoomTexture ? zoomTexture_ : item.texture;
        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        drawList->AddRectFilled(ImVec2(0.0f, 0.0f), displaySize, kOverlayBackdrop);

        const ImVec2 imageSize = computeZoomImageSize(item, displaySize);
        zoomPanOffset_ = clampZoomPanOffset(zoomPanOffset_, imageSize, displaySize);
        const ImVec2 imageMin(
            (displaySize.x - imageSize.x) * 0.5f + zoomPanOffset_.x,
            (displaySize.y - imageSize.y) * 0.5f + zoomPanOffset_.y);
        const ImVec2 imageMax = imageMin + imageSize;

        if (overlayTexture != 0) {
            drawList->AddImage((ImTextureID)(intptr_t)overlayTexture, imageMin, imageMax);
            drawList->AddRect(imageMin, imageMax, IM_COL32(255, 255, 255, 64), 0.0f, 0, 1.0f);
        } else {
            drawList->AddRectFilled(imageMin, imageMax, PlaceholderTint(index, 190));
            drawList->AddRect(imageMin, imageMax, IM_COL32(255, 255, 255, 48), 0.0f, 0, 1.0f);

            const char* message = "Loading full-resolution texture...";
            const ImVec2 textSize = ImGui::CalcTextSize(message);
            drawList->AddText(
                imageMin + (imageSize - textSize) * 0.5f,
                IM_COL32(245, 245, 245, 255),
                message);
        }
    }

    ImVec2 zoomDisplaySize() const {
        return ImVec2(viewportWidth_, viewportHeight_);
    }

    ImVec2 computeZoomImageSize(const ImageRecord& item, const ImVec2& displaySize) const {
        const bool hasZoomTexture =
            zoomedIndex_.has_value() &&
            zoomTextureIndex_.has_value() &&
            *zoomTextureIndex_ == *zoomedIndex_ &&
            zoomTexture_ != 0;
        const int sourceTextureWidth = hasZoomTexture ? zoomTextureWidth_ : item.textureWidth;
        const int sourceTextureHeight = hasZoomTexture ? zoomTextureHeight_ : item.textureHeight;
        const float sourceWidth = static_cast<float>(std::max(1, sourceTextureWidth > 0 ? sourceTextureWidth : item.sourceWidth));
        const float sourceHeight = static_cast<float>(std::max(1, sourceTextureHeight > 0 ? sourceTextureHeight : item.sourceHeight));
        const float fitScale = std::min((displaySize.x * 0.92f) / sourceWidth, (displaySize.y * 0.92f) / sourceHeight);
        const float scale = fitScale * zoomScale_;
        return ImVec2(sourceWidth * scale, sourceHeight * scale);
    }

    bool zoomCanPan(const ImVec2& imageSize, const ImVec2& displaySize) const {
        return imageSize.x > displaySize.x + 0.5f || imageSize.y > displaySize.y + 0.5f;
    }

    ImVec2 clampZoomPanOffset(const ImVec2& offset, const ImVec2& imageSize, const ImVec2& displaySize) const {
        ImVec2 clamped = offset;
        const float overflowX = std::max(0.0f, imageSize.x - displaySize.x);
        const float overflowY = std::max(0.0f, imageSize.y - displaySize.y);
        clamped.x = overflowX > 0.0f ? std::clamp(offset.x, -overflowX * 0.5f, overflowX * 0.5f) : 0.0f;
        clamped.y = overflowY > 0.0f ? std::clamp(offset.y, -overflowY * 0.5f, overflowY * 0.5f) : 0.0f;
        return clamped;
    }

    void clampZoomPanOffset() {
        if (!zoomedIndex_.has_value()) {
            zoomPanOffset_ = ImVec2(0.0f, 0.0f);
            return;
        }
        zoomPanOffset_ = clampZoomPanOffset(zoomPanOffset_, computeZoomImageSize(items_[*zoomedIndex_], zoomDisplaySize()), zoomDisplaySize());
    }

    bool drawOverlay(const fs::path& rootDirectory, bool* requestOpenFolder, bool* vsyncEnabled) {
        ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.34f);

        const ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_AlwaysAutoResize;

        bool wantsPointer = false;
        if (ImGui::Begin("TelemetryOverlay", nullptr, flags)) {
            const ImGuiIO& io = ImGui::GetIO();
            const std::string rootText = rootDirectory.u8string();
            const std::size_t staticCount = mediaKindCount(MediaKind::StaticImage);
            const std::size_t gifCount = mediaKindCount(MediaKind::AnimatedGif);
            const std::size_t videoCount = mediaKindCount(MediaKind::Video);
            if (ImGui::Button("Open Folder") && requestOpenFolder != nullptr) {
                *requestOpenFolder = true;
            }
            if (videoCount > 0) {
                ImGui::SameLine();
                if (ImGui::Button("Jump Video")) {
                    setClipboardStatus(jumpToFirstMedia(MediaKind::Video) ? "Jumped to first video." : "No video found.");
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(overlayExpanded_ ? "Hide Stats" : "Show Stats")) {
                overlayExpanded_ = !overlayExpanded_;
            }
            ImGui::SameLine();
            if (vsyncEnabled != nullptr) {
                ImGui::Checkbox("VSync", vsyncEnabled);
            }
            if (overlayExpanded_) {
                ImGui::Separator();
                ImGui::Text("FPS %.1f", io.Framerate);
                ImGui::Text("Frame %.2f ms", io.Framerate > 0.0f ? 1000.0f / io.Framerate : 0.0f);
                ImGui::Text("Images %zu", activeImageCount());
                ImGui::Text("Static %zu  GIF %zu  Video %zu", staticCount, gifCount, videoCount);
                ImGui::Text("Visible %zu", visibleIndices_.size());
                ImGui::Text("Columns %d%s", currentColumnCount_, manualColumnCount_.has_value() ? " manual" : " auto");
                ImGui::Text("Resident %zu / %zu", residentIndices_.size(), kMaxActiveTexturesSafety);
                ImGui::Text(
                    "VRAM est %.2f / %.0f GB",
                    static_cast<double>(gpuTextureBytes_) / (1024.0 * 1024.0 * 1024.0),
                    static_cast<double>(kMaxGpuTextureBytes) / (1024.0 * 1024.0 * 1024.0));
                ImGui::Text("Decode queue %zu", decodeQueue_.size());
                ImGui::Text("Decoded staging %zu", readyUploads_.size() + completedQueue_.size() + cachedUploadIndices_.size());
                ImGui::Text(
                    "RAM cache %.2f / %.0f GB",
                    static_cast<double>(cpuImageCacheBytes_) / (1024.0 * 1024.0 * 1024.0),
                    static_cast<double>(kMaxCpuImageCacheBytes) / (1024.0 * 1024.0 * 1024.0));
                ImGui::Text("Workers busy %d / %zu", activeWorkers_.load(), workerCount_);
#if defined(_WIN32)
                if (uiThreadPreferredProcessor_ >= 0) {
                    ImGui::Text(
                        "CPU UI core %d  workers %s",
                        uiThreadPreferredProcessor_,
                        uiCoreIsolationActive_ ? "isolated" : "shared");
                }
#endif
                ImGui::Text("Scroll %.0f / %.0f", scrollY_, static_cast<float>(maxScroll()));
                ImGui::TextWrapped("Root %s", rootText.c_str());
                ImGui::TextWrapped("%s", VideoRuntimeStatusText());
                ImGui::Text("Controls wheel/up/down scroll  left/right cols  V jump video  drag bar scroll");
                ImGui::Text("left zoom  wheel zoom image  drag pan zoom  right copy  del delete zoom");
            }
            if (!clipboardStatus_.empty() && ImGui::GetTime() < clipboardStatusExpiry_) {
                if (overlayExpanded_) {
                    ImGui::Separator();
                }
                ImGui::Text("%s", clipboardStatus_.c_str());
            }
            wantsPointer = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) || ImGui::IsAnyItemActive();
        }
        ImGui::End();
        return wantsPointer;
    }

    bool hasZoom() const {
        return zoomedIndex_.has_value();
    }

    bool deleteZoomedImage() {
        if (!zoomedIndex_.has_value()) {
            return false;
        }

        const std::size_t index = *zoomedIndex_;
        if (index >= items_.size()) {
            releaseZoomTexture();
            zoomedIndex_.reset();
            resetZoomInteraction();
            return false;
        }

        auto& item = items_[index];
        if (item.removed) {
            releaseZoomTexture();
            zoomedIndex_.reset();
            resetZoomInteraction();
            return false;
        }

        if (!DeleteImageFile(item.path)) {
            return false;
        }

        if (item.texture != 0) {
            evictTexture(index);
        }

        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            item.state = LoadState::Empty;
        }

        item.removed = true;
        item.textureWidth = 0;
        item.textureHeight = 0;
        item.textureBytes = 0;
        item.lastTouchedFrame = 0;
        clearAnimationState(item);
        clearImageCache(item);

        for (std::size_t i = 0; i < readyUploads_.size();) {
            if (readyUploads_[i].index == index) {
                readyUploads_.erase(readyUploads_.begin() + static_cast<std::ptrdiff_t>(i));
                continue;
            }
            ++i;
        }

        if (hoveredIndex_.has_value() && *hoveredIndex_ == index) {
            hoveredIndex_.reset();
        }
        releaseZoomTexture();
        zoomedIndex_.reset();
        resetZoomInteraction();
        scrollbarDragging_ = false;

        rebuildLayout(viewportWidth_, viewportHeight_);
        collectVisibleSets();
        return true;
    }

    void closeZoom() {
        releaseZoomTexture();
        zoomedIndex_.reset();
        resetZoomInteraction();
    }

    void releaseZoomTexture() {
        if (zoomTexture_ != 0) {
            gpuTextureBytes_ = gpuTextureBytes_ >= zoomTextureBytes_ ? gpuTextureBytes_ - zoomTextureBytes_ : 0;
            glDeleteTextures(1, &zoomTexture_);
            zoomTexture_ = 0;
        }
        zoomTextureIndex_.reset();
        zoomTextureWidth_ = 0;
        zoomTextureHeight_ = 0;
        zoomTextureBytes_ = 0;
        readyZoomUploads_.clear();
    }

    int galleryDecodeMaxDimensionFor(std::size_t index) const {
        if (index >= items_.size()) {
            return kGalleryDecodeDimensionMax;
        }

        const auto& item = items_[index];
        const bool fastScroll = isFastScroll();
        const float screenBound = std::max(viewportWidth_, viewportHeight_) * 1.15f;
        const float visibleMaxSide = std::min(std::max(item.w, item.h), screenBound);
        const float desired = std::max(
            static_cast<float>(fastScroll ? kScrollPreviewDecodeDimensionMin : kGalleryDecodeDimensionMin),
            visibleMaxSide * (fastScroll ? 1.15f : 2.0f));
        return std::clamp(
            static_cast<int>(std::ceil(desired)),
            fastScroll ? kScrollPreviewDecodeDimensionMin : kGalleryDecodeDimensionMin,
            std::min(maxTextureSize_, fastScroll ? kScrollPreviewDecodeDimensionMax : kGalleryDecodeDimensionMax));
    }

    int zoomDecodeMaxDimensionFor(std::size_t index) const {
        if (index >= items_.size()) {
            return std::min(maxTextureSize_, kZoomDecodeDimensionMax);
        }

        const auto& item = items_[index];
        const int sourceMaxSide = std::max(item.sourceWidth, item.sourceHeight);
        if (sourceMaxSide <= 0) {
            return std::min(maxTextureSize_, kZoomDecodeDimensionMax);
        }

        const float desired = std::max(viewportWidth_, viewportHeight_) * 2.25f;
        const int upperBound = std::min({maxTextureSize_, kZoomDecodeDimensionMax, sourceMaxSide});
        if (upperBound <= kGalleryDecodeDimensionMax) {
            return upperBound;
        }
        return std::clamp(
            static_cast<int>(std::ceil(desired)),
            kGalleryDecodeDimensionMax,
            upperBound);
    }

    void requestZoomDecode(std::size_t index) {
        if (index >= items_.size()) {
            return;
        }

        const auto& item = items_[index];
        if (item.removed || item.kind != MediaKind::StaticImage) {
            return;
        }

        if (zoomTextureIndex_.has_value() &&
            *zoomTextureIndex_ == index &&
            zoomTexture_ != 0 &&
            std::max(zoomTextureWidth_, zoomTextureHeight_) >= zoomDecodeMaxDimensionFor(index)) {
            return;
        }

        decodeQueue_.push(
            DecodeJob{index, DecodeMode::Zoom, zoomDecodeMaxDimensionFor(index), 0, false, false},
            true);
    }

    double scrollY() const {
        return scrollY_;
    }

    void setScrollY(double scrollY) {
        scrollY_ = scrollY;
        scrollTargetY_ = scrollY;
        scrollAnimatedVelocityPixelsPerSecond_ = 0.0;
        clampScroll();
    }

    double maxScroll() const {
        return std::max(0.0, static_cast<double>(contentHeight_ - viewportHeight_));
    }

    std::optional<fs::path> zoomedPath() const {
        if (!zoomedIndex_.has_value()) {
            return std::nullopt;
        }
        return items_[*zoomedIndex_].path;
    }

    void setClipboardStatus(const char* statusText) {
        clipboardStatus_ = statusText != nullptr ? statusText : "";
        clipboardStatusExpiry_ = ImGui::GetTime() + 1.6;
    }

    void resetZoomInteraction() {
        zoomScale_ = 1.0f;
        zoomPanOffset_ = ImVec2(0.0f, 0.0f);
        zoomDragOriginMouse_ = ImVec2(0.0f, 0.0f);
        zoomDragOriginPan_ = ImVec2(0.0f, 0.0f);
        zoomDragPending_ = false;
        zoomDragging_ = false;
        suppressZoomClose_ = false;
    }

    bool jumpToFirstMedia(MediaKind kind) {
        for (const std::size_t index : layoutOrder_) {
            const auto& item = items_[index];
            if (!item.removed && item.kind == kind) {
                setScrollY(item.y);
                hoveredIndex_ = index;
                return true;
            }
        }
        return false;
    }

private:
    void tracePerfFrame(const ImGuiIO& io, const PerfTraceSample& sample) {
        if (!perfTraceEnabled_ || !perfTraceFile_.is_open()) {
            return;
        }

        perfTraceFile_
            << frameIndex_ << ','
            << std::fixed << std::setprecision(3)
            << sample.totalMs << ','
            << sample.visibleMs << ','
            << sample.touchMs << ','
            << sample.collectDecodedMs << ','
            << sample.uploadMs << ','
            << sample.animationMs << ','
            << sample.evictMs << ','
            << sample.requestMs << ','
            << visibleIndices_.size() << ','
            << preloadIndices_.size() << ','
            << residentIndices_.size() << ','
            << decodeQueue_.size() << ','
            << completedQueue_.size() << ','
            << readyUploads_.size() << ','
            << cachedUploadIndices_.size() << ','
            << activeWorkers_.load() << ','
            << (static_cast<double>(gpuTextureBytes_) / (1024.0 * 1024.0)) << ','
            << (static_cast<double>(cpuImageCacheBytes_) / (1024.0 * 1024.0)) << ','
            << scrollY_ << ','
            << io.Framerate << ','
            << lastDecodedPumpCount_ << ','
            << lastUploadBudget_ << ','
            << lastUploadsThisFrame_ << ','
            << lastQueuedThisFrame_
            << '\n';

        if (frameIndex_ % 120 == 0) {
            perfTraceFile_.flush();
        }
    }

    void clearImageCache(ImageRecord& item) {
        if (item.cachedPixels != nullptr) {
            stbi_image_free(item.cachedPixels);
            item.cachedPixels = nullptr;
        }
        cpuImageCacheBytes_ = cpuImageCacheBytes_ >= item.cachedPixelBytes
            ? cpuImageCacheBytes_ - item.cachedPixelBytes
            : 0;
        item.cachedPixelBytes = 0;
        item.cachedWidth = 0;
        item.cachedHeight = 0;
        item.cachedLastTouchedFrame = 0;
    }

    void adoptImageCache(ImageRecord& item, DecodedImage& decoded) {
        if (decoded.kind != MediaKind::StaticImage || decoded.frameCount > 1 || decoded.pixels == nullptr) {
            clearImageCache(item);
            return;
        }

        clearImageCache(item);
        const std::size_t pixelBytes = decoded.pixelBytes();
        item.cachedPixels = decoded.releasePixels();
        item.cachedPixelBytes = pixelBytes;
        item.cachedWidth = decoded.width;
        item.cachedHeight = decoded.height;
        item.cachedLastTouchedFrame = frameIndex_;
        cpuImageCacheBytes_ += item.cachedPixelBytes;
    }

    void evictImageCaches() {
        while (cpuImageCacheBytes_ > kMaxCpuImageCacheBytes) {
            std::size_t oldestIndex = std::numeric_limits<std::size_t>::max();
            std::uint64_t oldestFrame = std::numeric_limits<std::uint64_t>::max();

            for (std::size_t index = 0; index < items_.size(); ++index) {
                const auto& item = items_[index];
                if (item.cachedPixels == nullptr || item.cachedPixelBytes == 0) {
                    continue;
                }
                if (zoomedIndex_.has_value() && *zoomedIndex_ == index) {
                    continue;
                }
                if (item.texture == 0 && item.state == LoadState::PendingUpload) {
                    continue;
                }
                if (item.cachedLastTouchedFrame < oldestFrame) {
                    oldestFrame = item.cachedLastTouchedFrame;
                    oldestIndex = index;
                }
            }

            if (oldestIndex == std::numeric_limits<std::size_t>::max()) {
                break;
            }
            clearImageCache(items_[oldestIndex]);
        }
    }

    void clearAnimationState(ImageRecord& item) {
        item.animationPixels.clear();
        item.animationFrameDelaysMs.clear();
        item.animationFrameCount = 0;
        item.animationFrameIndex = 0;
        item.animationFrameClockSeconds = 0.0;
    }

    double animationFrameDelaySeconds(const ImageRecord& item) const {
        if (item.animationFrameCount <= 1) {
            return 0.0;
        }
        const int delayMs =
            item.animationFrameIndex < static_cast<int>(item.animationFrameDelaysMs.size())
            ? item.animationFrameDelaysMs[static_cast<std::size_t>(item.animationFrameIndex)]
            : 100;
        return static_cast<double>(std::max(20, delayMs)) / 1000.0;
    }

    const stbi_uc* animationFramePixels(const ImageRecord& item, int frameIndex) const {
        if (frameIndex < 0 || frameIndex >= item.animationFrameCount || item.animationPixels.empty() ||
            item.textureWidth <= 0 || item.textureHeight <= 0) {
            return nullptr;
        }

        const std::size_t frameStride =
            static_cast<std::size_t>(item.textureWidth) *
            static_cast<std::size_t>(item.textureHeight) *
            4U;
        const std::size_t offset = frameStride * static_cast<std::size_t>(frameIndex);
        if (offset + frameStride > item.animationPixels.size()) {
            return nullptr;
        }
        return item.animationPixels.data() + offset;
    }

    bool canUseTextureUploadPbos() const {
        return g_glGenBuffers != nullptr &&
            g_glBindBuffer != nullptr &&
            g_glBufferData != nullptr &&
            g_glDeleteBuffers != nullptr &&
            ((g_glMapBufferRange != nullptr && g_glUnmapBuffer != nullptr) || g_glBufferSubData != nullptr);
    }

    void ensureTextureUploadPbos() {
        if (textureUploadPbosInitialized_ || !canUseTextureUploadPbos()) {
            return;
        }

        std::array<GLuint, kTextureUploadPboCount> buffers{};
        g_glGenBuffers(static_cast<int>(buffers.size()), buffers.data());
        for (const GLuint buffer : buffers) {
            if (buffer == 0) {
                std::array<GLuint, kTextureUploadPboCount> created{};
                int createdCount = 0;
                for (const GLuint createdBuffer : buffers) {
                    if (createdBuffer != 0) {
                        created[createdCount++] = createdBuffer;
                    }
                }
                if (createdCount > 0) {
                    g_glDeleteBuffers(createdCount, created.data());
                }
                return;
            }
        }

        for (std::size_t index = 0; index < buffers.size(); ++index) {
            textureUploadPbos_[index].buffer = buffers[index];
            textureUploadPbos_[index].capacityBytes = 0;
        }
        nextTextureUploadPbo_ = 0;
        textureUploadPbosInitialized_ = true;
    }

    void releaseTextureUploadPbos() {
        if (g_glBindBuffer != nullptr) {
            g_glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        }

        if (g_glDeleteBuffers != nullptr) {
            std::array<GLuint, kTextureUploadPboCount> buffers{};
            int bufferCount = 0;
            for (auto& pbo : textureUploadPbos_) {
                if (pbo.buffer != 0) {
                    buffers[bufferCount++] = pbo.buffer;
                }
                pbo.buffer = 0;
                pbo.capacityBytes = 0;
            }
            if (bufferCount > 0) {
                g_glDeleteBuffers(bufferCount, buffers.data());
            }
        } else {
            for (auto& pbo : textureUploadPbos_) {
                pbo.buffer = 0;
                pbo.capacityBytes = 0;
            }
        }

        nextTextureUploadPbo_ = 0;
        textureUploadPbosInitialized_ = false;
    }

    TextureUploadPbo* acquireTextureUploadPbo() {
        ensureTextureUploadPbos();
        if (!textureUploadPbosInitialized_) {
            return nullptr;
        }

        TextureUploadPbo* pbo = &textureUploadPbos_[nextTextureUploadPbo_];
        nextTextureUploadPbo_ = (nextTextureUploadPbo_ + 1) % textureUploadPbos_.size();
        return pbo->buffer != 0 ? pbo : nullptr;
    }

    bool stageTextureUploadPixels(TextureUploadPbo& pbo, const stbi_uc* pixels, std::size_t pixelBytes) {
        if (pixels == nullptr || pixelBytes == 0 || pbo.buffer == 0 ||
            g_glBindBuffer == nullptr || g_glBufferData == nullptr) {
            return false;
        }

        g_glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo.buffer);
        if (pbo.capacityBytes < pixelBytes) {
            pbo.capacityBytes = pixelBytes;
        }

        const std::ptrdiff_t bufferBytes = static_cast<std::ptrdiff_t>(pbo.capacityBytes);
        const std::ptrdiff_t copyBytes = static_cast<std::ptrdiff_t>(pixelBytes);
        g_glBufferData(GL_PIXEL_UNPACK_BUFFER, bufferBytes, nullptr, GL_STREAM_DRAW);

        if (g_glMapBufferRange != nullptr && g_glUnmapBuffer != nullptr) {
            void* mapped = g_glMapBufferRange(
                GL_PIXEL_UNPACK_BUFFER,
                0,
                copyBytes,
                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
            if (mapped != nullptr) {
                std::memcpy(mapped, pixels, pixelBytes);
                if (g_glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER) != 0) {
                    return true;
                }
            }
        }

        if (g_glBufferSubData != nullptr) {
            g_glBufferSubData(GL_PIXEL_UNPACK_BUFFER, 0, copyBytes, pixels);
            return true;
        }

        g_glBufferData(GL_PIXEL_UNPACK_BUFFER, copyBytes, pixels, GL_STREAM_DRAW);
        pbo.capacityBytes = pixelBytes;
        return true;
    }

    bool uploadBoundTexturePixels(int width, int height, const stbi_uc* pixels, bool subImage) {
        if (pixels == nullptr || width <= 0 || height <= 0 || !canUseTextureUploadPbos()) {
            return false;
        }

        TextureUploadPbo* pbo = acquireTextureUploadPbo();
        if (pbo == nullptr) {
            return false;
        }

        const std::size_t pixelBytes = TextureByteSize(width, height);
        if (!stageTextureUploadPixels(*pbo, pixels, pixelBytes)) {
            g_glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
            return false;
        }

        if (subImage) {
            glTexSubImage2D(
                GL_TEXTURE_2D,
                0,
                0,
                0,
                width,
                height,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                nullptr);
        } else {
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA8,
                width,
                height,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                nullptr);
        }

        g_glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        return true;
    }

    GLuint createTexture(int width, int height, const stbi_uc* pixels, bool useMipmaps) {
        if (pixels == nullptr || width <= 0 || height <= 0) {
            return 0;
        }

        GLuint texture = 0;
        glGenTextures(1, &texture);
        if (texture == 0) {
            return 0;
        }

        const bool enableMipmaps = useMipmaps && g_glGenerateMipmap != nullptr;
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, enableMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        if (!uploadBoundTexturePixels(width, height, pixels, false)) {
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA8,
                width,
                height,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                pixels);
        }
        if (enableMipmaps) {
            g_glGenerateMipmap(GL_TEXTURE_2D);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        return texture;
    }

    GLuint createTexture(const DecodedImage& decoded) {
        return createTexture(decoded.width, decoded.height, decoded.pixels, ShouldUseMipmapsForDecodedImage(decoded));
    }

    void uploadAnimationFrame(ImageRecord& item) {
        const stbi_uc* framePixels = animationFramePixels(item, item.animationFrameIndex);
        if (framePixels == nullptr || item.texture == 0) {
            return;
        }

        glBindTexture(GL_TEXTURE_2D, item.texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        if (!uploadBoundTexturePixels(item.textureWidth, item.textureHeight, framePixels, true)) {
            glTexSubImage2D(
                GL_TEXTURE_2D,
                0,
                0,
                0,
                item.textureWidth,
                item.textureHeight,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                framePixels);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void updateAnimatedItem(std::size_t index, double deltaSeconds) {
        if (index >= items_.size()) {
            return;
        }

        auto& item = items_[index];
        if ((item.kind != MediaKind::AnimatedGif && item.kind != MediaKind::Video) ||
            item.texture == 0 ||
            item.animationFrameCount <= 1) {
            return;
        }

        if (animationFrameDelaySeconds(item) <= 0.0) {
            return;
        }

        item.animationFrameClockSeconds += std::max(0.0, deltaSeconds);
        bool advanced = false;
        int safetyCounter = 0;
        while (item.animationFrameClockSeconds >= animationFrameDelaySeconds(item) &&
               safetyCounter < item.animationFrameCount * 2) {
            item.animationFrameClockSeconds -= animationFrameDelaySeconds(item);
            item.animationFrameIndex = (item.animationFrameIndex + 1) % item.animationFrameCount;
            advanced = true;
            ++safetyCounter;
        }

        if (advanced) {
            uploadAnimationFrame(item);
        }
    }

    void updateAnimatedMedia(double deltaSeconds) {
        for (const std::size_t index : visibleIndices_) {
            updateAnimatedItem(index, deltaSeconds);
        }

        if (zoomedIndex_.has_value() &&
            std::find(visibleIndices_.begin(), visibleIndices_.end(), *zoomedIndex_) == visibleIndices_.end()) {
            updateAnimatedItem(*zoomedIndex_, deltaSeconds);
        }
    }

    std::size_t activeImageCount() const {
        return static_cast<std::size_t>(std::count_if(items_.begin(), items_.end(), [](const ImageRecord& item) {
            return !item.removed;
        }));
    }

    std::size_t mediaKindCount(MediaKind kind) const {
        return static_cast<std::size_t>(std::count_if(items_.begin(), items_.end(), [&](const ImageRecord& item) {
            return !item.removed && item.kind == kind;
        }));
    }

    ScrollbarGeometry computeScrollbarGeometry(const ImVec2& canvasOrigin, const ImVec2& canvasSize) const {
        ScrollbarGeometry geometry;
        const double scrollRange = maxScroll();
        if (scrollRange <= 0.0 || canvasSize.y <= 0.0f) {
            return geometry;
        }

        const float margin = 10.0f;
        const float width = 12.0f;
        const float trackTop = canvasOrigin.y + margin;
        const float trackBottom = canvasOrigin.y + canvasSize.y - margin;
        const float trackHeight = trackBottom - trackTop;
        if (trackHeight <= 0.0f) {
            return geometry;
        }

        const float visibleRatio = std::clamp(viewportHeight_ / contentHeight_, 0.0f, 1.0f);
        const float thumbHeight = std::max(48.0f, trackHeight * visibleRatio);
        const float thumbTravel = std::max(0.0f, trackHeight - thumbHeight);
        const float thumbOffset = scrollRange > 0.0
            ? static_cast<float>((scrollY_ / scrollRange) * thumbTravel)
            : 0.0f;

        geometry.visible = true;
        geometry.trackMin = ImVec2(canvasOrigin.x + canvasSize.x - margin - width, trackTop);
        geometry.trackMax = ImVec2(canvasOrigin.x + canvasSize.x - margin, trackBottom);
        geometry.thumbHeight = thumbHeight;
        geometry.thumbTravel = thumbTravel;
        geometry.thumbMin = ImVec2(geometry.trackMin.x, trackTop + thumbOffset);
        geometry.thumbMax = ImVec2(geometry.trackMax.x, geometry.thumbMin.y + thumbHeight);
        return geometry;
    }

    int autoColumnCountForWidth(float contentWidth) const {
        return std::max(1, static_cast<int>((contentWidth + kTileSpacing) / (kTargetTileWidth + kTileSpacing)));
    }

    int maxColumnCountForWidth(float contentWidth) const {
        return std::min(
            kMaxColumnCount,
            std::max(1, static_cast<int>((contentWidth + kTileSpacing) / (kMinTileWidth + kTileSpacing))));
    }

    void adjustColumnCount(int delta) {
        if (delta == 0) {
            return;
        }

        const float contentWidth = std::max(240.0f, viewportWidth_ - (kOuterPadding * 2.0f));
        const int maxColumns = maxColumnCountForWidth(contentWidth);
        const int baseColumns = std::clamp(currentColumnCount_, 1, maxColumns);
        const int targetColumns = std::clamp(baseColumns + delta, 1, maxColumns);
        if (targetColumns == currentColumnCount_ && manualColumnCount_.has_value() && *manualColumnCount_ == targetColumns) {
            return;
        }

        const double previousMaxScroll = maxScroll();
        const double scrollRatio = previousMaxScroll > 0.0 ? scrollY_ / previousMaxScroll : 0.0;

        manualColumnCount_ = targetColumns;
        rebuildLayout(viewportWidth_, viewportHeight_);

        if (previousMaxScroll > 0.0) {
            setScrollY(scrollRatio * maxScroll());
        }
    }

    void startWorkers() {
        workers_.reserve(workerCount_);
        for (std::size_t i = 0; i < workerCount_; ++i) {
            workers_.emplace_back([this, i] {
#if defined(_WIN32)
                configureDecodeWorkerThread(i);
#endif
                workerLoop();
            });
        }
    }

    void stopWorkers() {
        const std::vector<DecodeJob> droppedJobs = decodeQueue_.closeAndClear();
        for (const DecodeJob& droppedJob : droppedJobs) {
            discardQueuedJob(droppedJob);
        }
        completedQueue_.closeAndClear();
        completedZoomQueue_.closeAndClear();
        workerThrottleCondition_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        workers_.clear();
    }

    void releaseTextures() {
        releaseZoomTexture();
        for (auto& item : items_) {
            if (item.texture != 0) {
                gpuTextureBytes_ = gpuTextureBytes_ >= item.textureBytes ? gpuTextureBytes_ - item.textureBytes : 0;
                glDeleteTextures(1, &item.texture);
                item.texture = 0;
                item.textureWidth = 0;
                item.textureHeight = 0;
                item.textureBytes = 0;
                item.textureHasMipmaps = false;
                item.residentSlot = std::numeric_limits<std::size_t>::max();
            }
            item.galleryTextureMaxDimension = 0;
            clearGalleryUpgrade(item);
            clearAnimationState(item);
            clearImageCache(item);
        }
        residentIndices_.clear();
        cachedUploadIndices_.clear();
        gpuTextureBytes_ = 0;
    }

    void rebuildLayout(float width, float height) {
        viewportWidth_ = width;
        viewportHeight_ = height;

        if (activeImageCount() == 0) {
            contentHeight_ = height;
            scrollY_ = 0.0;
            scrollTargetY_ = 0.0;
            layoutOrder_.clear();
            bucketIndex_.rebuild(items_, contentHeight_, layoutOrder_);
            return;
        }

        const float contentWidth = std::max(240.0f, viewportWidth_ - (kOuterPadding * 2.0f));
        const int autoColumns = autoColumnCountForWidth(contentWidth);
        const int maxColumns = maxColumnCountForWidth(contentWidth);
        const int requestedColumns = manualColumnCount_.has_value() ? *manualColumnCount_ : autoColumns;
        const int columnCount = std::clamp(requestedColumns, 1, maxColumns);
        currentColumnCount_ = columnCount;
        const float tileWidth = (contentWidth - static_cast<float>(columnCount - 1) * kTileSpacing) / static_cast<float>(columnCount);

        std::vector<float> columnHeights(static_cast<std::size_t>(columnCount), kOuterPadding);

        for (auto& item : items_) {
            if (item.removed) {
                continue;
            }
            const auto bestColumnIt = std::min_element(columnHeights.begin(), columnHeights.end());
            const int bestColumn = static_cast<int>(std::distance(columnHeights.begin(), bestColumnIt));

            item.w = tileWidth;
            item.h = tileWidth * static_cast<float>(item.sourceHeight) / static_cast<float>(item.sourceWidth);
            item.x = kOuterPadding + static_cast<float>(bestColumn) * (tileWidth + kTileSpacing);
            item.y = columnHeights[static_cast<std::size_t>(bestColumn)];

            columnHeights[static_cast<std::size_t>(bestColumn)] += item.h + kTileSpacing;
        }

        layoutOrder_.clear();
        layoutOrder_.reserve(activeImageCount());
        for (std::size_t index = 0; index < items_.size(); ++index) {
            if (!items_[index].removed) {
                layoutOrder_.push_back(index);
            }
        }
        std::sort(layoutOrder_.begin(), layoutOrder_.end(), [&](std::size_t lhs, std::size_t rhs) {
            if (items_[lhs].y == items_[rhs].y) {
                return items_[lhs].x < items_[rhs].x;
            }
            return items_[lhs].y < items_[rhs].y;
        });

        contentHeight_ = *std::max_element(columnHeights.begin(), columnHeights.end()) + kOuterPadding;
        clampScroll();
        bucketIndex_.rebuild(items_, contentHeight_, layoutOrder_);
    }

    void handleScroll(const ImGuiIO& io) {
        if (zoomedIndex_.has_value()) {
            scrollTargetY_ = scrollY_;
            scrollAnimatedVelocityPixelsPerSecond_ = 0.0;
            if (std::abs(io.MouseWheel) > 0.001f) {
                zoomScale_ = std::clamp(
                    zoomScale_ + (io.MouseWheel * kZoomWheelLinearStep),
                    kZoomScaleMin,
                    kZoomScaleMax);
                clampZoomPanOffset();
            }
            return;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false)) {
            adjustColumnCount(-1);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false)) {
            adjustColumnCount(1);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_V, false)) {
            jumpToFirstMedia(MediaKind::Video);
        }

        if (std::abs(io.MouseWheel) > 0.001f) {
            scrollTargetY_ -= static_cast<double>(io.MouseWheel) * kWheelStep;
        }

        const double keyboardStep = std::max(240.0, static_cast<double>(viewportHeight_) * 1.25) * io.DeltaTime;
        bool immediateScrollChange = false;
        if (ImGui::IsKeyDown(ImGuiKey_DownArrow)) {
            scrollY_ += keyboardStep;
            immediateScrollChange = true;
        }
        if (ImGui::IsKeyDown(ImGuiKey_UpArrow)) {
            scrollY_ -= keyboardStep;
            immediateScrollChange = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) {
            scrollY_ += viewportHeight_ * 0.9;
            immediateScrollChange = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) {
            scrollY_ -= viewportHeight_ * 0.9;
            immediateScrollChange = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Home)) {
            scrollY_ = 0.0;
            immediateScrollChange = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_End)) {
            scrollY_ = std::max(0.0f, contentHeight_ - viewportHeight_);
            immediateScrollChange = true;
        }

        if (immediateScrollChange) {
            scrollTargetY_ = scrollY_;
            scrollAnimatedVelocityPixelsPerSecond_ = 0.0;
        } else {
            animateWheelScroll(io.DeltaTime);
        }

        clampScroll();
    }

    void animateWheelScroll(float deltaSeconds) {
        const double scrollDelta = scrollTargetY_ - scrollY_;
        if (std::abs(scrollDelta) <= kWheelScrollSnapDistance &&
            std::abs(scrollAnimatedVelocityPixelsPerSecond_) <= kWheelScrollSnapVelocityPixelsPerSecond) {
            scrollY_ = scrollTargetY_;
            scrollAnimatedVelocityPixelsPerSecond_ = 0.0;
            return;
        }
        if (deltaSeconds <= 0.0f) {
            scrollY_ = scrollTargetY_;
            scrollAnimatedVelocityPixelsPerSecond_ = 0.0;
            return;
        }

        const double desiredVelocity = std::clamp(
            scrollDelta / kWheelScrollCatchupSeconds,
            -kWheelScrollMaxSpeedPixelsPerSecond,
            kWheelScrollMaxSpeedPixelsPerSecond);
        const double maxVelocityDelta =
            kWheelScrollAccelerationPixelsPerSecond2 * static_cast<double>(deltaSeconds);
        scrollAnimatedVelocityPixelsPerSecond_ += std::clamp(
            desiredVelocity - scrollAnimatedVelocityPixelsPerSecond_,
            -maxVelocityDelta,
            maxVelocityDelta);

        const double frameScrollDelta =
            scrollAnimatedVelocityPixelsPerSecond_ * static_cast<double>(deltaSeconds);
        if (std::abs(frameScrollDelta) >= std::abs(scrollDelta)) {
            scrollY_ = scrollTargetY_;
            scrollAnimatedVelocityPixelsPerSecond_ = 0.0;
            return;
        }
        scrollY_ += frameScrollDelta;
    }

    void clampScroll() {
        const double scrollLimit = maxScroll();
        scrollTargetY_ = std::clamp(scrollTargetY_, 0.0, scrollLimit);
        scrollY_ = std::clamp(scrollY_, 0.0, scrollLimit);
        if ((scrollY_ <= 0.0 && scrollAnimatedVelocityPixelsPerSecond_ < 0.0) ||
            (scrollY_ >= scrollLimit && scrollAnimatedVelocityPixelsPerSecond_ > 0.0) ||
            std::abs(scrollTargetY_ - scrollY_) <= kWheelScrollSnapDistance) {
            scrollAnimatedVelocityPixelsPerSecond_ = 0.0;
        }
    }

    void collectVisibleSets() {
        visibleIndices_.clear();
        preloadIndices_.clear();

        const float viewTop = static_cast<float>(scrollY_);
        const float viewBottom = viewTop + viewportHeight_;
        const float preloadMargin = viewportHeight_ * kPreloadScreens;

        queryRange(viewTop, viewBottom, visibleIndices_);
        queryRange(viewTop - preloadMargin, viewBottom + preloadMargin, preloadIndices_);
    }

    void queryRange(float top, float bottom, std::vector<std::size_t>& output) {
        if (layoutOrder_.empty()) {
            return;
        }

        const float safeTop = std::max(0.0f, top);
        const float safeBottom = std::max(safeTop, bottom);

        bucketIndex_.query(safeTop, safeBottom, [&](std::size_t index) {
            const auto& item = items_[index];
            if (item.y < safeBottom && item.y + item.h > safeTop) {
                output.push_back(index);
            }
        });
    }

    void touchVisibleItems() {
        for (const std::size_t index : visibleIndices_) {
            items_[index].lastTouchedFrame = frameIndex_;
            if (items_[index].cachedPixels != nullptr) {
                items_[index].cachedLastTouchedFrame = frameIndex_;
            }
        }
        if (zoomedIndex_.has_value()) {
            items_[*zoomedIndex_].lastTouchedFrame = frameIndex_;
            if (items_[*zoomedIndex_].cachedPixels != nullptr) {
                items_[*zoomedIndex_].cachedLastTouchedFrame = frameIndex_;
            }
        }
    }

    void collectDecodedImages() {
        const bool fastScroll = isFastScroll();
        const float viewTop = static_cast<float>(scrollY_);
        const float viewBottom = viewTop + viewportHeight_;
        const float preloadMargin = viewportHeight_ * (fastScroll ? kFastScrollPreloadBandScreens : kPreloadScreens);
        const int pumpBudget = fastScroll
            ? kFastScrollMaxDecodedPumpPerFrame
            : (lastFrameDeltaSeconds_ > (1.0f / 120.0f) ? 4 : std::min(kMaxDecodedPumpPerFrame, 12));
        lastDecodedPumpCount_ = 0;
        for (int count = 0; count < pumpBudget; ++count) {
            DecodedImage decoded;
            if (!completedQueue_.tryPop(decoded)) {
                break;
            }
            const std::size_t index = decoded.index;
            const bool zoomPriority = zoomedIndex_.has_value() && *zoomedIndex_ == index;
            if (!zoomPriority && !intersectsRange(index, viewTop - preloadMargin, viewBottom + preloadMargin)) {
                if (!fastScroll && shouldKeepCpuStaticCache() && index < items_.size() && shouldRetainCpuStaticCache(decoded)) {
                    adoptImageCache(items_[index], decoded);
                }
                clearPendingDecode(index, decoded.replaceResidentTexture);
            } else {
                readyUploads_.push_back(std::move(decoded));
            }
            ++lastDecodedPumpCount_;
        }

        for (int count = 0; count < 2; ++count) {
            DecodedImage decoded;
            if (!completedZoomQueue_.tryPop(decoded)) {
                break;
            }
            readyZoomUploads_.push_back(std::move(decoded));
        }
    }

    int computeUploadBudget() const {
        if (isFastScroll()) {
            return kFastScrollMaxUploadsPerFrame;
        }
        const std::size_t backlog =
            readyUploads_.size() +
            completedQueue_.size() +
            cachedUploadIndices_.size();

        int budget = kMaxUploadsPerFrame;
        if (backlog > 24) {
            budget += 4;
        }
        if (backlog > 96) {
            budget += 6;
        }
        if (gpuTextureBytes_ + (256ULL * 1024ULL * 1024ULL) < kMaxGpuTextureBytes) {
            budget += 4;
        }
        return std::clamp(budget, 4, 20);
    }

    void processCachedUploads(
        float viewTop,
        float viewBottom,
        float preloadMargin,
        float uploadMargin,
        int uploadBudget,
        bool fastScroll,
        int& uploadsThisFrame) {
        const std::size_t scanBudget = std::min<std::size_t>(
            cachedUploadIndices_.size(),
            static_cast<std::size_t>(std::max(64, uploadBudget * 24)));

        for (std::size_t scanned = 0; scanned < scanBudget && !cachedUploadIndices_.empty();) {
            const std::size_t index = cachedUploadIndices_.front();
            cachedUploadIndices_.pop_front();
            ++scanned;

            if (index >= items_.size() || items_[index].removed) {
                discardPendingUpload(index);
                continue;
            }

            auto& item = items_[index];
            const bool zoomPriority = zoomedIndex_.has_value() && *zoomedIndex_ == index;
            if (item.texture != 0 || item.state != LoadState::PendingUpload ||
                item.cachedPixels == nullptr || item.cachedWidth <= 0 || item.cachedHeight <= 0) {
                discardPendingUpload(index);
                continue;
            }

            if (!zoomPriority && !intersectsRange(index, viewTop - preloadMargin, viewBottom + preloadMargin)) {
                discardPendingUpload(index);
                continue;
            }

            if (!zoomPriority && (uploadsThisFrame >= uploadBudget ||
                !intersectsRange(index, viewTop - uploadMargin, viewBottom + uploadMargin))) {
                if (fastScroll) {
                    discardPendingUpload(index);
                } else {
                    cachedUploadIndices_.push_back(index);
                }
                continue;
            }

            const bool useMipmaps = item.kind == MediaKind::StaticImage && g_glGenerateMipmap != nullptr;
            GLuint texture = createTexture(item.cachedWidth, item.cachedHeight, item.cachedPixels, useMipmaps);
            if (texture == 0) {
                discardPendingUpload(index);
                clearImageCache(item);
                continue;
            }

            if (!promotePendingUpload(index)) {
                glDeleteTextures(1, &texture);
                continue;
            }

            item.texture = texture;
            item.textureWidth = item.cachedWidth;
            item.textureHeight = item.cachedHeight;
            item.textureHasMipmaps = useMipmaps;
            item.textureBytes = TextureByteSize(item.textureWidth, item.textureHeight, item.textureHasMipmaps);
            item.lastTouchedFrame = frameIndex_;
            item.residentSlot = residentIndices_.size();
            item.galleryTextureMaxDimension = MaxDimension(item.textureWidth, item.textureHeight);
            clearAnimationState(item);
            item.cachedLastTouchedFrame = frameIndex_;
            residentIndices_.push_back(index);
            gpuTextureBytes_ += item.textureBytes;

            ++uploadsThisFrame;
        }
    }

    void processReadyZoomUploads() {
        while (!readyZoomUploads_.empty()) {
            DecodedImage decoded = std::move(readyZoomUploads_.back());
            readyZoomUploads_.pop_back();

            if (!zoomedIndex_.has_value() ||
                decoded.index != *zoomedIndex_ ||
                decoded.kind != MediaKind::StaticImage ||
                decoded.mode != DecodeMode::Zoom) {
                continue;
            }

            GLuint texture = createTexture(decoded.width, decoded.height, decoded.pixels, false);
            if (texture == 0) {
                continue;
            }

            releaseZoomTexture();
            zoomTexture_ = texture;
            zoomTextureIndex_ = decoded.index;
            zoomTextureWidth_ = decoded.width;
            zoomTextureHeight_ = decoded.height;
            zoomTextureBytes_ = TextureByteSize(zoomTextureWidth_, zoomTextureHeight_, false);
            gpuTextureBytes_ += zoomTextureBytes_;
            break;
        }
    }

    void processReadyUploads() {
        const bool fastScroll = isFastScroll();
        const float viewTop = static_cast<float>(scrollY_);
        const float viewBottom = viewTop + viewportHeight_;
        const float preloadMargin = viewportHeight_ * (fastScroll ? kFastScrollPreloadBandScreens : kPreloadScreens);
        const float uploadMargin = viewportHeight_ * (fastScroll ? kFastScrollActiveBandScreens : kUploadScreens);
        const int uploadBudget = computeUploadBudget();
        lastUploadBudget_ = uploadBudget;
        int uploadsThisFrame = 0;

        processReadyZoomUploads();

        processCachedUploads(viewTop, viewBottom, preloadMargin, uploadMargin, uploadBudget, fastScroll, uploadsThisFrame);

        const std::size_t scanBudget = std::min<std::size_t>(
            readyUploads_.size(),
            static_cast<std::size_t>(fastScroll ? 32 : std::max(96, uploadBudget * 20)));

        for (std::size_t scanned = 0; scanned < scanBudget && !readyUploads_.empty(); ++scanned) {
            DecodedImage decoded = std::move(readyUploads_.front());
            readyUploads_.pop_front();
            const std::size_t index = decoded.index;
            if (index >= items_.size() || items_[index].removed) {
                clearPendingDecode(index, decoded.replaceResidentTexture);
                continue;
            }
            const bool zoomPriority = zoomedIndex_.has_value() && *zoomedIndex_ == index;

            if (!zoomPriority && !intersectsRange(index, viewTop - preloadMargin, viewBottom + preloadMargin)) {
                if (!fastScroll && shouldKeepCpuStaticCache() && shouldRetainCpuStaticCache(decoded)) {
                    adoptImageCache(items_[index], decoded);
                }
                clearPendingDecode(index, decoded.replaceResidentTexture);
                continue;
            }

            if (!zoomPriority && (uploadsThisFrame >= uploadBudget ||
                !intersectsRange(index, viewTop - uploadMargin, viewBottom + uploadMargin))) {
                const bool inUploadBand = intersectsRange(index, viewTop - uploadMargin, viewBottom + uploadMargin);
                if (decoded.replaceResidentTexture) {
                    readyUploads_.push_back(std::move(decoded));
                } else if (!fastScroll && shouldKeepCpuStaticCache() && shouldRetainCpuStaticCache(decoded)) {
                    adoptImageCache(items_[index], decoded);
                    clearPendingDecode(index, false);
                } else if (fastScroll || (!inUploadBand && !shouldKeepCpuStaticCache() && decoded.kind == MediaKind::StaticImage)) {
                    clearPendingDecode(index, false);
                } else {
                    readyUploads_.push_back(std::move(decoded));
                }
                continue;
            }

            GLuint texture = createTexture(decoded);
            if (texture == 0) {
                clearPendingDecode(index, decoded.replaceResidentTexture);
                continue;
            }

            auto& item = items_[index];
            const bool useMipmaps = ShouldUseMipmapsForDecodedImage(decoded) && g_glGenerateMipmap != nullptr;
            if (decoded.replaceResidentTexture) {
                if (item.texture == 0) {
                    glDeleteTextures(1, &texture);
                    clearPendingDecode(index, true);
                    continue;
                }

                const std::size_t previousBytes = item.textureBytes;
                glDeleteTextures(1, &item.texture);
                gpuTextureBytes_ = gpuTextureBytes_ >= previousBytes ? gpuTextureBytes_ - previousBytes : 0;
                item.texture = texture;
                item.textureWidth = decoded.width;
                item.textureHeight = decoded.height;
                item.textureHasMipmaps = useMipmaps;
                item.textureBytes = TextureByteSize(item.textureWidth, item.textureHeight, item.textureHasMipmaps);
                item.galleryTextureMaxDimension = MaxDimension(item.textureWidth, item.textureHeight);
                item.lastTouchedFrame = frameIndex_;
                clearAnimationState(item);
                if (shouldRetainCpuStaticCache(decoded)) {
                    adoptImageCache(item, decoded);
                } else {
                    clearImageCache(item);
                }
                clearGalleryUpgrade(item);
                gpuTextureBytes_ += item.textureBytes;
                ++uploadsThisFrame;
                continue;
            }

            if (!promotePendingUpload(index)) {
                glDeleteTextures(1, &texture);
                continue;
            }

            item.texture = texture;
            item.textureWidth = decoded.width;
            item.textureHeight = decoded.height;
            item.textureHasMipmaps = useMipmaps;
            item.textureBytes = TextureByteSize(item.textureWidth, item.textureHeight, item.textureHasMipmaps);
            item.lastTouchedFrame = frameIndex_;
            item.residentSlot = residentIndices_.size();
            item.galleryTextureMaxDimension = MaxDimension(item.textureWidth, item.textureHeight);
            if ((decoded.kind == MediaKind::AnimatedGif || decoded.kind == MediaKind::Video) && decoded.frameCount > 1) {
                clearImageCache(item);
                const std::size_t totalBytes =
                    static_cast<std::size_t>(decoded.width) *
                    static_cast<std::size_t>(decoded.height) *
                    4U *
                    static_cast<std::size_t>(decoded.frameCount);
                item.animationPixels.assign(decoded.pixels, decoded.pixels + totalBytes);
                item.animationFrameDelaysMs = std::move(decoded.frameDelaysMs);
                item.animationFrameCount = decoded.frameCount;
                item.animationFrameIndex = 0;
                item.animationFrameClockSeconds = 0.0;
            } else {
                clearAnimationState(item);
                if (shouldKeepCpuStaticCache() && shouldRetainCpuStaticCache(decoded)) {
                    adoptImageCache(item, decoded);
                } else {
                    clearImageCache(item);
                }
            }
            residentIndices_.push_back(index);
            gpuTextureBytes_ += item.textureBytes;

            ++uploadsThisFrame;
        }

        evictImageCaches();
        lastUploadsThisFrame_ = uploadsThisFrame;
    }

    void evictTextures() {
        const float viewTop = static_cast<float>(scrollY_);
        const float viewBottom = viewTop + viewportHeight_;
        const float evictMargin = viewportHeight_ * kEvictScreens;
        const bool overBudget = gpuTextureBytes_ > kMaxGpuTextureBytes || residentIndices_.size() > kMaxActiveTexturesSafety;

        if (overBudget) {
            for (std::size_t i = 0; i < residentIndices_.size();) {
                const std::size_t index = residentIndices_[i];
                if (zoomedIndex_.has_value() && *zoomedIndex_ == index) {
                    ++i;
                    continue;
                }

                if (!intersectsRange(index, viewTop - evictMargin, viewBottom + evictMargin)) {
                    evictTexture(index);
                    continue;
                }
                ++i;
            }
        }

        while (residentIndices_.size() > kMaxActiveTexturesSafety || gpuTextureBytes_ > kMaxGpuTextureBytes) {
            std::size_t oldestIndex = std::numeric_limits<std::size_t>::max();
            std::uint64_t oldestFrame = std::numeric_limits<std::uint64_t>::max();

            for (const std::size_t residentIndex : residentIndices_) {
                if (zoomedIndex_.has_value() && *zoomedIndex_ == residentIndex) {
                    continue;
                }
                if (items_[residentIndex].lastTouchedFrame < oldestFrame) {
                    oldestFrame = items_[residentIndex].lastTouchedFrame;
                    oldestIndex = residentIndex;
                }
            }

            if (oldestIndex == std::numeric_limits<std::size_t>::max()) {
                break;
            }
            evictTexture(oldestIndex);
        }
    }

    void evictTexture(std::size_t index) {
        auto& item = items_[index];
        if (item.texture == 0) {
            return;
        }

        gpuTextureBytes_ = gpuTextureBytes_ >= item.textureBytes ? gpuTextureBytes_ - item.textureBytes : 0;
        glDeleteTextures(1, &item.texture);
        item.texture = 0;
        item.textureWidth = 0;
        item.textureHeight = 0;
        item.textureBytes = 0;
        item.textureHasMipmaps = false;
        item.galleryTextureMaxDimension = 0;
        clearGalleryUpgrade(item);
        clearAnimationState(item);

        const std::size_t slot = item.residentSlot;
        const std::size_t lastIndex = residentIndices_.back();
        residentIndices_[slot] = lastIndex;
        items_[lastIndex].residentSlot = slot;
        residentIndices_.pop_back();
        item.residentSlot = std::numeric_limits<std::size_t>::max();

        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            if (item.state == LoadState::Resident) {
                item.state = LoadState::Empty;
            }
        }
    }

    void requestVisibleDecodes() {
        lastQueuedThisFrame_ = 0;
        if (zoomedIndex_.has_value()) {
            if (queueDecode(*zoomedIndex_, true)) {
                ++lastQueuedThisFrame_;
            }
        }

        const bool fastScroll = isFastScroll();
        if (fastScroll) {
            for (const DecodeJob& droppedJob : decodeQueue_.trimToSize(kFastScrollQueueTrimSize)) {
                discardQueuedJob(droppedJob);
            }
        }
        const std::size_t inflightLimit = fastScroll ? kFastScrollInflightDecodes : kMaxInflightDecodes;
        const int queueBudgetLimit = fastScroll ? kFastScrollMaxQueuedDecodesPerFrame : kMaxQueuedDecodesPerFrame;
        const std::size_t inflightBacklog =
            decodeQueue_.size() +
            completedQueue_.size() +
            readyUploads_.size();
        if (inflightBacklog >= inflightLimit) {
            return;
        }

        const int queueBudget = std::max(
            0,
            std::min<int>(
                queueBudgetLimit,
                static_cast<int>(inflightLimit - inflightBacklog)));
        int queuedThisFrame = 0;
        queueCenteredVisibleDecodes(queuedThisFrame, queueBudget);
        lastQueuedThisFrame_ += queuedThisFrame;

        if (fastScroll) {
            return;
        }

        for (const std::size_t index : preloadIndices_) {
            if (queuedThisFrame >= queueBudget) {
                break;
            }
            if (queueDecode(index, false)) {
                ++queuedThisFrame;
                ++lastQueuedThisFrame_;
            }
        }
    }

    void queueCenteredVisibleDecodes(int& queuedThisFrame, int queueBudget) {
        if (visibleIndices_.empty() || queuedThisFrame >= queueBudget) {
            return;
        }

        const float centerY = static_cast<float>(scrollY_) + viewportHeight_ * 0.5f;
        const auto compareCenter = [&](std::size_t index, float value) {
            return itemCenterY(index) < value;
        };

        std::size_t right = static_cast<std::size_t>(std::lower_bound(
            visibleIndices_.begin(),
            visibleIndices_.end(),
            centerY,
            compareCenter) - visibleIndices_.begin());
        std::size_t left = right;

        while (queuedThisFrame < queueBudget && (left > 0 || right < visibleIndices_.size())) {
            const bool hasLeft = left > 0;
            const bool hasRight = right < visibleIndices_.size();

            std::size_t selectedIndex = std::numeric_limits<std::size_t>::max();
            if (hasLeft && hasRight) {
                const std::size_t leftCandidate = visibleIndices_[left - 1];
                const std::size_t rightCandidate = visibleIndices_[right];
                const float leftDistance = std::abs(itemCenterY(leftCandidate) - centerY);
                const float rightDistance = std::abs(itemCenterY(rightCandidate) - centerY);
                if (leftDistance <= rightDistance) {
                    selectedIndex = leftCandidate;
                    --left;
                } else {
                    selectedIndex = rightCandidate;
                    ++right;
                }
            } else if (hasLeft) {
                selectedIndex = visibleIndices_[left - 1];
                --left;
            } else {
                selectedIndex = visibleIndices_[right];
                ++right;
            }

            if (queueDecode(selectedIndex, true)) {
                ++queuedThisFrame;
            }
        }
    }

    bool markQueued(std::size_t index) {
        std::lock_guard<std::mutex> lock(stateMutex_);
        auto& item = items_[index];
        if (item.removed || item.state != LoadState::Empty) {
            return false;
        }
        item.state = LoadState::Queued;
        return true;
    }

    bool queueCachedUpload(std::size_t index, bool highPriority, int desiredMaxDimension, bool allowLowerQuality) {
        if (index >= items_.size()) {
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            auto& item = items_[index];
            if (item.removed ||
                item.texture != 0 ||
                item.state != LoadState::Empty ||
                item.cachedPixels == nullptr ||
                item.cachedWidth <= 0 ||
                item.cachedHeight <= 0 ||
                !cachedGalleryImageSatisfies(item, desiredMaxDimension, allowLowerQuality)) {
                return false;
            }
            item.state = LoadState::PendingUpload;
        }

        if (highPriority) {
            cachedUploadIndices_.push_front(index);
        } else {
            cachedUploadIndices_.push_back(index);
        }
        return true;
    }

    bool queueDecode(std::size_t index, bool highPriority) {
        if (index >= items_.size()) {
            return false;
        }
        const bool fastScroll = isFastScroll();
        const int desiredMaxDimension = galleryDecodeMaxDimensionFor(index);
        if (queueCachedUpload(index, highPriority, desiredMaxDimension, fastScroll)) {
            return true;
        }

        bool replaceResidentTexture = false;
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            auto& item = items_[index];
            if (item.removed) {
                return false;
            }
            if (item.texture != 0) {
                if (!canQueueGalleryUpgrade(item, desiredMaxDimension, fastScroll)) {
                    return false;
                }
                item.galleryUpgradePending = true;
                item.galleryUpgradeTargetDimension = desiredMaxDimension;
                replaceResidentTexture = true;
            } else if (item.state == LoadState::Empty) {
                item.state = LoadState::Queued;
            } else {
                return false;
            }
        }

        decodeQueue_.push(
            DecodeJob{
                index,
                DecodeMode::Gallery,
                desiredMaxDimension,
                fastScroll ? scrollRevision_.load(std::memory_order_relaxed) : 0,
                fastScroll && !replaceResidentTexture,
                replaceResidentTexture},
            highPriority);
        return true;
    }

    bool beginDecode(std::size_t index) {
        std::lock_guard<std::mutex> lock(stateMutex_);
        auto& item = items_[index];
        if (item.removed || item.state != LoadState::Queued) {
            return false;
        }
        item.state = LoadState::Decoding;
        return true;
    }

    void finishDecode(std::size_t index, bool success) {
        std::lock_guard<std::mutex> lock(stateMutex_);
        auto& item = items_[index];
        if (item.state != LoadState::Decoding) {
            return;
        }
        item.state = item.removed ? LoadState::Empty : (success ? LoadState::PendingUpload : LoadState::Empty);
    }

    void discardQueuedJob(const DecodeJob& job) {
        const std::size_t index = job.index;
        if (index >= items_.size()) {
            return;
        }
        std::lock_guard<std::mutex> lock(stateMutex_);
        auto& item = items_[index];
        if (job.replaceResidentTexture) {
            clearGalleryUpgrade(item);
            return;
        }
        if (!item.removed && item.state == LoadState::Queued) {
            item.state = LoadState::Empty;
        }
    }

    bool promotePendingUpload(std::size_t index) {
        std::lock_guard<std::mutex> lock(stateMutex_);
        auto& item = items_[index];
        if (item.removed || item.state != LoadState::PendingUpload) {
            return false;
        }
        item.state = LoadState::Resident;
        return true;
    }

    void discardPendingUpload(std::size_t index) {
        if (index >= items_.size()) {
            return;
        }
        std::lock_guard<std::mutex> lock(stateMutex_);
        auto& item = items_[index];
        if (item.removed || item.state == LoadState::PendingUpload) {
            item.state = LoadState::Empty;
        }
    }

    bool intersectsRange(std::size_t index, float top, float bottom) const {
        const auto& item = items_[index];
        return item.y < bottom && item.y + item.h > top;
    }

    float itemCenterY(std::size_t index) const {
        const auto& item = items_[index];
        return item.y + (item.h * 0.5f);
    }

    bool isFastScroll() const {
        return !zoomedIndex_.has_value() &&
            lastScrollVelocityPixelsPerSecond_ >= kFastScrollVelocityPixelsPerSecond;
    }

    bool isActivelyScrolling(const ImGuiIO& io) const {
        return !zoomedIndex_.has_value() &&
            (scrollbarDragging_ ||
             std::abs(io.MouseWheel) > 0.001f ||
             ImGui::IsKeyDown(ImGuiKey_DownArrow) ||
             ImGui::IsKeyDown(ImGuiKey_UpArrow) ||
             ImGui::IsKeyPressed(ImGuiKey_PageDown, false) ||
             ImGui::IsKeyPressed(ImGuiKey_PageUp, false) ||
             ImGui::IsKeyPressed(ImGuiKey_Home, false) ||
             ImGui::IsKeyPressed(ImGuiKey_End, false) ||
             lastScrollVelocityPixelsPerSecond_ >= kInteractiveScrollBoostVelocityPixelsPerSecond);
    }

    bool shouldKeepCpuStaticCache() const {
        return true;
    }

    void updateDecodeConcurrencyLimit(const ImGuiIO& io) {
        (void)io;
        const int target = std::max(1, static_cast<int>(workerCount_));

        const int previous = galleryDecodeConcurrencyLimit_.exchange(target, std::memory_order_relaxed);
        if (previous != target) {
            workerThrottleCondition_.notify_all();
        }
    }

    void acquireGalleryDecodeSlot() {
        std::unique_lock<std::mutex> lock(workerThrottleMutex_);
        workerThrottleCondition_.wait(lock, [this] {
            return galleryDecodeSlotsInUse_ < galleryDecodeConcurrencyLimit_.load(std::memory_order_relaxed);
        });
        ++galleryDecodeSlotsInUse_;
    }

    void releaseGalleryDecodeSlot() {
        std::lock_guard<std::mutex> lock(workerThrottleMutex_);
        if (galleryDecodeSlotsInUse_ > 0) {
            --galleryDecodeSlotsInUse_;
        }
        workerThrottleCondition_.notify_one();
    }

    bool shouldPreferGpuMinification(
        const DecodeJob& job,
        const DecodedImage& decoded,
        int decodeLimit) const {
        if (decoded.pixels == nullptr ||
            decoded.kind != MediaKind::StaticImage ||
            decoded.frameCount > 1 ||
            job.mode != DecodeMode::Gallery ||
            decoded.width <= 0 ||
            decoded.height <= 0 ||
            (decoded.width <= decodeLimit && decoded.height <= decodeLimit) ||
            decoded.width > maxTextureSize_ ||
            decoded.height > maxTextureSize_) {
            return false;
        }

        const int sourceMaxDimension = MaxDimension(decoded.width, decoded.height);
        const std::size_t sourceBytes = TextureByteSize(decoded.width, decoded.height);
        if (sourceMaxDimension > kGpuScaleEligibleDimensionMax ||
            sourceBytes > kGpuScaleEligiblePixelBytesMax) {
            return false;
        }

        const float ratioLimit = job.discardIfStale
            ? kFastScrollGpuScaleEligibleRatioMax
            : kGpuScaleEligibleRatioMax;
        return static_cast<float>(sourceMaxDimension) <=
            static_cast<float>(std::max(1, decodeLimit)) * ratioLimit;
    }

    bool shouldRetainCpuStaticCache(const DecodedImage& decoded) const {
        return decoded.kind == MediaKind::StaticImage &&
            decoded.frameCount <= 1 &&
            decoded.width > 0 &&
            decoded.height > 0 &&
            MaxDimension(decoded.width, decoded.height) <= kCpuCacheRetainDimensionMax &&
            TextureByteSize(decoded.width, decoded.height) <= kCpuCacheRetainPixelBytesMax;
    }

#if defined(_WIN32)
    void captureSchedulingDefaults() {
        const int threadPriority = GetThreadPriority(GetCurrentThread());
        if (threadPriority != THREAD_PRIORITY_ERROR_RETURN) {
            uiThreadRestorePriority_ = threadPriority;
        }

        DWORD_PTR processMask = 0;
        DWORD_PTR systemMask = 0;
        if (!GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask) || processMask == 0) {
            return;
        }

        processAffinityRestoreMask_ = processMask;
        uiThreadAffinityMask_ = LowestSetBit(processMask);
        if (uiThreadAffinityMask_ == 0) {
            uiThreadAffinityMask_ = processMask;
        }

        workerThreadAffinityMask_ = processMask & ~uiThreadAffinityMask_;
        uiCoreIsolationActive_ = workerThreadAffinityMask_ != 0;
        if (!uiCoreIsolationActive_) {
            workerThreadAffinityMask_ = processMask;
        }

        uiThreadPreferredProcessor_ = FirstProcessorIndexFromMask(uiThreadAffinityMask_);
        workerIdealProcessors_ = ProcessorIndicesFromMask(workerThreadAffinityMask_);
    }

    void configureUiThreadScheduling() {
        const HANDLE thread = GetCurrentThread();
        if (uiThreadPreferredProcessor_ >= 0) {
            SetThreadIdealProcessor(thread, static_cast<DWORD>(uiThreadPreferredProcessor_));
        }
        SetThreadPriority(thread, kUiThreadBasePriority);
    }

    void restoreSchedulingDefaults() {
        applySchedulingProfile(false);
        if (processAffinityRestoreMask_ != 0) {
            SetThreadAffinityMask(GetCurrentThread(), processAffinityRestoreMask_);
        }
        SetThreadPriority(GetCurrentThread(), uiThreadRestorePriority_);
    }

    void configureDecodeWorkerThread(std::size_t workerIndex) {
        HANDLE thread = GetCurrentThread();
        const DWORD_PTR workerAffinityMask =
            workerThreadAffinityMask_ != 0 ? workerThreadAffinityMask_ : processAffinityRestoreMask_;
        if (workerAffinityMask != 0) {
            SetThreadAffinityMask(thread, workerAffinityMask);
        }
        if (!workerIdealProcessors_.empty()) {
            const DWORD idealProcessor =
                workerIdealProcessors_[workerIndex % workerIdealProcessors_.size()];
            SetThreadIdealProcessor(thread, idealProcessor);
        }
        SetThreadPriority(thread, THREAD_PRIORITY_BELOW_NORMAL);
    }

    void updateSchedulingProfile(const ImGuiIO& io) {
        applySchedulingProfile(isActivelyScrolling(io));
    }

    void applySchedulingProfile(bool interactiveBoost) {
        if (interactiveBoost == schedulingBoostActive_) {
            return;
        }

        schedulingBoostActive_ = interactiveBoost;
        const int targetThreadPriority =
            interactiveBoost ? kUiThreadActivePriority : kUiThreadBasePriority;
        SetThreadPriority(GetCurrentThread(), targetThreadPriority);
    }
#endif

    bool cachedGalleryImageSatisfies(
        const ImageRecord& item,
        int desiredMaxDimension,
        bool allowLowerQuality) const {
        if (item.cachedPixels == nullptr || item.cachedWidth <= 0 || item.cachedHeight <= 0) {
            return false;
        }
        return allowLowerQuality ||
            MaxDimension(item.cachedWidth, item.cachedHeight) + kGalleryUpgradeSlackPixels >= desiredMaxDimension;
    }

    bool residentGalleryTextureSatisfies(const ImageRecord& item, int desiredMaxDimension) const {
        if (item.texture == 0 || item.textureWidth <= 0 || item.textureHeight <= 0) {
            return false;
        }
        return MaxDimension(item.textureWidth, item.textureHeight) + kGalleryUpgradeSlackPixels >= desiredMaxDimension;
    }

    bool canQueueGalleryUpgrade(
        const ImageRecord& item,
        int desiredMaxDimension,
        bool fastScroll) const {
        return item.kind == MediaKind::StaticImage &&
            !fastScroll &&
            item.state == LoadState::Resident &&
            !item.galleryUpgradePending &&
            !residentGalleryTextureSatisfies(item, desiredMaxDimension);
    }

    void clearGalleryUpgrade(ImageRecord& item) {
        item.galleryUpgradePending = false;
        item.galleryUpgradeTargetDimension = 0;
    }

    void clearPendingDecode(std::size_t index, bool replaceResidentTexture) {
        if (index >= items_.size()) {
            return;
        }
        if (replaceResidentTexture) {
            std::lock_guard<std::mutex> lock(stateMutex_);
            clearGalleryUpgrade(items_[index]);
        } else {
            discardPendingUpload(index);
        }
    }

    void workerLoop() {
        DecodeJob job;
        while (decodeQueue_.waitPop(job)) {
            const bool galleryDecode = job.mode == DecodeMode::Gallery;
            const bool throttledGalleryDecode = galleryDecode && !job.replaceResidentTexture;
            if (galleryDecode && !job.replaceResidentTexture && !beginDecode(job.index)) {
                continue;
            }
            if (galleryDecode && job.replaceResidentTexture) {
                std::lock_guard<std::mutex> lock(stateMutex_);
                if (job.index >= items_.size() ||
                    items_[job.index].removed ||
                    !items_[job.index].galleryUpgradePending ||
                    items_[job.index].texture == 0) {
                    continue;
                }
            }
            if (job.index >= items_.size()) {
                continue;
            }

            if (throttledGalleryDecode) {
                acquireGalleryDecodeSlot();
            }
            activeWorkers_.fetch_add(1);
            DecodedImage decoded = DecodeGalleryFile(
                job.index,
                items_[job.index].path,
                items_[job.index].kind,
                items_[job.index].sourceWidth,
                items_[job.index].sourceHeight,
                items_[job.index].sourceFrameRate);
            decoded.mode = job.mode;
            decoded.requestedMaxDimension = job.maxDimension;
            decoded.replaceResidentTexture = job.replaceResidentTexture;
            const int decodeLimit = std::max(1, job.maxDimension > 0 ? job.maxDimension : maxTextureSize_);
            const bool preferGpuMinification = shouldPreferGpuMinification(job, decoded, decodeLimit);
            if (decoded.pixels != nullptr &&
                !preferGpuMinification &&
                !DownscaleToFit(decoded, decodeLimit)) {
                decoded.reset();
            }
            activeWorkers_.fetch_sub(1);
            if (throttledGalleryDecode) {
                releaseGalleryDecodeSlot();
            }

            const bool staleGalleryDecode =
                galleryDecode &&
                !job.replaceResidentTexture &&
                job.discardIfStale &&
                job.scrollRevision != scrollRevision_.load(std::memory_order_relaxed);

            if (galleryDecode && !job.replaceResidentTexture) {
                finishDecode(job.index, decoded.pixels != nullptr && !staleGalleryDecode);
            } else if (galleryDecode && decoded.pixels == nullptr) {
                clearPendingDecode(job.index, true);
            }
            if (decoded.pixels != nullptr) {
                if (staleGalleryDecode) {
                    decoded.reset();
                } else if (galleryDecode) {
                    completedQueue_.push(std::move(decoded));
                } else {
                    completedZoomQueue_.push(std::move(decoded));
                }
            }
        }
    }

    std::vector<ImageRecord> items_;
    std::size_t workerCount_ = 1;
    int maxTextureSize_ = 16384;

    float viewportWidth_ = 1920.0f;
    float viewportHeight_ = 1080.0f;
    float contentHeight_ = 1080.0f;
    float lastFrameDeltaSeconds_ = 0.0f;
    double scrollY_ = 0.0;
    double scrollTargetY_ = 0.0;
    double scrollAnimatedVelocityPixelsPerSecond_ = 0.0;
    double lastScrollVelocityPixelsPerSecond_ = 0.0;
    double lastScrollVelocitySampleY_ = 0.0;
    double lastScrollRevisionY_ = 0.0;
    int currentColumnCount_ = kDefaultColumnCount;
    std::uint64_t frameIndex_ = 0;
    std::atomic<std::uint64_t> scrollRevision_{1};

    mutable std::mutex stateMutex_;
    DecodeRequestQueue decodeQueue_;
    ThreadSafeQueue<DecodedImage> completedQueue_;
    ThreadSafeQueue<DecodedImage> completedZoomQueue_;
    std::deque<DecodedImage> readyUploads_;
    std::deque<DecodedImage> readyZoomUploads_;
    std::deque<std::size_t> cachedUploadIndices_;
    std::vector<std::thread> workers_;
    std::atomic<int> activeWorkers_{0};
    std::mutex workerThrottleMutex_;
    std::condition_variable workerThrottleCondition_;
    std::atomic<int> galleryDecodeConcurrencyLimit_{1};
    int galleryDecodeSlotsInUse_ = 0;

    VerticalBucketIndex bucketIndex_;
    std::vector<std::size_t> layoutOrder_;
    std::vector<std::size_t> visibleIndices_;
    std::vector<std::size_t> preloadIndices_;
    std::vector<std::size_t> residentIndices_;
    std::optional<int> manualColumnCount_ = kDefaultColumnCount;
    std::optional<std::size_t> hoveredIndex_;
    std::optional<std::size_t> zoomedIndex_;
    std::optional<std::size_t> zoomTextureIndex_;
    GLuint zoomTexture_ = 0;
    int zoomTextureWidth_ = 0;
    int zoomTextureHeight_ = 0;
    std::size_t zoomTextureBytes_ = 0;
    float zoomScale_ = 1.0f;
    ImVec2 zoomPanOffset_ = ImVec2(0.0f, 0.0f);
    ImVec2 zoomDragOriginMouse_ = ImVec2(0.0f, 0.0f);
    ImVec2 zoomDragOriginPan_ = ImVec2(0.0f, 0.0f);
    bool zoomDragPending_ = false;
    bool zoomDragging_ = false;
    bool scrollbarDragging_ = false;
    float scrollbarDragOffsetY_ = 0.0f;
    bool suppressZoomClose_ = false;
    bool overlayExpanded_ = false;
    std::string clipboardStatus_;
    double clipboardStatusExpiry_ = 0.0;
    std::array<TextureUploadPbo, kTextureUploadPboCount> textureUploadPbos_{};
    std::size_t nextTextureUploadPbo_ = 0;
    bool textureUploadPbosInitialized_ = false;
    std::size_t gpuTextureBytes_ = 0;
    std::size_t cpuImageCacheBytes_ = 0;
    bool perfTraceEnabled_ = false;
    std::ofstream perfTraceFile_;
#if defined(_WIN32)
    int uiThreadRestorePriority_ = THREAD_PRIORITY_NORMAL;
    DWORD_PTR processAffinityRestoreMask_ = 0;
    DWORD_PTR uiThreadAffinityMask_ = 0;
    DWORD_PTR workerThreadAffinityMask_ = 0;
    std::vector<DWORD> workerIdealProcessors_;
    int uiThreadPreferredProcessor_ = -1;
    bool uiCoreIsolationActive_ = false;
    bool schedulingBoostActive_ = false;
#endif
    int lastDecodedPumpCount_ = 0;
    int lastUploadBudget_ = 0;
    int lastUploadsThisFrame_ = 0;
    int lastQueuedThisFrame_ = 0;
};

void GLFWErrorCallback(int error, const char* description) {
    std::cerr << "GLFW error " << error << ": " << description << '\n';
}

fs::path ResolveRootDirectory(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == nullptr) {
            continue;
        }
        if (std::strncmp(argv[i], "--", 2) == 0) {
            continue;
        }
        return fs::path(argv[i]);
    }

    std::error_code ec;
    const fs::path currentDir = fs::current_path(ec);
    const fs::path exeDir = ExecutableDirectory();

    if (!currentDir.empty() && DirectoryHintsMediaRoot(currentDir)) {
        return currentDir;
    }

    if (!exeDir.empty() && DirectoryHintsMediaRoot(exeDir)) {
        return exeDir;
    }

    if (!currentDir.empty()) {
        const fs::path imagesDir = currentDir / "images";
        if (fs::exists(imagesDir, ec) && !ec) {
            return imagesDir;
        }
        ec.clear();
    }

    if (!exeDir.empty()) {
        const fs::path imagesDir = exeDir / "images";
        if (fs::exists(imagesDir, ec) && !ec) {
            return imagesDir;
        }
    }

    if (!currentDir.empty()) {
        return currentDir;
    }
    return exeDir;
}

#if defined(_WIN32)
DWORD MakeAccentGradientColor(BYTE alpha, BYTE red, BYTE green, BYTE blue) {
    return (static_cast<DWORD>(alpha) << 24) |
        (static_cast<DWORD>(blue) << 16) |
        (static_cast<DWORD>(green) << 8) |
        static_cast<DWORD>(red);
}

bool TrySetImmersiveDarkMode(HWND hwnd) {
    const BOOL enabled = TRUE;
    if (SUCCEEDED(DwmSetWindowAttribute(hwnd, kDwmwaUseImmersiveDarkMode, &enabled, sizeof(enabled)))) {
        return true;
    }
    return SUCCEEDED(DwmSetWindowAttribute(hwnd, kDwmwaUseImmersiveDarkModeBefore20, &enabled, sizeof(enabled)));
}

void ApplyEmbeddedWindowIcons(HWND hwnd) {
    HMODULE module = GetModuleHandleW(nullptr);
    if (module == nullptr) {
        return;
    }

    const int largeWidth = GetSystemMetrics(SM_CXICON);
    const int largeHeight = GetSystemMetrics(SM_CYICON);
    const int smallWidth = GetSystemMetrics(SM_CXSMICON);
    const int smallHeight = GetSystemMetrics(SM_CYSMICON);

    if (HICON largeIcon = static_cast<HICON>(LoadImageW(
            module,
            MAKEINTRESOURCEW(kAppIconResourceId),
            IMAGE_ICON,
            largeWidth,
            largeHeight,
            LR_DEFAULTCOLOR | LR_SHARED))) {
        SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(largeIcon));
    }

    if (HICON smallIcon = static_cast<HICON>(LoadImageW(
            module,
            MAKEINTRESOURCEW(kAppIconResourceId),
            IMAGE_ICON,
            smallWidth,
            smallHeight,
            LR_DEFAULTCOLOR | LR_SHARED))) {
        SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));
    }
}

bool TryEnableAccentBackdrop(HWND hwnd, int accentState, DWORD gradientColor) {
    enum WindowCompositionAttributeCompat : int {
        WcaAccentPolicy = 19,
    };

    struct AccentPolicyCompat {
        int accentState;
        int accentFlags;
        DWORD gradientColor;
        int animationId;
    };

    struct WindowCompositionAttribDataCompat {
        int attrib;
        PVOID pvData;
        SIZE_T cbData;
    };

    using SetWindowCompositionAttributeProc = BOOL(WINAPI*)(HWND, WindowCompositionAttribDataCompat*);

    static SetWindowCompositionAttributeProc setWindowCompositionAttribute = [] {
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        if (user32 == nullptr) {
            user32 = LoadLibraryW(L"user32.dll");
        }
        if (user32 == nullptr) {
            return static_cast<SetWindowCompositionAttributeProc>(nullptr);
        }
        return reinterpret_cast<SetWindowCompositionAttributeProc>(
            GetProcAddress(user32, "SetWindowCompositionAttribute"));
    }();

    if (setWindowCompositionAttribute == nullptr) {
        return false;
    }

    AccentPolicyCompat accentPolicy{};
    accentPolicy.accentState = accentState;
    accentPolicy.gradientColor = gradientColor;

    WindowCompositionAttribDataCompat data{};
    data.attrib = WcaAccentPolicy;
    data.pvData = &accentPolicy;
    data.cbData = sizeof(accentPolicy);
    return setWindowCompositionAttribute(hwnd, &data) == TRUE;
}

void ApplyWindowsWindowChrome(GLFWwindow* window) {
    HWND hwnd = glfwGetWin32Window(window);
    if (hwnd == nullptr) {
        return;
    }

    ApplyEmbeddedWindowIcons(hwnd);
    TrySetImmersiveDarkMode(hwnd);

    const int roundedCorners = kDwmWindowCornerRound;
    (void)DwmSetWindowAttribute(
        hwnd,
        kDwmwaWindowCornerPreference,
        &roundedCorners,
        sizeof(roundedCorners));

    const int systemBackdrop = kDwmSystemBackdropMainWindow;
    if (SUCCEEDED(DwmSetWindowAttribute(
            hwnd,
            kDwmwaSystemBackdropType,
            &systemBackdrop,
            sizeof(systemBackdrop)))) {
        return;
    }

    if (TryEnableAccentBackdrop(hwnd, kAccentEnableAcrylicBlurBehind, MakeAccentGradientColor(0xD6, 20, 28, 40))) {
        return;
    }

    if (TryEnableAccentBackdrop(hwnd, kAccentEnableBlurBehind, 0)) {
        return;
    }

    DWM_BLURBEHIND blurBehind{};
    blurBehind.dwFlags = DWM_BB_ENABLE;
    blurBehind.fEnable = TRUE;
    (void)DwmEnableBlurBehindWindow(hwnd, &blurBehind);
}
#endif

const char* ResolveGlslVersion() {
#if defined(__APPLE__)
    return "#version 150";
#else
    return "#version 330";
#endif
}

}  // namespace

int main(int argc, char** argv) {
    fs::path rootDirectory = ResolveRootDirectory(argc, argv);
    const bool perfTraceEnabled = HasCommandFlag(argc, argv, "--trace-perf");
    const bool benchScrollEnabled = HasCommandFlag(argc, argv, "--bench-scroll");
    if (HasCommandFlag(argc, argv, "--diagnose-media")) {
        PrintMediaDiagnostics(rootDirectory);
        return 0;
    }
    std::vector<ImageRecord> images = LoadGalleryRecords(rootDirectory);

#if defined(_WIN32)
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool comInitialized = SUCCEEDED(comResult);
#endif

    glfwSetErrorCallback(GLFWErrorCallback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW.\n";
#if defined(_WIN32)
        if (comInitialized) {
            CoUninitialize();
        }
#endif
        return 1;
    }

#if defined(__APPLE__)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* videoMode = primaryMonitor != nullptr ? glfwGetVideoMode(primaryMonitor) : nullptr;
    const int initialWidth = videoMode != nullptr ? videoMode->width : 1920;
    const int initialHeight = videoMode != nullptr ? videoMode->height : 1080;

    GLFWwindow* window = glfwCreateWindow(initialWidth, initialHeight, MakeWindowTitle(rootDirectory).c_str(), nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create a GLFW window.\n";
        glfwTerminate();
#if defined(_WIN32)
        if (comInitialized) {
            CoUninitialize();
        }
#endif
        return 1;
    }

#if defined(_WIN32)
    ApplyWindowsWindowChrome(window);
#endif

    if (primaryMonitor != nullptr) {
        glfwMaximizeWindow(window);
    }

    glfwMakeContextCurrent(window);
    g_glGenerateMipmap = reinterpret_cast<GlGenerateMipmapProc>(glfwGetProcAddress("glGenerateMipmap"));
    g_glGenBuffers = reinterpret_cast<GlGenBuffersProc>(glfwGetProcAddress("glGenBuffers"));
    g_glBindBuffer = reinterpret_cast<GlBindBufferProc>(glfwGetProcAddress("glBindBuffer"));
    g_glBufferData = reinterpret_cast<GlBufferDataProc>(glfwGetProcAddress("glBufferData"));
    g_glBufferSubData = reinterpret_cast<GlBufferSubDataProc>(glfwGetProcAddress("glBufferSubData"));
    g_glMapBufferRange = reinterpret_cast<GlMapBufferRangeProc>(glfwGetProcAddress("glMapBufferRange"));
    g_glUnmapBuffer = reinterpret_cast<GlUnmapBufferProc>(glfwGetProcAddress("glUnmapBuffer"));
    g_glDeleteBuffers = reinterpret_cast<GlDeleteBuffersProc>(glfwGetProcAddress("glDeleteBuffers"));
    bool vsyncEnabled = true;
    glfwSwapInterval(vsyncEnabled ? 1 : 0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize = 0.0f;
    style.WindowRounding = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        std::cerr << "Failed to initialize ImGui GLFW backend.\n";
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
#if defined(_WIN32)
        if (comInitialized) {
            CoUninitialize();
        }
#endif
        return 1;
    }

    if (!ImGui_ImplOpenGL3_Init(ResolveGlslVersion())) {
        std::cerr << "Failed to initialize ImGui OpenGL3 backend.\n";
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
#if defined(_WIN32)
        if (comInitialized) {
            CoUninitialize();
        }
#endif
        return 1;
    }

    int maxTextureSize = 16384;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    const std::size_t workerCount = RecommendedWorkerCount();

    {
        auto app = std::make_unique<GalleryApp>(std::move(images), workerCount, maxTextureSize, perfTraceEnabled);
        const double benchStartSeconds = glfwGetTime();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if (app->hasZoom() && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                const bool deleted = app->deleteZoomedImage();
                app->setClipboardStatus(deleted ? "Image deleted." : "Image delete failed.");
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                if (app->hasZoom()) {
                    app->closeZoom();
                } else {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
            }

            const ImVec2 displaySize = io.DisplaySize;
            app->updateViewport(displaySize);
            const bool leftClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
            const bool leftDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
            const bool leftReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
            const bool rightClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
            const bool scrollbarCaptured = app->handleScrollbar(ImVec2(0.0f, 0.0f), displaySize, io.MousePos, leftClicked, leftDown, leftReleased);

            if (benchScrollEnabled) {
                const double benchElapsed = glfwGetTime() - benchStartSeconds;
                if (benchElapsed < 18.0) {
                    app->setScrollY(app->scrollY() + 4200.0 * io.DeltaTime);
                } else if (benchElapsed < 26.0) {
                    app->setScrollY(app->scrollY() - 2600.0 * io.DeltaTime);
                } else {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
            }

            app->tick(io);

            bool requestOpenFolder = false;
            const bool previousVsync = vsyncEnabled;
            const bool overlayCapturesPointer = app->drawOverlay(rootDirectory, &requestOpenFolder, &vsyncEnabled);
            if (previousVsync != vsyncEnabled) {
                glfwSwapInterval(vsyncEnabled ? 1 : 0);
            }

            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(displaySize, ImGuiCond_Always);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

            const ImGuiWindowFlags canvasFlags =
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoNav |
                ImGuiWindowFlags_NoBackground;

            ImGui::Begin("GalleryCanvas", nullptr, canvasFlags);
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const ImVec2 canvasOrigin = ImGui::GetWindowPos();
            const ImVec2 canvasSize = ImGui::GetWindowSize();
            drawList->PushClipRect(canvasOrigin, canvasOrigin + canvasSize, true);

            std::optional<fs::path> copyPath;
            if (!scrollbarCaptured && !overlayCapturesPointer) {
                copyPath = app->handlePointer(canvasOrigin, io.MousePos, leftClicked, leftDown, leftReleased, rightClicked);
            }
            app->drawGallery(drawList, canvasOrigin, canvasSize);
            app->drawScrollbar(drawList, canvasOrigin, canvasSize);
            drawList->PopClipRect();
            ImGui::End();

            ImGui::PopStyleVar(2);

            if (copyPath.has_value()) {
                const bool copied = CopyImageFileToClipboard(*copyPath);
                app->setClipboardStatus(copied ? "Clipboard copied." : "Clipboard copy failed.");
            }
            app->drawZoomOverlay(displaySize);

            if (requestOpenFolder) {
                if (const auto selectedFolder = PickFolderDialog(rootDirectory)) {
                    rootDirectory = *selectedFolder;
                    glfwSetWindowTitle(window, MakeWindowTitle(rootDirectory).c_str());
                    app = std::make_unique<GalleryApp>(LoadGalleryRecords(rootDirectory), workerCount, maxTextureSize, perfTraceEnabled);
                }
            }

            ImGui::Render();

            int framebufferWidth = 0;
            int framebufferHeight = 0;
            glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
            glViewport(0, 0, framebufferWidth, framebufferHeight);
            glClearColor(0.02f, 0.03f, 0.05f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
        }

        if (!perfTraceEnabled && !benchScrollEnabled) {
            // On real app exit, let process teardown reclaim background decode work
            // instead of blocking the UI on worker joins for stale media tasks.
            (void)app.release();
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
#if defined(_WIN32)
    if (comInitialized) {
        CoUninitialize();
    }
#endif
    return 0;
}
