#include "Ppu.h"

#include "ChrTileDecoder.h"

Ppu::Ppu()
	: ppuBus(0x4000),
	  nametableRam(0x1000),
	  paletteRam(0x0020),
	  nametableDevice(this->nametableRam),
	  paletteDevice(this->paletteRam),
	  oam(0x0100) {
	const Bus::Mapping nametableMapping{
		.start = 0x2000,
		.end = 0x3EFF,
		.priority = 0,
		.flags = AccessFlags::Readable | AccessFlags::Writable,
		.name = QStringLiteral("PPU nametable RAM"),
		.device = &this->nametableDevice,
		.translate = [this](quint16 address) {
			return this->translateNametableAddress(address);
		},
	};
	const Bus::Mapping paletteMapping{
		.start = 0x3F00,
		.end = 0x3FFF,
		.priority = 0,
		.flags = AccessFlags::Readable | AccessFlags::Writable,
		.name = QStringLiteral("PPU palette RAM"),
		.device = &this->paletteDevice,
		.translate = [](quint16 address) {
			quint16 index = static_cast<quint16>(address & 0x001F);
			if ((index & 0x0013) == 0x0010) {
				index = static_cast<quint16>(index & 0x000F);
			}
			return index;
		},
	};
	this->ppuBus.registerMapping(nametableMapping);
	this->ppuBus.registerMapping(paletteMapping);
}

bool Ppu::read(quint16 address, quint8 &value) {
	switch (address & 0x0007) {
	case 0x0002:
		value = this->status;
		this->status &= static_cast<quint8>(~0x80);
		this->writeToggle = false;
		return true;
	case 0x0004:
		value = this->oam.getU8(this->oamAddress);
		return true;
	case 0x0007: {
		quint8 fetched = this->ppuBus.openBusValue();
		this->ppuBus.read(this->currentAddress & 0x3FFF, fetched);
		value = this->readBuffer;
		this->readBuffer = fetched;
		if ((this->currentAddress & 0x3FFF) >= 0x3F00) {
			value = fetched;
		}
		this->incrementAddress();
		return true;
	}
	default:
		value = this->ppuBus.openBusValue();
		return true;
	}
}

bool Ppu::write(quint16 address, quint8 value) {
	switch (address & 0x0007) {
	case 0x0000:
		if ((this->control & 0x80) == 0 && (value & 0x80) != 0 &&
			(this->status & 0x80) != 0 && this->nmiCallback) {
			this->nmiCallback();
		}
		this->control = value;
		this->temporaryAddress = static_cast<quint16>(
			(this->temporaryAddress & 0xF3FF) | ((value & 0x03) << 10));
		return true;
	case 0x0001:
		this->mask = value;
		return true;
	case 0x0003:
		this->oamAddress = value;
		return true;
	case 0x0004:
		this->oam.setU8(this->oamAddress, value);
		++this->oamAddress;
		return true;
	case 0x0005:
		if (!this->writeToggle) {
			this->fineX = value & 0x07;
			this->temporaryAddress = static_cast<quint16>(
				(this->temporaryAddress & 0xFFE0) | (value >> 3));
			this->writeToggle = true;
		} else {
			this->temporaryAddress = static_cast<quint16>(
				(this->temporaryAddress & 0x8FFF) | ((value & 0x07) << 12));
			this->temporaryAddress = static_cast<quint16>(
				(this->temporaryAddress & 0xFC1F) | ((value & 0xF8) << 2));
			this->writeToggle = false;
		}
		return true;
	case 0x0006:
		if (!this->writeToggle) {
			this->temporaryAddress = static_cast<quint16>(
				(this->temporaryAddress & 0x00FF) | ((value & 0x3F) << 8));
			this->writeToggle = true;
		} else {
			this->temporaryAddress = static_cast<quint16>(
				(this->temporaryAddress & 0xFF00) | value);
			this->currentAddress = this->temporaryAddress;
			this->writeToggle = false;
		}
		return true;
	case 0x0007:
		this->ppuBus.write(this->currentAddress & 0x3FFF, value);
		this->incrementAddress();
		return true;
	default:
		return true;
	}
}

void Ppu::incrementAddress() {
	this->currentAddress = static_cast<quint16>(
		(this->currentAddress + ((this->control & 0x04) ? 32 : 1)) & 0x3FFF);
}

void Ppu::clock() {
	++this->totalTicksValue;
	this->advanceTiming();
}

void Ppu::resetClock() {
	this->totalTicksValue = 0;
	this->scanlineValue = 0;
	this->dotValue = 0;
	this->frameValue = 0;
}

void Ppu::setNmiCallback(NmiCallback callback) {
	this->nmiCallback = std::move(callback);
}

