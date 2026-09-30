#include "AudioSampleScheduler.h"

#include <utility>

AudioSampleScheduler::AudioSampleScheduler()
    : AudioSampleScheduler(Configuration()) {
}

AudioSampleScheduler::AudioSampleScheduler(Configuration configuration)
    : configurationValue(configuration) {
    if (this->configurationValue.inputClockNumerator == 0) {
        this->configurationValue.inputClockNumerator = 1;
    }
    if (this->configurationValue.inputClockDenominator == 0) {
        this->configurationValue.inputClockDenominator = 1;
    }
    if (this->configurationValue.outputSampleRate == 0) {
        this->configurationValue.outputSampleRate = 1;
    }
}

quint32 AudioSampleScheduler::advance() {
    this->phaseValue += static_cast<quint64>(
        this->configurationValue.outputSampleRate)
        * this->configurationValue.inputClockDenominator;
    const quint64 inputClock = this->configurationValue.inputClockNumerator;
    quint32 sampleCount = 0;
    while (this->phaseValue >= inputClock) {
        this->phaseValue -= inputClock;
        ++this->generatedSamples;
        ++sampleCount;
    }
    return sampleCount;
}

void AudioSampleScheduler::clock(float currentSample) {
    const quint32 sampleCount = this->advance();
    for (quint32 index = 0; index < sampleCount; ++index) {
        this->submitSample(currentSample);
    }
}

void AudioSampleScheduler::submitSample(float currentSample) {
    if (this->sampleCallback) {
        this->sampleCallback(currentSample);
    }
}

void AudioSampleScheduler::reset() {
    this->phaseValue = 0;
    this->generatedSamples = 0;
}

void AudioSampleScheduler::setSampleCallback(SampleCallback callback) {
    this->sampleCallback = std::move(callback);
}

const AudioSampleScheduler::Configuration &AudioSampleScheduler::configuration() const {
    return this->configurationValue;
}

quint64 AudioSampleScheduler::generatedSampleCount() const {
    return this->generatedSamples;
}

quint64 AudioSampleScheduler::phase() const {
    return this->phaseValue;
}
