#pragma once

#include "../Bus.h"

#include <functional>
#include <utility>

class Mapper {
public:
    enum class Mirroring : quint8 {
        SingleScreenA,
        SingleScreenB,
        Vertical,
        Horizontal,
        FourScreen,
    };
    using MirroringCallback = std::function<void(Mirroring)>;
    using MappingChangedCallback = std::function<void()>;

    virtual ~Mapper() = default;

    [[nodiscard]] virtual bool chrRam() const = 0;
    [[nodiscard]] virtual Mirroring mirroring() const {
        return Mirroring::Horizontal;
    }

    void setMirroringCallback(MirroringCallback callback) {
        this->mirroringCallback = std::move(callback);
    }

    void setMappingChangedCallback(MappingChangedCallback callback) {
        this->mappingChangedCallback = std::move(callback);
    }

    virtual bool readCpu(quint16 address, quint8 &value) = 0;
    virtual bool writeCpu(quint16 address, quint8 value) = 0;
    virtual bool readPpu(quint16 address, quint8 &value) = 0;
    virtual bool writePpu(quint16 address, quint8 value) = 0;

protected:
    void notifyMirroringChanged() {
        if (this->mirroringCallback) {
            this->mirroringCallback(this->mirroring());
        }
    }

    void notifyMappingChanged() {
        if (this->mappingChangedCallback) {
            this->mappingChangedCallback();
        }
    }

private:
    MirroringCallback mirroringCallback;
    MappingChangedCallback mappingChangedCallback;
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
