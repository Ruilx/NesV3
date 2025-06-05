#include "Ram.h"

#include "../Excption/OutOfRangeException.h"
#include "../Excption/ValueError.h"

void Ram::checkAddr(int addr, int bytes) const {
	if (addr < 0 || (addr + bytes - 1) >= this->size) {
		throw OutOfRangeException(QString("Ram address out of range: %1 to %2").arg(addr).arg(addr + bytes - 1));
	}
}

Ram::Ram(size_t size, quint8 initValue) {
    if(size <= 0){
        throw ValueError("Ram size must be greater than 0");
    }
    this->size = size;
    this->ram = new quint8[size];
    memset(this->ram, initValue, this->size);
}

void Ram::set8(int addr, qint8 value) {
	this->checkAddr(addr);
	this->ram[addr] = quint8(value);
}

qint8 Ram::get8(int addr) {
	this->checkAddr(addr);
	return qint8(this->ram[addr]);
}

void Ram::setU8(int addr, quint8 value) {
	this->checkAddr(addr);
	this->ram[addr] = value;
}

quint8 Ram::getU8(int addr) {
	this->checkAddr(addr);
	return ram[addr];
}

void Ram::set16(int addr, qint16 value, QSysInfo::Endian endian) {
	this->checkAddr(addr, 2);
	if (endian == QSysInfo::LittleEndian) {
		this->ram[addr] = quint8(value & 0xFF);
		this->ram[addr + 1] = quint8((value >> 8) & 0xFF);
	} else {
		this->ram[addr] = quint8((value >> 8) & 0xFF);
		this->ram[addr + 1] = quint8(value & 0xFF);
	}
}

qint16 Ram::get16(int addr, QSysInfo::Endian endian) {
	this->checkAddr(addr, 2);
	if (endian == QSysInfo::LittleEndian) {
		return qint16((this->ram[addr] & 0xFF) | ((this->ram[addr + 1] & 0xFF) << 8));
	} else {
		return qint16(((this->ram[addr + 1] & 0xFF) << 8) | (this->ram[addr] & 0xFF));
	}

}

void Ram::setU16(int addr, quint16 value, QSysInfo::Endian endian) {
	this->checkAddr(addr, 2);
	if (endian == QSysInfo::LittleEndian) {
		this->ram[addr] = quint8(value & 0xFF);
		this->ram[addr + 1] = quint8((value >> 8) & 0xFF);
	} else {
		this->ram[addr] = quint8((value >> 8) & 0xFF);
		this->ram[addr + 1] = quint8(value & 0xFF);
	}
}

quint16 Ram::getU16(int addr, QSysInfo::Endian endian) {
	this->checkAddr(addr, 2);
	if (endian == QSysInfo::LittleEndian) {
		return quint16((this->ram[addr] & 0xFF) | ((this->ram[addr + 1] & 0xFF) << 8));
	} else {
		return quint16(((this->ram[addr + 1] & 0xFF) << 8) | (this->ram[addr] & 0xFF));
	}
}

void Ram::set32(int addr, qint32 value, QSysInfo::Endian endian) {
	this->checkAddr(addr, 4);
	if (endian == QSysInfo::LittleEndian) {
		this->ram[addr] = quint8(value & 0xFF);
		this->ram[addr + 1] = quint8((value >> 8) & 0xFF);
		this->ram[addr + 2] = quint8((value >> 16) & 0xFF);
		this->ram[addr + 3] = quint8((value >> 24) & 0xFF);
	} else {
		this->ram[addr] = quint8((value >> 24) & 0xFF);
		this->ram[addr + 1] = quint8((value >> 16) & 0xFF);
		this->ram[addr + 2] = quint8((value >> 8) & 0xFF);
		this->ram[addr + 3] = quint8(value & 0xFF);
	}
}

qint32 Ram::get32(int addr, QSysInfo::Endian endian) {
	this->checkAddr(addr, 4);
	if (endian == QSysInfo::LittleEndian) {
		return qint32(
				(this->ram[addr] & 0xFF) |
				((this->ram[addr + 1] & 0xFF) << 8) |
				((this->ram[addr + 2] & 0xFF) << 16) |
				((this->ram[addr + 3] & 0xFF) << 24)
		);
	} else {
		return qint32(
				((this->ram[addr + 3] & 0xFF) << 24) |
				((this->ram[addr + 2] & 0xFF) << 16) |
				((this->ram[addr + 1] & 0xFF) << 8) |
				(this->ram[addr] & 0xFF)
		);
	}
}

void Ram::setU32(int addr, quint32 value, QSysInfo::Endian endian) {
	this->checkAddr(addr, 4);
	if (endian == QSysInfo::LittleEndian) {
		this->ram[addr] = quint8(value & 0xFF);
		this->ram[addr + 1] = quint8((value >> 8) & 0xFF);
		this->ram[addr + 2] = quint8((value >> 16) & 0xFF);
		this->ram[addr + 3] = quint8((value >> 24) & 0xFF);
	} else {
		this->ram[addr] = quint8((value >> 24) & 0xFF);
		this->ram[addr + 1] = quint8((value >> 16) & 0xFF);
		this->ram[addr + 2] = quint8((value >> 8) & 0xFF);
		this->ram[addr + 3] = quint8(value & 0xFF);
	}
}

quint32 Ram::getU32(int addr, QSysInfo::Endian endian) {
	this->checkAddr(addr, 4);
	if (endian == QSysInfo::LittleEndian) {
		return quint32(
				(this->ram[addr] & 0xFF) |
				((this->ram[addr + 1] & 0xFF) << 8) |
				((this->ram[addr + 2] & 0xFF) << 16) |
				((this->ram[addr + 3] & 0xFF) << 24)
		);
		)else{
			return quint32(
					((this->ram[addr + 3] & 0xFF) << 24) |
					((this->ram[addr + 2] & 0xFF) << 16) |
					((this->ram[addr + 1] & 0xFF) << 8) |
					(this->ram[addr] & 0xFF)
			)
		}
	}
}

const quint8 &Ram::operator[](int addr) const {
	this->checkAddr(addr);
	return this->ram[addr];
}

quint8 &Ram::operator[](int addr){
	this->checkAddr(addr);
	return this->ram[addr];
}
