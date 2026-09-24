#pragma once

#include "../Bus.h"

class Mapper {
public:
    virtual ~Mapper() = default;

    [[nodiscard]] virtual bool chrRam() const = 0;

    virtual bool readCpu(quint16 address, quint8 &value) = 0;
    virtual bool writeCpu(quint16 address, quint8 value) = 0;
    virtual bool readPpu(quint16 address, quint8 &value) = 0;
    virtual bool writePpu(quint16 address, quint8 value) = 0;
};

class MapperCpuDevice final : public BusDevice {
public:
    explicit MapperCpuDevice(Mapper *mapper = nullptr);

    void setMapper(Mapper *mapper);

    bool read(quint16 address, quint8 &value) override;
    bool write(quint16 address, quint8 value) override;

private:
    Mapper *mapper = nullptr;
};

class MapperPpuDevice final : public BusDevice {
public:
    explicit MapperPpuDevice(Mapper *mapper = nullptr);

    void setMapper(Mapper *mapper);

    bool read(quint16 address, quint8 &value) override;
    bool write(quint16 address, quint8 value) override;

private:
    Mapper *mapper = nullptr;
};
