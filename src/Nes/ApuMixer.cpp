#include "ApuMixer.h"

#include <algorithm>
#include <utility>

namespace {
qsizetype busIndex(AudioMixBus bus) {
    return static_cast<qsizetype>(bus);
}

class NativePulsePipeline final : public AudioMixPipeline {
public:
    [[nodiscard]] float render(const AudioMixContext &context) const override {
        const float pulseSum = std::clamp(
            context.value(AudioMixBus::NativePulse), 0.0F, 2.0F) * 15.0F;
        return pulseSum == 0.0F
            ? 0.0F
            : 95.88F / (8128.0F / pulseSum + 100.0F);
    }
};

class NativeTndPipeline final : public AudioMixPipeline {
public:
    [[nodiscard]] float render(const AudioMixContext &context) const override {
        const float tndInput = std::clamp(
            context.value(AudioMixBus::NativeTriangle), 0.0F, 1.0F) * 15.0F / 8227.0F
            + std::clamp(context.value(AudioMixBus::NativeNoise), 0.0F, 1.0F)
                * 15.0F / 12241.0F
            + std::clamp(context.value(AudioMixBus::NativeDpcm), 0.0F, 1.0F)
                * 127.0F / 22638.0F;
        return tndInput == 0.0F
            ? 0.0F
            : 159.79F / (1.0F / tndInput + 100.0F);
    }
};

class ExpansionPipeline final : public AudioMixPipeline {
public:
    [[nodiscard]] float render(const AudioMixContext &context) const override {
        return context.value(AudioMixBus::Expansion);
    }
};
}

void AudioMixContext::add(AudioMixBus bus, float value) {
    this->values[busIndex(bus)] += value;
}

float AudioMixContext::value(AudioMixBus bus) const {
    return this->values[busIndex(bus)];
}

void AudioMixContext::clear() {
    this->values.fill(0.0F);
}

ApuMixer::ApuMixer() {
    this->registerPipeline(std::make_unique<NativePulsePipeline>());
    this->registerPipeline(std::make_unique<NativeTndPipeline>());
    this->registerPipeline(std::make_unique<ExpansionPipeline>());
}

ApuMixer::PipelineHandle ApuMixer::registerPipeline(
    std::unique_ptr<AudioMixPipeline> pipeline,
    int priority) {
    if (!pipeline) {
        return 0;
    }
    const PipelineHandle handle = this->nextPipelineHandle++;
    this->pipelines.push_back(PipelineEntry{
        .handle = handle,
        .priority = priority,
        .pipeline = std::move(pipeline),
    });
    std::stable_sort(this->pipelines.begin(), this->pipelines.end(),
        [](const PipelineEntry &left, const PipelineEntry &right) {
            return left.priority < right.priority;
        });
    return handle;
}

bool ApuMixer::unregisterPipeline(PipelineHandle handle) {
    for (auto iterator = this->pipelines.begin(); iterator != this->pipelines.end();
         ++iterator) {
        if (iterator->handle == handle) {
            this->pipelines.erase(iterator);
            return true;
        }
    }
    return false;
}

float ApuMixer::mix(const AudioMixContext &context) const {
    float accumulatedValue = 0.0F;
    for (const PipelineEntry &entry : this->pipelines) {
        accumulatedValue += entry.pipeline->render(context);
    }
    return this->arrange(accumulatedValue);
}

float ApuMixer::masterVolume() const {
    return this->masterVolumeValue;
}

void ApuMixer::setMasterVolume(float volume) {
    this->masterVolumeValue = std::clamp(volume, 0.0F, 1.0F);
}

float ApuMixer::arrange(float accumulatedValue) const {
    return std::clamp(
        accumulatedValue * this->masterVolumeValue, 0.0F, 1.0F);
}
