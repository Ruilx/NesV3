#pragma once

#include <cstdint>
#include <qglobal.h>

#include <QSysInfo>
#include <QException>

class Ram {
    quint8 *ram = nullptr;
    quint8 initValue = 0x00;
    size_t size = 0;
    bool isAllocated = false;

    void checkAddr(int addr, int bytes = 1) const;

    void allocMem();

public:
    explicit Ram(size_t size, quint8 initValue = 0x00);

    void set8(int addr, qint8 value);

    qint8 get8(int addr);

    void setU8(int addr, quint8 value);

    quint8 getU8(int addr);

    void set16(int addr, qint16 value, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    qint16 get16(int addr, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    void setU16(int addr, quint16 value, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    quint16 getU16(int addr, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    void set32(int addr, qint32 value, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    qint32 get32(int addr, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    void setU32(int addr, quint32 value, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    quint32 getU32(int addr, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    quint8 &operator[](int addr);

    const quint8 &operator[](int addr) const;
};
