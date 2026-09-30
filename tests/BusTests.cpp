#include "Bus.h"

#include <cstdlib>
#include <iostream>

namespace {
void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class ProbeDevice final : public BusDevice {
public:
    bool read(quint16 address, quint8 &value) override {
        this->lastReadAddress = address;
        value = this->readValue;
        ++this->readCount;
        return true;
    }

    bool write(quint16 address, quint8 value) override {
        this->lastWriteAddress = address;
        this->lastWriteValue = value;
        ++this->writeCount;
        return true;
    }

    quint16 lastReadAddress = 0;
    quint16 lastWriteAddress = 0;
    quint8 lastWriteValue = 0;
    quint8 readValue = 0x5A;
    int readCount = 0;
    int writeCount = 0;
};

void testReadWriteMappingsMayShareAddress() {
    Bus bus(0x10000);
    ProbeDevice reader;
    ProbeDevice writer;

    const Bus::Mapping readMapping{
        .start = 0x4017,
        .end = 0x4017,
        .flags = AccessFlags::Readable,
        .name = QStringLiteral("read mapping"),
        .device = &reader,
        .translate = [](quint16) { return static_cast<quint16>(0x17); },
    };
    const Bus::Mapping writeMapping{
        .start = 0x4017,
        .end = 0x4017,
        .flags = AccessFlags::Writable,
        .name = QStringLiteral("write mapping"),
        .device = &writer,
        .translate = [](quint16) { return static_cast<quint16>(0x17); },
    };

    check(bus.registerMapping(readMapping) != 0,
        "read mapping should register");
    check(bus.registerMapping(writeMapping) != 0,
        "write mapping should share an address with read mapping");

    quint8 value = 0;
    check(bus.read(0x4017, value) == Bus::AccessResult::Handled && value == 0x5A,
        "read should select the readable mapping");
    check(reader.lastReadAddress == 0x17,
        "read translator should receive the device address");
    check(bus.write(0x4017, 0xA5) == Bus::AccessResult::Handled,
        "write should select the writable mapping");
    check(writer.lastWriteAddress == 0x17 && writer.lastWriteValue == 0xA5,
        "write translator should receive the device address and value");
}

void testSameDirectionOverlapIsRejected() {
    Bus bus(0x10000);
    ProbeDevice first;
    ProbeDevice second;
    const Bus::Mapping readMapping{
        .start = 0x1000,
        .end = 0x10FF,
        .flags = AccessFlags::Readable,
        .name = QStringLiteral("first read mapping"),
        .device = &first,
    };
    const Bus::Mapping overlappingReadMapping{
        .start = 0x1080,
        .end = 0x117F,
        .flags = AccessFlags::Readable,
        .name = QStringLiteral("second read mapping"),
        .device = &second,
    };

    check(bus.registerMapping(readMapping) != 0,
        "first read mapping should register");
    check(bus.registerMapping(overlappingReadMapping) == 0,
        "same-direction overlap should be rejected");
}

void testPermissionAndCacheInvalidation() {
    Bus bus(0x10000);
    ProbeDevice device;
    const Bus::Mapping readMapping{
        .start = 0x2000,
        .end = 0x20FF,
        .flags = AccessFlags::Readable,
        .name = QStringLiteral("read-only mapping"),
        .device = &device,
    };
    const Bus::MappingId id = bus.registerMapping(readMapping);
    check(id != 0, "read-only mapping should register");

    quint8 value = 0;
    check(bus.write(0x2000, 0x11) == Bus::AccessResult::PermissionDenied,
        "write to read-only mapping should be denied");
    check(bus.read(0x2000, value) == Bus::AccessResult::Handled,
        "read-only mapping should be readable");
    check(bus.setMappingFlags(id, AccessFlags::None),
        "mapping flags should be changeable");
    check(bus.read(0x2000, value) == Bus::AccessResult::PermissionDenied,
        "changing mapping flags should invalidate the read cache");
}
}

int main() {
    testReadWriteMappingsMayShareAddress();
    testSameDirectionOverlapIsRejected();
    testPermissionAndCacheInvalidation();
    std::cout << "BusTests passed\n";
    return EXIT_SUCCESS;
}
