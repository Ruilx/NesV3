#pragma once

#include "Bus.h"

class Ppu {
public:
	Ppu();

	void clock();
	void resetClock();
	[[nodiscard]] quint64 totalTicks() const;

	[[nodiscard]] Bus &bus();
	[[nodiscard]] const Bus &bus() const;

private:
	Bus ppuBus;
	quint64 totalTicksValue = 0;
};
