#include "Ppu.h"

Ppu::Ppu()
	: ppuBus(0x4000) {
}

Bus &Ppu::bus() {
	return this->ppuBus;
}

const Bus &Ppu::bus() const {
	return this->ppuBus;
}
