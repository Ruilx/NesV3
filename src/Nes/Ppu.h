#pragma once

#include "Bus.h"
#include "Ram.h"

#include <array>
#include <functional>
#include <QVector>

class Ppu final : public BusDevice {
public:
	using NmiCallback = std::function<void()>;
	struct DirtyTile {
		quint8 nametable = 0;
		quint8 tileX = 0;
		quint8 tileY = 0;
	};
	struct WriteStats {
		quint64 controlWrites = 0;
		quint64 maskWrites = 0;
		quint64 oamAddressWrites = 0;
		quint64 oamDataWrites = 0;
		quint64 scrollWrites = 0;
		quint64 addressWrites = 0;
		quint64 dataWrites = 0;
		quint64 chrWrites = 0;
		quint64 nametableWrites = 0;
		quint64 paletteWrites = 0;
		quint16 lastMemoryAddress = 0;
	};
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
	[[nodiscard]] QVector<DirtyTile> takeDirtyTiles();
	[[nodiscard]] WriteStats takeWriteStats();
	void invalidateAllTiles();
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
	std::array<bool, 4 * 32 * 30> dirtyTiles{};
	WriteStats writeStats;

	void incrementAddress();
	void advanceTiming();
	void markMemoryWrite(quint16 address);
	void markAllTilesDirty();
	void markNametableTileDirty(quint16 address);
	void markAttributeDirty(quint16 address);
	void markLogicalTileDirty(int nametable, int tileX, int tileY);
	[[nodiscard]] int dirtyTileIndex(int nametable, int tileX, int tileY) const;
	[[nodiscard]] quint16 translateNametableAddress(quint16 address) const;
};
