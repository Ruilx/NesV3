#include "Bus.h"

#include "Ram.h"

Bus::Bus(quint32 addressSpaceSize)
    : addressSpaceSizeValue(addressSpaceSize) {
}

Bus::MappingId Bus::registerMapping(const Mapping &mapping) {
    if (mapping.device == nullptr || mapping.start > mapping.end
        || mapping.end >= this->addressSpaceSizeValue) {
        return 0;
    }

    for (const RegisteredMapping &registered : this->mappings) {
        if (overlaps(registered.mapping, mapping)) {
            return 0;
        }
    }

    const MappingId id = this->nextMappingId++;
    this->mappings.append({id, mapping});
    return id;
}

bool Bus::unregisterMapping(MappingId id) {
    for (qsizetype index = 0; index < this->mappings.size(); ++index) {
        if (this->mappings[index].id != id) {
            continue;
        }
        this->mappings.removeAt(index);
        this->cachedMappingIndex = -1;
        return true;
    }
    return false;
}

bool Bus::setMappingFlags(MappingId id, AccessFlags flags) {
    for (RegisteredMapping &registered : this->mappings) {
        if (registered.id == id) {
            registered.mapping.flags = flags;
            return true;
        }
    }
    return false;
}

Bus::AccessResult Bus::read(quint16 address, quint8 &value) {
    const RegisteredMapping *registered = findMapping(address);
    if (registered == nullptr) {
        value = this->openBus;
        return AccessResult::Unmapped;
    }

    const Mapping &mapping = registered->mapping;
    if (!hasAccessFlag(mapping.flags, AccessFlags::Readable)) {
        value = this->openBus;
        return AccessResult::PermissionDenied;
    }

    const quint16 deviceAddress = mapping.translate
        ? mapping.translate(address)
        : static_cast<quint16>(address - mapping.start);
    if (!mapping.device->read(deviceAddress, value)) {
        value = this->openBus;
        return AccessResult::DeviceRejected;
    }

    this->openBus = value;
    return AccessResult::Handled;
}

Bus::AccessResult Bus::write(quint16 address, quint8 value) {
    const RegisteredMapping *registered = findMapping(address);
    if (registered == nullptr) {
        this->openBus = value;
        return AccessResult::Unmapped;
    }

    const Mapping &mapping = registered->mapping;
    if (!hasAccessFlag(mapping.flags, AccessFlags::Writable)) {
        this->openBus = value;
        return AccessResult::PermissionDenied;
    }

    const quint16 deviceAddress = mapping.translate
        ? mapping.translate(address)
        : static_cast<quint16>(address - mapping.start);
    if (!mapping.device->write(deviceAddress, value)) {
        this->openBus = value;
        return AccessResult::DeviceRejected;
    }

    this->openBus = value;
    return AccessResult::Handled;
}

quint8 Bus::openBusValue() const {
    return this->openBus;
}

quint32 Bus::addressSpaceSize() const {
    return this->addressSpaceSizeValue;
}

const Bus::RegisteredMapping *Bus::findMapping(quint16 address) const {
    if (this->cachedMappingIndex >= 0
        && this->cachedMappingIndex < this->mappings.size()) {
        const RegisteredMapping &cached =
            this->mappings[this->cachedMappingIndex];
        if (address >= cached.mapping.start && address <= cached.mapping.end) {
            return &cached;
        }
    }

    const RegisteredMapping *best = nullptr;
    for (qsizetype index = 0; index < this->mappings.size(); ++index) {
        const RegisteredMapping &registered = this->mappings[index];
        const Mapping &mapping = registered.mapping;
        if (address < mapping.start || address > mapping.end) {
            continue;
        }

        if (best == nullptr || mapping.priority > best->mapping.priority) {
            best = &registered;
            this->cachedMappingIndex = index;
        }
    }
    return best;
}

bool Bus::overlaps(const Mapping &left, const Mapping &right) const {
    return left.start <= right.end && right.start <= left.end;
}

RamBusDevice::RamBusDevice(Ram &ram)
    : ram(ram) {
}

bool RamBusDevice::read(quint16 address, quint8 &value) {
    if (address >= this->ram.getSize()) {
        return false;
    }

    value = this->ram.getU8(address);
    return true;
}

bool RamBusDevice::write(quint16 address, quint8 value) {
    if (address >= this->ram.getSize()) {
        return false;
    }

    this->ram.setU8(address, value);
    return true;
}
