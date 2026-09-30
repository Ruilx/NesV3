#pragma once

#include <QtGlobal>

#include <array>
#include <memory>
#include <vector>

enum class AudioMixBus : quint8 {
    NativePulse,
    NativeTriangle,
    NativeNoise,
    NativeDpcm,
    Expansion,
};

class AudioMixContext final {
public:
    void add(AudioMixBus bus, float value);
    [[nodiscard]] float value(AudioMixBus bus) const;
    void clear();

private:
    static constexpr qsizetype BusCount = 5;
    std::array<float, BusCount> values = {};
};

class AudioMixPipeline {
public:
    virtual ~AudioMixPipeline() = default;

    [[nodiscard]] virtual float render(const AudioMixContext &context) const = 0;
};

class ApuMixer final {
public:
    using PipelineHandle = quint64;

    ApuMixer();

    PipelineHandle registerPipeline(std::unique_ptr<AudioMixPipeline> pipeline,
        int priority = 0);
    bool unregisterPipeline(PipelineHandle handle);
    [[nodiscard]] float mix(const AudioMixContext &context) const;
    [[nodiscard]] float masterVolume() const;
    void setMasterVolume(float volume);

private:
    struct PipelineEntry {
        PipelineHandle handle = 0;
        int priority = 0;
        std::unique_ptr<AudioMixPipeline> pipeline;
    };

    std::vector<PipelineEntry> pipelines;
    PipelineHandle nextPipelineHandle = 1;
    float masterVolumeValue = 1.0F;

    [[nodiscard]] float arrange(float accumulatedValue) const;
};
