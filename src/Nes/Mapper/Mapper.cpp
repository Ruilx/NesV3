#include "Mapper.h"

MapperCpuDevice::MapperCpuDevice(Mapper &mapper)
    : mapper(mapper) {
}

bool MapperCpuDevice::read(quint16 address, quint8 &value) {
    return this->mapper.readCpu(address, value);
}

bool MapperCpuDevice::write(quint16 address, quint8 value) {
    return this->mapper.writeCpu(address, value);
}

MapperPpuDevice::MapperPpuDevice(Mapper &mapper)
    : mapper(mapper) {
}

bool MapperPpuDevice::read(quint16 address, quint8 &value) {
    return this->mapper.readPpu(address, value);
}

bool MapperPpuDevice::write(quint16 address, quint8 value) {
    return this->mapper.writePpu(address, value);
}