void Ppu::setNametableMirroring(NametableMirroring mirroring) {
	this->nametableMirroringValue = mirroring;
}

Ppu::NametableMirroring Ppu::nametableMirroring() const {
	return this->nametableMirroringValue;
}

bool Ppu::renderNametableTile(
		int nametable,
		int tileX,
		int tileY,
		QVector<quint8> &pixels,
		QVector<quint8> &subpalette) {
	if (nametable < 0 || nametable >= 4 || tileX < 0 || tileX >= 32
			|| tileY < 0 || tileY >= 30) {
		pixels.clear();
		subpalette.clear();
		return false;
	}

	const quint16 nametableBase = static_cast<quint16>(
			0x2000 + nametable * 0x0400);
	quint8 tileIndex = 0;
	quint8 attribute = 0;
	quint8 value = 0;
	if (this->ppuBus.read(
			nametableBase + static_cast<quint16>(tileY * 32 + tileX), value)
				!= Bus::AccessResult::Handled) {
		pixels.clear();
		subpalette.clear();
		return false;
	}
	tileIndex = value;
	if (this->ppuBus.read(
			nametableBase + 0x03C0
				+ static_cast<quint16>((tileY / 4) * 8 + tileX / 4), value)
				!= Bus::AccessResult::Handled) {
		pixels.clear();
		subpalette.clear();
		return false;
	}
	attribute = value;
	const int quadrant = ((tileY & 0x02) != 0 ? 2 : 0)
			+ ((tileX & 0x02) != 0 ? 1 : 0);
	const quint8 paletteNumber = static_cast<quint8>(
			(attribute >> (quadrant * 2)) & 0x03);

	subpalette.resize(4);
	for (int index = 0; index < 4; ++index) {
		const quint16 paletteAddress = index == 0
				? 0x3F00
				: static_cast<quint16>(0x3F00 + paletteNumber * 4 + index);
		if (this->ppuBus.read(
				paletteAddress, value)
					!= Bus::AccessResult::Handled) {
			pixels.clear();
			subpalette.clear();
			return false;
		}
		subpalette[index] = value;
	}

	const ChrTileDecoder::PatternTable patternTable =
			(this->control & 0x10) != 0
			? ChrTileDecoder::PatternTable::Upper
			: ChrTileDecoder::PatternTable::Lower;
	return ChrTileDecoder::decodeTile(
			this->ppuBus, tileIndex, patternTable, pixels);
}

void Ppu::reset() {
	this->control = 0;
	this->mask = 0;
	this->status = 0;
	this->oamAddress = 0;
	this->currentAddress = 0;
	this->temporaryAddress = 0;
	this->fineX = 0;
	this->writeToggle = false;
	this->readBuffer = 0;
	this->resetClock();
}

quint64 Ppu::totalTicks() const {
	return this->totalTicksValue;
}

quint16 Ppu::scanline() const {
	return this->scanlineValue;
}

quint16 Ppu::dot() const {
	return this->dotValue;
}

quint64 Ppu::frame() const {
	return this->frameValue;
}

void Ppu::advanceTiming() {
	++this->dotValue;
	if (this->dotValue != 341) {
		if (this->scanlineValue == 241 && this->dotValue == 1) {
			this->status |= 0x80;
			if ((this->control & 0x80) != 0 && this->nmiCallback) {
				this->nmiCallback();
			}
		} else if (this->scanlineValue == 261 && this->dotValue == 1) {
			this->status &= static_cast<quint8>(~0x80);
			this->status &= static_cast<quint8>(~0x40);
			this->status &= static_cast<quint8>(~0x20);
		}
		return;
	}

	this->dotValue = 0;
	++this->scanlineValue;
	if (this->scanlineValue == 262) {
		this->scanlineValue = 0;
		++this->frameValue;
	}
}

Bus &Ppu::bus() {
	return this->ppuBus;
}

const Bus &Ppu::bus() const {
	return this->ppuBus;
}

quint16 Ppu::translateNametableAddress(quint16 address) const {
	const quint16 offset = static_cast<quint16>((address - 0x2000) & 0x1FFF);
	const quint16 table = static_cast<quint16>(offset / 0x0400);
	const quint16 inner = static_cast<quint16>(offset & 0x03FF);
	quint16 physicalTable = table;
	if (this->nametableMirroringValue == NametableMirroring::Vertical) {
		physicalTable = static_cast<quint16>(table & 0x01);
	} else if (this->nametableMirroringValue == NametableMirroring::Horizontal) {
		physicalTable = static_cast<quint16>((table >> 1) & 0x01);
	}
	return static_cast<quint16>(physicalTable * 0x0400 + inner);
}
