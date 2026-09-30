#pragma once

#include <QtGlobal>

#include <functional>

class AudioSampleScheduler final {
public:
    struct Configuration {
        quint64 inputClockNumerator = 21477272;
        quint64 inputClockDenominator = 12;
        quint32 outputSampleRate = 44100;
    };

    using SampleCallback = std::function<void(float)>;

    AudioSampleScheduler();
    explicit AudioSampleScheduler(Configuration configuration);

    [[nodiscard]] quint32 advance();
    void clock(float currentSample);
    void submitSample(float currentSample);
    void reset();
    void setSampleCallback(SampleCallback callback);

    [[nodiscard]] const Configuration &configuration() const;
    [[nodiscard]] quint64 generatedSampleCount() const;
    [[nodiscard]] quint64 phase() const;

private:
    Configuration configurationValue;
    SampleCallback sampleCallback;
    quint64 phaseValue = 0;
    quint64 generatedSamples = 0;
};
