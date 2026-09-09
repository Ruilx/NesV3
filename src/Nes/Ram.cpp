#include "Ram.h"

#include "../Excption/OutOfRangeError.h"
#include "../Excption/ValueError.h"
#include <QByteArray>

namespace {
quint32 readUnsigned(const QByteArray &bytes, qsizetype address, qsizetype width, QSysInfo::Endian endian) {
    quint32 value = 0;
    if (endian == QSysInfo::LittleEndian) {
        for (qsizetype index = width - 1; index >= 0; --index) {
            value = (value << 8) | static_cast<quint8>(bytes.at(address + index));
        }
    } else {
        for (qsizetype index = 0; index < width; ++index) {
            value = (value << 8) | static_cast<quint8>(bytes.at(address + index));
        }
    }
    return value;
}

void writeUnsigned(QByteArray &bytes, qsizetype address, quint32 value, qsizetype width, QSysInfo::Endian endian) {
    for (qsizetype index = 0; index < width; ++index) {
        const qsizetype destination = endian == QSysInfo::LittleEndian
            ? address + index
            : address + width - index - 1;
        bytes[destination] = static_cast<char>(value >> (index * 8));
    }
}
}

Ram::Ram(size_t size, quint8 initValue)
    : ram(static_cast<qsizetype>(size), static_cast<char>(initValue)) {
    if (size == 0) {
        throw ValueError("Ram size must be greater than 0");
    }
}

void Ram::checkAddr(qsizetype address, qsizetype bytes) const {
    if (address < 0 || bytes <= 0 || address > this->ram.size() - bytes) {
        throw OutOfRangeError(QString("Ram address out of range: %1 to %2")
                                  .arg(address)
                                  .arg(address + bytes - 1));
    }
}

void Ram::set8(qsizetype address, qint8 value) {
    this->setU8(address, static_cast<quint8>(value));
}

qint8 Ram::get8(qsizetype address) const {
    return static_cast<qint8>(this->getU8(address));
}

void Ram::setU8(qsizetype address, quint8 value) {
    this->checkAddr(address);
    this->ram[address] = static_cast<char>(value);
}

quint8 Ram::getU8(qsizetype address) const {
    this->checkAddr(address);
    return static_cast<quint8>(this->ram.at(address));
}

void Ram::set16(qsizetype address, qint16 value, QSysInfo::Endian endian) {
    this->setU16(address, static_cast<quint16>(value), endian);
}

qint16 Ram::get16(qsizetype address, QSysInfo::Endian endian) const {
    return static_cast<qint16>(this->getU16(address, endian));
}

void Ram::setU16(qsizetype address, quint16 value, QSysInfo::Endian endian) {
    this->checkAddr(address, sizeof(value));
    writeUnsigned(this->ram, address, value, sizeof(value), endian);
}

quint16 Ram::getU16(qsizetype address, QSysInfo::Endian endian) const {
    this->checkAddr(address, sizeof(quint16));
    return static_cast<quint16>(readUnsigned(this->ram, address, sizeof(quint16), endian));
}

void Ram::set32(qsizetype address, qint32 value, QSysInfo::Endian endian) {
    this->setU32(address, static_cast<quint32>(value), endian);
}

qint32 Ram::get32(qsizetype address, QSysInfo::Endian endian) const {
    return static_cast<qint32>(this->getU32(address, endian));
}

void Ram::setU32(qsizetype address, quint32 value, QSysInfo::Endian endian) {
    this->checkAddr(address, sizeof(value));
    writeUnsigned(this->ram, address, value, sizeof(value), endian);
}

quint32 Ram::getU32(qsizetype address, QSysInfo::Endian endian) const {
    this->checkAddr(address, sizeof(quint32));
    return readUnsigned(this->ram, address, sizeof(quint32), endian);
}

const quint8 &Ram::operator[](qsizetype address) const {
    this->checkAddr(address);
    return reinterpret_cast<const quint8 *>(this->ram.constData())[address];
}

quint8 &Ram::operator[](qsizetype address) {
    this->checkAddr(address);
    return reinterpret_cast<quint8 *>(this->ram.data())[address];
}

size_t Ram::getSize() const {
    return static_cast<size_t>(this->ram.size());
}

void Ram::clear(quint8 value) {
    this->ram.fill(static_cast<char>(value));
}
