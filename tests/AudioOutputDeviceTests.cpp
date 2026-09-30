#include "AudioOutputDevice.h"

#include <QByteArray>

#include <cstdlib>
#include <iostream>

namespace {
void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void checkByte(const QByteArray &data, int index, unsigned char expected,
        const char *message) {
    check(index < data.size()
            && static_cast<unsigned char>(data.at(index)) == expected,
        message);
}

void testPcm16Conversion() {
    AudioRingBuffer buffer(8);
    const float samples[] = {-1.0F, -0.5F, 0.0F, 0.5F, 1.0F};
    check(buffer.write(samples, 5) == 5,
        "all PCM conversion samples should enter the ring");

    AudioOutputDevice device(buffer);
    check(device.open(QIODevice::ReadOnly),
        "audio output device should open for reading");
    const QByteArray data = device.read(10);
    check(data.size() == 10,
        "PCM reads should return the requested byte count");

    checkByte(data, 0, 0x00, "-1.0 low byte should be zero");
    checkByte(data, 1, 0x80, "-1.0 high byte should be 0x80");
    checkByte(data, 2, 0x00, "-0.5 low byte should be zero");
    checkByte(data, 3, 0xC0, "-0.5 high byte should be 0xC0");
    checkByte(data, 4, 0x00, "zero low byte should be zero");
    checkByte(data, 5, 0x00, "zero high byte should be zero");
    checkByte(data, 6, 0x00, "0.5 low byte should be zero");
    checkByte(data, 7, 0x40, "0.5 high byte should be 0x40");
    checkByte(data, 8, 0xFF, "1.0 low byte should be 0xFF");
    checkByte(data, 9, 0x7F, "1.0 high byte should be 0x7F");
}

void testUnderrunFillsSilence() {
    AudioRingBuffer buffer(2);
    AudioOutputDevice device(buffer);
    check(device.open(QIODevice::ReadOnly),
        "empty audio output device should open for reading");

    const QByteArray data = device.read(5);
    check(data.size() == 5,
        "underrun reads should still return the requested byte count");
    for (char value : data) {
        check(value == 0, "underrun bytes should be silence");
    }
    check(buffer.underrunSampleCount() > 0,
        "underrun should count missing PCM frames");
}
}

int main() {
    testPcm16Conversion();
    testUnderrunFillsSilence();
    std::cout << "AudioOutputDevice tests passed\n";
    return EXIT_SUCCESS;
}
