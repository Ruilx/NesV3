#include "AudioOutputShaper.h"
#include "Nes.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {
void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void checkNear(float actual, float expected, float tolerance, const char *message) {
    check(std::fabs(actual - expected) <= tolerance, message);
}

void testDcBlocker() {
    AudioOutputShaper shaper({
        .dcBlockerCoefficient = 0.5F,
        .outputGain = 1.0F,
    });

    checkNear(shaper.process(0.8F), 0.8F, 0.0001F,
        "the first sample should pass through the dc blocker");
    checkNear(shaper.process(0.8F), 0.4F, 0.0001F,
        "a constant input should decay by the blocker coefficient");
    checkNear(shaper.process(0.0F), -0.6F, 0.0001F,
        "a falling input should produce a negative transient");
}

void testInputSanitizingAndOutputClamping() {
    AudioOutputShaper shaper({
        .dcBlockerCoefficient = 0.0F,
        .outputGain = 4.0F,
    });

    check(shaper.process(2.0F) == 1.0F,
        "input above the mixer range should be clamped before shaping");
    check(shaper.process(-1.0F) == -1.0F,
        "output gain should be limited by the final signed output clamp");
    check(shaper.process(NAN) == 0.0F,
        "non-finite input should be treated as silence");
}

void testReset() {
    AudioOutputShaper shaper({
        .dcBlockerCoefficient = 0.5F,
        .outputGain = 1.0F,
    });
    const float firstSample = shaper.process(0.75F);
    const float secondSample = shaper.process(0.75F);
    check(firstSample > 0.0F && secondSample > 0.0F,
        "the reset test should establish non-zero blocker state");
    check(shaper.previousInput() == 0.75F,
        "the blocker should retain the previous input before reset");

    shaper.reset();

    check(shaper.previousInput() == 0.0F && shaper.previousOutput() == 0.0F,
        "reset should clear both blocker state values");
    check(shaper.process(0.0F) == 0.0F,
        "zero input after reset should produce zero output immediately");
}

void testNesAudioCallbackIntegration() {
    Nes nes;
    std::vector<float> samples;
    nes.setAudioSampleCallback(
        [&samples](float sample) { samples.push_back(sample); });

    nes.runMasterTicks(300);

    check(nes.generatedAudioSampleCount() == 2,
        "100 CPU cycles should generate two 44100 Hz audio samples");
    check(samples.size() == 2,
        "Nes should forward every generated sample to the callback");
    check(samples[0] == 0.0F && samples[1] == 0.0F,
        "an idle APU should produce shaped silence");

    nes.resetAudioSampling();
    check(nes.generatedAudioSampleCount() == 0,
        "reset should clear the integrated scheduler sample count");
    nes.runMasterTicks(300);
    check(nes.generatedAudioSampleCount() == 2,
        "sampling should restart from a clean phase after reset");
    check(samples.size() == 4,
        "the callback should continue after audio sampling reset");
}
}

int main() {
    testDcBlocker();
    testInputSanitizingAndOutputClamping();
    testReset();
    testNesAudioCallbackIntegration();
    std::cout << "AudioOutputShaper tests passed\n";
    return EXIT_SUCCESS;
}
