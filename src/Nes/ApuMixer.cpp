#include "ApuMixer.h"

#include <algorithm>

float ApuMixer::mix(
    float pulse1,
    float pulse2,
    float triangle,
    float noise,
    float dpcm) {
    const float pulseSum = std::clamp(pulse1 + pulse2, 0.0F, 2.0F) * 15.0F;
    const float pulseOutput = pulseSum == 0.0F
        ? 0.0F
        : 95.88F / (8128.0F / pulseSum + 100.0F);

    const float tndInput = std::clamp(triangle, 0.0F, 1.0F) * 15.0F / 8227.0F
        + std::clamp(noise, 0.0F, 1.0F) * 15.0F / 12241.0F
        + std::clamp(dpcm, 0.0F, 1.0F) * 127.0F / 22638.0F;
    const float tndOutput = tndInput == 0.0F
        ? 0.0F
        : 159.79F / (1.0F / tndInput + 100.0F);

    return std::clamp(pulseOutput + tndOutput, 0.0F, 1.0F);
}
