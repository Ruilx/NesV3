#pragma once

#include <QIODevice>

#include "Nes/AudioRingBuffer.h"

#include <atomic>

class AudioOutputDevice final : public QIODevice {
public:
    explicit AudioOutputDevice(AudioRingBuffer &ringBuffer, QObject *parent = nullptr);

    [[nodiscard]] qint64 bytesAvailable() const override;
    [[nodiscard]] bool isSequential() const override;
    [[nodiscard]] qint64 maximumReadRequestBytes() const;
    [[nodiscard]] quint64 readCallCount() const;
    void resetReadStatistics();

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    AudioRingBuffer &ringBuffer;
    std::atomic<qint64> maximumReadRequest = 0;
    std::atomic<quint64> readCalls = 0;
};
