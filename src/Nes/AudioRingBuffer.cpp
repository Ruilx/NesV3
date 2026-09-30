#include "AudioRingBuffer.h"

#include <algorithm>
#include <utility>

AudioRingBuffer::AudioRingBuffer()
    : AudioRingBuffer(DefaultCapacity) {
}

AudioRingBuffer::AudioRingBuffer(quint32 capacity)
    : samples(capacity == 0 ? 1 : capacity),
            capacityValue(capacity == 0 ? 1 : capacity) {
}

quint32 AudioRingBuffer::write(const float *source, quint32 count) {
    if (source == nullptr || count == 0) {
        return 0;
    }

    const quint64 currentWriteIndex =
        this->writeIndex.load(std::memory_order_relaxed);
    const quint64 currentReadIndex =
        this->readIndex.load(std::memory_order_acquire);
    const quint64 queuedSamples = currentWriteIndex - currentReadIndex;
    const quint32 writableSamples = static_cast<quint32>(std::min<quint64>(
        this->capacityValue - static_cast<quint32>(queuedSamples), count));
    const quint32 droppedCount = count - writableSamples;

    for (quint32 index = 0; index < writableSamples; ++index) {
        const quint32 bufferIndex = static_cast<quint32>(
            (currentWriteIndex + index) % this->capacityValue);
        this->samples[bufferIndex] = source[index];
    }
    this->writeIndex.store(
        currentWriteIndex + writableSamples,
        std::memory_order_release);
    if (droppedCount > 0) {
        this->droppedSamples.fetch_add(
            droppedCount, std::memory_order_relaxed);
    }
    return writableSamples;
}

quint32 AudioRingBuffer::read(float *destination, quint32 count) {
    if (destination == nullptr || count == 0) {
        return 0;
    }

    const quint64 currentReadIndex =
        this->readIndex.load(std::memory_order_relaxed);
    const quint64 currentWriteIndex =
        this->writeIndex.load(std::memory_order_acquire);
    const quint64 queuedSamples = currentWriteIndex - currentReadIndex;
    const quint32 readableSamples = static_cast<quint32>(std::min<quint64>(
        queuedSamples, count));
    const quint32 missingSamples = count - readableSamples;

    for (quint32 index = 0; index < readableSamples; ++index) {
        const quint32 bufferIndex = static_cast<quint32>(
            (currentReadIndex + index) % this->capacityValue);
        destination[index] = this->samples[bufferIndex];
    }
    this->readIndex.store(
        currentReadIndex + readableSamples,
        std::memory_order_release);
    if (missingSamples > 0) {
        this->underrunSamples.fetch_add(
            missingSamples, std::memory_order_relaxed);
    }
    return readableSamples;
}

bool AudioRingBuffer::push(float sample) {
    return this->write(&sample, 1) == 1;
}

bool AudioRingBuffer::pop(float &sample) {
    return this->read(&sample, 1) == 1;
}

void AudioRingBuffer::clear() {
    const quint64 currentWriteIndex =
        this->writeIndex.load(std::memory_order_acquire);
    this->readIndex.store(currentWriteIndex, std::memory_order_release);
}

void AudioRingBuffer::reset() {
    this->clear();
    this->droppedSamples.store(0, std::memory_order_relaxed);
    this->underrunSamples.store(0, std::memory_order_relaxed);
}

quint32 AudioRingBuffer::capacity() const {
    return this->capacityValue;
}

quint32 AudioRingBuffer::availableToRead() const {
    const quint64 currentReadIndex =
        this->readIndex.load(std::memory_order_relaxed);
    const quint64 currentWriteIndex =
        this->writeIndex.load(std::memory_order_acquire);
    return static_cast<quint32>(currentWriteIndex - currentReadIndex);
}

quint32 AudioRingBuffer::availableToWrite() const {
    const quint64 currentReadIndex =
        this->readIndex.load(std::memory_order_acquire);
    const quint64 currentWriteIndex =
        this->writeIndex.load(std::memory_order_relaxed);
    const quint64 queuedSamples = currentWriteIndex - currentReadIndex;
    return this->capacityValue - static_cast<quint32>(queuedSamples);
}

quint64 AudioRingBuffer::droppedSampleCount() const {
    return this->droppedSamples.load(std::memory_order_relaxed);
}

quint64 AudioRingBuffer::underrunSampleCount() const {
    return this->underrunSamples.load(std::memory_order_relaxed);
}
