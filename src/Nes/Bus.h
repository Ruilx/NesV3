#pragma once

#include <QHash>
#include <QString>
#include <QtGlobal>

#include <functional>

class Ram;

enum class AccessFlags : quint8 {
    None = 0,
    Readable = 1 << 0,
    Writable = 1 << 1,
    Executable = 1 << 2,
};

constexpr AccessFlags operator|(AccessFlags left, AccessFlags right) {
    return static_cast<AccessFlags>(static_cast<quint8>(left) | static_cast<quint8>(right));
}

constexpr AccessFlags operator&(AccessFlags left, AccessFlags right) {
    return static_cast<AccessFlags>(static_cast<quint8>(left) & static_cast<quint8>(right));
}

constexpr bool hasAccessFlag(AccessFlags flags, AccessFlags flag) {
    return (flags & flag) == flag;
}

class BusDevice {
public:
    virtual ~BusDevice() = default;

    virtual bool read(quint16 address, quint8 &value) = 0;
    virtual bool write(quint16 address, quint8 value) = 0;
};

class Bus {
public:
    using MappingId = quint64;
    using AddressTranslator = std::function<quint16(quint16)>;

    enum class AccessResult {
        Handled,
        Unmapped,
        PermissionDenied,
        DeviceRejected,
    };

    struct Mapping {
        quint16 start = 0;
        quint16 end = 0;
        int priority = 0;
        AccessFlags flags = AccessFlags::Readable | AccessFlags::Writable;
        QString name;
        BusDevice *device = nullptr;
        AddressTranslator translate;
    };

    explicit Bus(quint32 addressSpaceSize);

    MappingId registerMapping(const Mapping &mapping);
    bool unregisterMapping(MappingId id);
    bool setMappingFlags(MappingId id, AccessFlags flags);

    AccessResult read(quint16 address, quint8 &value);
    AccessResult write(quint16 address, quint8 value);

    [[nodiscard]] quint8 openBusValue() const;
    [[nodiscard]] quint32 addressSpaceSize() const;

private:
    struct RegisteredMapping {
        MappingId id = 0;
        Mapping mapping;
    };

    [[nodiscard]] const RegisteredMapping *findMapping(quint16 address) const;
    [[nodiscard]] bool overlaps(const Mapping &left, const Mapping &right) const;

    quint32 addressSpaceSizeValue;
    quint8 openBus = 0;
    MappingId nextMappingId = 1;
    QHash<MappingId, RegisteredMapping> mappings;
};

class RamBusDevice final : public BusDevice {
public:
    explicit RamBusDevice(Ram &ram);

    bool read(quint16 address, quint8 &value) override;
    bool write(quint16 address, quint8 value) override;

private:
    Ram &ram;
};
