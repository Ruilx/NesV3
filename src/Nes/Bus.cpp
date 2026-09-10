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
    this->mappings.insert(id, {id, mapping});
    return id;
}

bool Bus::unregisterMapping(MappingId id) {
    return this->mappings.remove(id) > 0;
}

bool Bus::setMappingFlags(MappingId id, AccessFlags flags) {
    auto iterator = this->mappings.find(id);
    if (iterator == this->mappings.end()) {
        return false;
    }

    iterator->mapping.flags = flags;
    return true;
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
    const RegisteredMapping *best = nullptr;
    for (const RegisteredMapping &registered : this->mappings) {
        const Mapping &mapping = registered.mapping;
        if (address < mapping.start || address > mapping.end) {
            continue;
        }

        if (best == nullptr || mapping.priority > best->mapping.priority) {
            best = &registered;
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
