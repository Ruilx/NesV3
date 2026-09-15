#pragma once

#include "Bus.h"
#include "Ram.h"

#include <functional>

class Ppu final : public BusDevice {
public:
	using NmiCallback = std::function<void()>;
	enum class NametableMirroring : quint8 {
		Horizontal,
		Vertical,
		FourScreen,
	};

	Ppu();

	bool read(quint16 address, quint8 &value) override;
	bool write(quint16 address, quint8 value) override;

	void clock();
	void resetClock();
	void setNmiCallback(NmiCallback callback);
	void setNametableMirroring(NametableMirroring mirroring);
	[[nodiscard]] NametableMirroring nametableMirroring() const;
	[[nodiscard]] bool renderNametableTile(
			int nametable,
			int tileX,
			int tileY,
			QVector<quint8> &pixels,
			QVector<quint8> &subpalette);
	[[nodiscard]] quint64 totalTicks() const;
	[[nodiscard]] quint16 scanline() const;
	[[nodiscard]] quint16 dot() const;
	[[nodiscard]] quint64 frame() const;

	[[nodiscard]] Bus &bus();
	[[nodiscard]] const Bus &bus() const;
	void reset();

private:
	Bus ppuBus;
	Ram nametableRam;
	Ram paletteRam;
	RamBusDevice nametableDevice;
	RamBusDevice paletteDevice;
	Ram oam;

	quint8 control = 0;
	quint8 mask = 0;
	quint8 status = 0;
	quint8 oamAddress = 0;
	quint16 currentAddress = 0;
	quint16 temporaryAddress = 0;
	quint8 fineX = 0;
	bool writeToggle = false;
	quint8 readBuffer = 0;
	quint64 totalTicksValue = 0;
	quint16 scanlineValue = 0;
	quint16 dotValue = 0;
	quint64 frameValue = 0;
	NmiCallback nmiCallback;
	NametableMirroring nametableMirroringValue = NametableMirroring::Horizontal;

	void incrementAddress();
	void advanceTiming();
	[[nodiscard]] quint16 translateNametableAddress(quint16 address) const;
};
