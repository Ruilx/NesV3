#pragma once

#include "Bus.h"

class Ppu {
public:
	Ppu();

	[[nodiscard]] Bus &bus();
	[[nodiscard]] const Bus &bus() const;

private:
	Bus ppuBus;
};
