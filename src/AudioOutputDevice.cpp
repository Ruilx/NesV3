#include "AudioOutputDevice.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace {
constexpr quint32 ConversionChunkSamples = 256;

qint16 toPcm16(float sample) {
    if (!std::isfinite(sample)) {
        sample = 0.0F;
    }
    sample = std::clamp(sample, -1.0F, 1.0F);
    if (sample <= -1.0F) {
        return static_cast<qint16>(-32768);
    }
    if (sample >= 1.0F) {
        return static_cast<qint16>(32767);
    }

    const long scaled = sample < 0.0F
        ? std::lround(sample * 32768.0F)
        : std::lround(sample * 32767.0F);
    return static_cast<qint16>(scaled);
}

void writeLittleEndianPcm16(char *destination, qint16 sample) {
    const quint16 bits = static_cast<quint16>(sample);
    destination[0] = static_cast<char>(bits & 0x00FF);
    destination[1] = static_cast<char>((bits >> 8) & 0x00FF);
}
}

AudioOutputDevice::AudioOutputDevice(
        AudioRingBuffer &ringBuffer,
        QObject *parent)
    : QIODevice(parent), ringBuffer(ringBuffer) {
}

qint64 AudioOutputDevice::bytesAvailable() const {
    return static_cast<qint64>(this->ringBuffer.availableToRead())
            * static_cast<qint64>(sizeof(qint16))
        + QIODevice::bytesAvailable();
}

bool AudioOutputDevice::isSequential() const {
    return true;
}

qint64 AudioOutputDevice::maximumReadRequestBytes() const {
    return this->maximumReadRequest.load(std::memory_order_relaxed);
}

quint64 AudioOutputDevice::readCallCount() const {
    return this->readCalls.load(std::memory_order_relaxed);
}

void AudioOutputDevice::resetReadStatistics() {
    this->maximumReadRequest.store(0, std::memory_order_relaxed);
    this->readCalls.store(0, std::memory_order_relaxed);
}

qint64 AudioOutputDevice::readData(char *data, qint64 maxlen) {
    if (data == nullptr || maxlen <= 0) {
        return 0;
    }

    this->readCalls.fetch_add(1, std::memory_order_relaxed);
    qint64 previousMaximum = this->maximumReadRequest.load(
        std::memory_order_relaxed);
    while (previousMaximum < maxlen
           && !this->maximumReadRequest.compare_exchange_weak(
               previousMaximum, maxlen,
               std::memory_order_relaxed,
               std::memory_order_relaxed)) {
    }

    const qint64 frameCount = maxlen / static_cast<qint64>(sizeof(qint16));
    if (frameCount == 0) {
        data[0] = 0;
        return 1;
    }

    std::array<float, ConversionChunkSamples> samples = {};
    qint64 framesRemaining = frameCount;
    qint64 frameOffset = 0;
    while (framesRemaining > 0) {
        const quint32 requestedFrames = static_cast<quint32>(std::min<qint64>(
            framesRemaining, ConversionChunkSamples));
        const quint32 readFrames = this->ringBuffer.read(
            samples.data(), requestedFrames);
        for (quint32 index = 0; index < requestedFrames; ++index) {
            const float sample = index < readFrames ? samples[index] : 0.0F;
            writeLittleEndianPcm16(
                data + (frameOffset + index) * sizeof(qint16),
                toPcm16(sample));
        }
        frameOffset += requestedFrames;
        framesRemaining -= requestedFrames;
    }

    if ((maxlen % static_cast<qint64>(sizeof(qint16))) != 0) {
        data[maxlen - 1] = 0;
    }
    return maxlen;
}

qint64 AudioOutputDevice::writeData(const char *, qint64) {
    return -1;
}
