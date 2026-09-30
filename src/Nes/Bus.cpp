#include "Bus.h"

#include "Diagnostics.h"
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
        this->invalidateMappingCaches();
        return true;
    }
    return false;
}

bool Bus::setMappingFlags(MappingId id, AccessFlags flags) {
    for (RegisteredMapping &registered : this->mappings) {
        if (registered.id == id) {
            Mapping candidate = registered.mapping;
            candidate.flags = flags;
            for (const RegisteredMapping &other : this->mappings) {
                if (other.id != id && overlaps(other.mapping, candidate)) {
                    return false;
                }
            }
            registered.mapping.flags = flags;
            this->invalidateMappingCaches();
            return true;
        }
    }
    return false;
}

Bus::AccessResult Bus::read(quint16 address, quint8 &value) {
    if constexpr (NesDiagnostics::EnableBusStats) {
        ++this->statistics.reads;
    }
    const RegisteredMapping *registered = findMapping(address, AccessFlags::Readable);
    if (registered == nullptr) {
        value = this->openBus;
        return hasMapping(address) ? AccessResult::PermissionDenied
                                    : AccessResult::Unmapped;
    }

    const Mapping &mapping = registered->mapping;

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
    if constexpr (NesDiagnostics::EnableBusStats) {
        ++this->statistics.writes;
    }
    const RegisteredMapping *registered = findMapping(address, AccessFlags::Writable);
    if (registered == nullptr) {
        this->openBus = value;
        return hasMapping(address) ? AccessResult::PermissionDenied
                                   : AccessResult::Unmapped;
    }

    const Mapping &mapping = registered->mapping;

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

Bus::Stats Bus::takeStats() {
    const Stats result = this->statistics;
    this->statistics = Stats();
    return result;
}

const Bus::RegisteredMapping *Bus::findMapping(
        quint16 address, AccessFlags requiredAccess) const {
    qsizetype &cachedIndex = requiredAccess == AccessFlags::Readable
        ? this->cachedReadMappingIndex
        : this->cachedWriteMappingIndex;
    if (cachedIndex >= 0 && cachedIndex < this->mappings.size()) {
        const RegisteredMapping &cached = this->mappings[cachedIndex];
        if (address >= cached.mapping.start && address <= cached.mapping.end) {
            if (hasAccessFlag(cached.mapping.flags, requiredAccess)) {
                if constexpr (NesDiagnostics::EnableBusStats) {
                    if (requiredAccess == AccessFlags::Readable) {
                        ++this->statistics.readCacheHits;
                    } else {
                        ++this->statistics.writeCacheHits;
                    }
                }
                return &cached;
            }
        }
    }

    if constexpr (NesDiagnostics::EnableBusStats) {
        if (requiredAccess == AccessFlags::Readable) {
            ++this->statistics.readCacheMisses;
        } else {
            ++this->statistics.writeCacheMisses;
        }
    }

    const RegisteredMapping *best = nullptr;
    for (qsizetype index = 0; index < this->mappings.size(); ++index) {
        if constexpr (NesDiagnostics::EnableBusStats) {
            ++this->statistics.mappingScanEntries;
        }
        const RegisteredMapping &registered = this->mappings[index];
        const Mapping &mapping = registered.mapping;
        if (address < mapping.start || address > mapping.end) {
            continue;
        }
        if (!hasAccessFlag(mapping.flags, requiredAccess)) {
            continue;
        }

        if (best == nullptr || mapping.priority > best->mapping.priority) {
            best = &registered;
            cachedIndex = index;
        }
    }
    return best;
}

bool Bus::overlaps(const Mapping &left, const Mapping &right) const {
    const bool addressOverlap = left.start <= right.end && right.start <= left.end;
    const quint8 sharedFlags = static_cast<quint8>(left.flags & right.flags);
    return addressOverlap && sharedFlags != 0;
}

bool Bus::hasMapping(quint16 address) const {
    for (const RegisteredMapping &registered : this->mappings) {
        if (address >= registered.mapping.start && address <= registered.mapping.end) {
            return true;
        }
    }
    return false;
}

void Bus::invalidateMappingCaches() {
    this->cachedReadMappingIndex = -1;
    this->cachedWriteMappingIndex = -1;
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
