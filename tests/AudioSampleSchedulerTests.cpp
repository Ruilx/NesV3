#include "AudioSampleScheduler.h"

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

void testPhaseAccumulatorAndCallbackValues() {
    AudioSampleScheduler scheduler({
        .inputClockNumerator = 10,
        .inputClockDenominator = 1,
        .outputSampleRate = 3,
    });
    std::vector<float> samples;
    scheduler.setSampleCallback(
        [&samples](float sample) { samples.push_back(sample); });

    for (quint64 cycle = 1; cycle <= 10; ++cycle) {
        scheduler.clock(static_cast<float>(cycle));
    }

    check(samples.size() == 3,
        "ten input cycles at a three tenths sample ratio should produce three samples");
    check(samples[0] == 4.0F && samples[1] == 7.0F && samples[2] == 10.0F,
        "samples should use the current value at each sample point");
    check(scheduler.generatedSampleCount() == 3,
        "generated sample count should match callback count");
    check(scheduler.phase() == 0,
        "phase should retain the exact remainder after ten input cycles");
}

void testNtscSecondSampleCount() {
    AudioSampleScheduler scheduler({
        .inputClockNumerator = 21477272,
        .inputClockDenominator = 12,
        .outputSampleRate = 44100,
    });

    for (quint64 cycle = 0; cycle < 1789773; ++cycle) {
        scheduler.clock(0.0F);
    }

    check(scheduler.generatedSampleCount() == 44100,
        "one NTSC CPU second should produce 44100 samples");
}

void testReset() {
    AudioSampleScheduler scheduler({
        .inputClockNumerator = 10,
        .inputClockDenominator = 1,
        .outputSampleRate = 3,
    });
    scheduler.clock(1.0F);
    scheduler.clock(2.0F);
    scheduler.clock(3.0F);
    scheduler.clock(4.0F);
    check(scheduler.generatedSampleCount() == 1,
        "reset test should first reach one sample");

    scheduler.reset();

    check(scheduler.generatedSampleCount() == 0,
        "reset should clear generated sample count");
    check(scheduler.phase() == 0,
        "reset should clear phase remainder");
    scheduler.clock(5.0F);
    check(scheduler.generatedSampleCount() == 0,
        "reset should restart the phase accumulator");
}
}

int main() {
    testPhaseAccumulatorAndCallbackValues();
    testNtscSecondSampleCount();
    testReset();
    std::cout << "AudioSampleScheduler tests passed\n";
    return EXIT_SUCCESS;
}
