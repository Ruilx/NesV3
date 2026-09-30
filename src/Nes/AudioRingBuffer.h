#pragma once

#include <QtGlobal>

#include <atomic>
#include <vector>

class AudioRingBuffer final {
public:
    static constexpr quint32 DefaultCapacity = 32768;

    AudioRingBuffer();
    explicit AudioRingBuffer(quint32 capacity);

    AudioRingBuffer(const AudioRingBuffer &) = delete;
    AudioRingBuffer &operator=(const AudioRingBuffer &) = delete;

    [[nodiscard]] quint32 write(const float *samples, quint32 count);
    [[nodiscard]] quint32 read(float *samples, quint32 count);
    [[nodiscard]] bool push(float sample);
    [[nodiscard]] bool pop(float &sample);

    void clear();
    void reset();

    [[nodiscard]] quint32 capacity() const;
    [[nodiscard]] quint32 availableToRead() const;
    [[nodiscard]] quint32 availableToWrite() const;
    [[nodiscard]] quint64 droppedSampleCount() const;
    [[nodiscard]] quint64 underrunSampleCount() const;

private:
    std::vector<float> samples;
    quint32 capacityValue = 0;
    std::atomic<quint64> readIndex = 0;
    std::atomic<quint64> writeIndex = 0;
    std::atomic<quint64> droppedSamples = 0;
    std::atomic<quint64> underrunSamples = 0;
};
