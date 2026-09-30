#pragma once

#include <QtGlobal>

class AudioOutputShaper final {
public:
    struct Configuration {
        float dcBlockerCoefficient = 0.995F;
        float outputGain = 1.0F;
    };

    AudioOutputShaper();
    explicit AudioOutputShaper(Configuration configuration);

    [[nodiscard]] float process(float inputSample);
    void reset();

    [[nodiscard]] const Configuration &configuration() const;
    [[nodiscard]] float previousInput() const;
    [[nodiscard]] float previousOutput() const;

private:
    Configuration configurationValue;
    float previousInputValue = 0.0F;
    float previousOutputValue = 0.0F;
};
