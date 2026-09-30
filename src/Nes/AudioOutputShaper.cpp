#include "AudioOutputShaper.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float DefaultDcBlockerCoefficient = 0.995F;
constexpr float MaximumDcBlockerCoefficient = 0.999999F;

float sanitizeInput(float value) {
    if (!std::isfinite(value)) {
        return 0.0F;
    }
    return std::clamp(value, 0.0F, 1.0F);
}
}

AudioOutputShaper::AudioOutputShaper()
    : AudioOutputShaper(Configuration()) {
}

AudioOutputShaper::AudioOutputShaper(Configuration configuration)
    : configurationValue(configuration) {
    if (!std::isfinite(this->configurationValue.dcBlockerCoefficient)) {
        this->configurationValue.dcBlockerCoefficient = DefaultDcBlockerCoefficient;
    }
    this->configurationValue.dcBlockerCoefficient = std::clamp(
        this->configurationValue.dcBlockerCoefficient,
        0.0F,
        MaximumDcBlockerCoefficient);
    if (!std::isfinite(this->configurationValue.outputGain)
        || this->configurationValue.outputGain < 0.0F) {
        this->configurationValue.outputGain = 1.0F;
    }
}

float AudioOutputShaper::process(float inputSample) {
    const float sanitizedInput = sanitizeInput(inputSample);
    const float filteredSample = sanitizedInput
        - this->previousInputValue
        + this->configurationValue.dcBlockerCoefficient
            * this->previousOutputValue;
    this->previousInputValue = sanitizedInput;
    this->previousOutputValue = filteredSample;

    const float shapedSample = filteredSample
        * this->configurationValue.outputGain;
    return std::clamp(shapedSample, -1.0F, 1.0F);
}

void AudioOutputShaper::reset() {
    this->previousInputValue = 0.0F;
    this->previousOutputValue = 0.0F;
}

const AudioOutputShaper::Configuration &AudioOutputShaper::configuration() const {
    return this->configurationValue;
}

float AudioOutputShaper::previousInput() const {
    return this->previousInputValue;
}

float AudioOutputShaper::previousOutput() const {
    return this->previousOutputValue;
}
