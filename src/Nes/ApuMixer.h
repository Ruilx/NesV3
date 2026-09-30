#pragma once

class ApuMixer final {
public:
    [[nodiscard]] static float mix(
        float pulse1,
        float pulse2,
        float triangle,
        float noise,
        float dpcm);
};
