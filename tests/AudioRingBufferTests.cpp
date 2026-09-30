#include "AudioRingBuffer.h"

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

void testFifoAndOverflow() {
    AudioRingBuffer buffer(3);
    const float input[] = {1.0F, 2.0F, 3.0F, 4.0F};

    check(buffer.write(input, 4) == 3,
        "a full ring should accept only its capacity");
    check(buffer.availableToRead() == 3 && buffer.availableToWrite() == 0,
        "a full ring should report matching read and write availability");
    check(buffer.droppedSampleCount() == 1,
        "overflow should count dropped samples");

    float output[3] = {};
    check(buffer.read(output, 3) == 3,
        "a full ring should return every queued sample");
    check(output[0] == 1.0F && output[1] == 2.0F && output[2] == 3.0F,
        "ring reads should preserve FIFO order");
}

void testWrapAround() {
    AudioRingBuffer buffer(3);
    check(buffer.push(1.0F), "first sample should be written");
    check(buffer.push(2.0F), "second sample should be written");

    float sample = 0.0F;
    check(buffer.pop(sample) && sample == 1.0F,
        "the oldest sample should be read before wrap-around");
    check(buffer.push(3.0F), "third sample should be written after reading");
    check(buffer.push(4.0F), "fourth sample should wrap to the start");

    const float expected[] = {2.0F, 3.0F, 4.0F};
    float output[3] = {};
    check(buffer.read(output, 3) == 3,
        "wrapped samples should all be readable");
    check(output[0] == expected[0] && output[1] == expected[1]
            && output[2] == expected[2],
        "wrapped reads should preserve FIFO order");
}

void testUnderrunAndReset() {
    AudioRingBuffer buffer(2);
    float output[3] = {};
    check(buffer.read(output, 3) == 0,
        "an empty ring should not return samples");
    check(buffer.underrunSampleCount() == 3,
        "reading an empty ring should count missing samples");

    check(buffer.push(5.0F), "sample should be written before reset");
    buffer.reset();
    check(buffer.availableToRead() == 0 && buffer.availableToWrite() == 2,
        "reset should discard queued samples");
    check(buffer.droppedSampleCount() == 0
            && buffer.underrunSampleCount() == 0,
        "reset should clear ring statistics");
}

void testBatchOperations() {
    AudioRingBuffer buffer(4);
    const std::vector<float> input = {0.25F, -0.5F, 0.75F};
    check(buffer.write(input.data(), static_cast<quint32>(input.size())) == 3,
        "batch write should return the number of accepted samples");

    float output[2] = {};
    check(buffer.read(output, 2) == 2,
        "batch read should return the requested available samples");
    check(output[0] == 0.25F && output[1] == -0.5F,
        "batch operations should preserve sample values");
    check(buffer.availableToRead() == 1,
        "batch read should leave the remaining samples queued");
}
}

int main() {
    testFifoAndOverflow();
    testWrapAround();
    testUnderrunAndReset();
    testBatchOperations();
    std::cout << "AudioRingBuffer tests passed\n";
    return EXIT_SUCCESS;
}
