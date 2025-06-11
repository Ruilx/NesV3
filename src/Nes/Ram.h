#pragma once

#include <cstdint>
#include <qglobal.h>

#include <QSysInfo>
#include <QException>

class RamIterator;
class Ram {
    quint8 *ram = nullptr;
    quint8 initValue = 0x00;
    size_t size = 0;
    bool allocated = false;

    void checkAddr(int addr, int bytes = 1) const;

    void allocMem();

public:
    explicit Ram(size_t size, quint8 initValue = 0x00);

    void set8(int addr, qint8 value);

    [[nodiscard]] qint8 get8(int addr);

    void setU8(int addr, quint8 value);

    [[nodiscard]] quint8 getU8(int addr);

    void set16(int addr, qint16 value, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    [[nodiscard]] qint16 get16(int addr, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    void setU16(int addr, quint16 value, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    [[nodiscard]] quint16 getU16(int addr, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    void set32(int addr, qint32 value, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    [[nodiscard]] qint32 get32(int addr, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    void setU32(int addr, quint32 value, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    [[nodiscard]] quint32 getU32(int addr, QSysInfo::Endian endian = QSysInfo::LittleEndian);

    quint8 &operator[](int addr);

    [[nodiscard]] const quint8 &operator[](int addr) const;

    [[nodiscard]] size_t getSize() const;

    ~Ram();

    [[nodiscard]] bool isAllocated() const;

    void clear(quint8 initValue = 0x00);

    [[nodiscard]] RamIterator *iterator();

    [[nodiscard]] const RamIterator *iterator() const;

    [[nodiscard]] quint8 *data(){
        return this->ram;
    }

    [[nodiscard]] const quint8 *constData() const{
        return this->ram;
    }
};

class RamIterator {
    Ram *ram = nullptr;
    quint8 *pointer = nullptr;
    quint8 *pointerEnd = nullptr;
    size_t size = 0;

    bool checkAllocated();

    void setRam(Ram *r);
public:
    explicit RamIterator(Ram *ram) {
        this->setRam(ram);
    }

    void seek(int offset, int whence = SEEK_SET);

    inline void operator++(){
        this->seek(1, SEEK_CUR);
    }

    inline void operator--(){
        this->seek(-1, SEEK_CUR);
    }

    [[nodiscard]] inline bool hasNext() const{
        return this->pointer < this->pointerEnd;
    }
    [[nodiscard]] inline bool hasPrev() const{
        return this->pointer > this->ram->data();
    }
    quint8 next(){
        if(this->hasNext()){
            return *(this->pointer++);
        }
        return 0;
    }
    quint8 prev(){
        if(this->hasPrev()){
            return *(this->pointer--);
        }
        return 0;
    }

    quint8 &operator*();
};

