#include "Mapper.h"

MapperCpuDevice::MapperCpuDevice(Mapper *mapper)
    : mapper(mapper) {
}

void MapperCpuDevice::setMapper(Mapper *mapper) {
    this->mapper = mapper;
}

bool MapperCpuDevice::read(quint16 address, quint8 &value) {
    return this->mapper != nullptr && this->mapper->readCpu(address, value);
}

bool MapperCpuDevice::write(quint16 address, quint8 value) {
    return this->mapper != nullptr && this->mapper->writeCpu(address, value);
}

MapperPpuDevice::MapperPpuDevice(Mapper *mapper)
    : mapper(mapper) {
}

void MapperPpuDevice::setMapper(Mapper *mapper) {
    this->mapper = mapper;
}

bool MapperPpuDevice::read(quint16 address, quint8 &value) {
    return this->mapper != nullptr && this->mapper->readPpu(address, value);
}

bool MapperPpuDevice::write(quint16 address, quint8 value) {
    return this->mapper != nullptr && this->mapper->writePpu(address, value);
}
