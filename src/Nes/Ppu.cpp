#include "Ppu.h"

Ppu::Ppu()
	: ppuBus(0x4000) {
}

void Ppu::clock() {
	++this->totalTicksValue;
}

void Ppu::resetClock() {
	this->totalTicksValue = 0;
}

quint64 Ppu::totalTicks() const {
	return this->totalTicksValue;
}

Bus &Ppu::bus() {
	return this->ppuBus;
}

const Bus &Ppu::bus() const {
	return this->ppuBus;
}
