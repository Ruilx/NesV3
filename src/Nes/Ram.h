#pragma once

#include <QByteArray>
#include <QSysInfo>
#include <QtGlobal>

class Ram {
    QByteArray ram;

    void checkAddr(qsizetype address, qsizetype bytes = 1) const;

public:
    explicit Ram(size_t size, quint8 initValue = 0x00);

    void set8(qsizetype address, qint8 value);

    [[nodiscard]] qint8 get8(qsizetype address) const;

    void setU8(qsizetype address, quint8 value);

    [[nodiscard]] quint8 getU8(qsizetype address) const;

    void set16(qsizetype address, qint16 value, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    [[nodiscard]] qint16 get16(qsizetype address, QSysInfo::Endian endian = QSysInfo::LittleEndian) const;

    void setU16(qsizetype address, quint16 value, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    [[nodiscard]] quint16 getU16(qsizetype address, QSysInfo::Endian endian = QSysInfo::LittleEndian) const;

    void set32(qsizetype address, qint32 value, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    [[nodiscard]] qint32 get32(qsizetype address, QSysInfo::Endian endian = QSysInfo::LittleEndian) const;

    void setU32(qsizetype address, quint32 value, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    [[nodiscard]] quint32 getU32(qsizetype address, QSysInfo::Endian endian = QSysInfo::LittleEndian) const;

    quint8 &operator[](qsizetype address);

    [[nodiscard]] const quint8 &operator[](qsizetype address) const;

    [[nodiscard]] size_t getSize() const;

    void clear(quint8 initValue = 0x00);

    [[nodiscard]] quint8 *data() {
        return reinterpret_cast<quint8 *>(this->ram.data());
    }

    [[nodiscard]] const quint8 *constData() const {
        return reinterpret_cast<const quint8 *>(this->ram.constData());
    }
};