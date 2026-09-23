#pragma once

#include "Bus.h"
#include "Ram.h"

#include <array>
#include <functional>
#include <QVector>

class Ppu final : public BusDevice {
public:
	using NmiCallback = std::function<void()>;
	using OamDmaCallback = std::function<void(quint8)>;
	enum class SpriteLimitMode : quint8 {
		HardwareAccurate,
		Unlimited,
	};
	struct SpriteEntry {
		quint8 index = 0;
		quint8 y = 0;
		quint8 tile = 0;
		quint8 attributes = 0;
		quint8 x = 0;
	};
	struct SpriteEvaluation {
		QVector<SpriteEntry> sprites;
		bool overflow = false;
	};
	struct SpriteRender {
		quint8 width = 0;
		quint8 height = 0;
		QVector<quint8> pixels;
		QVector<quint8> paletteIndices;
	};
	struct SpritePixel {
		quint8 paletteIndex = 0;
		bool spriteOpaque = false;
		bool sprite0Hit = false;
	};
	struct SpriteOutput {
		SpriteEntry entry;
		SpriteRender render;
		quint8 screenX = 0;
		quint16 screenY = 0;
	};
	struct BackgroundFrame {
		QVector<quint8> paletteIndices;
		quint16 width = 256;
		quint16 height = 240;
	};
	struct ScrollSnapshot {
		int x = 0;
		int y = 0;
	};
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
		quint64 sprite0HitChecks = 0;
		quint64 sprite0HitEvaluations = 0;
		quint64 sprite0HitRenders = 0;
		quint64 sprite0HitSamples = 0;
		quint64 sprite0Hits = 0;
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
	void setOamDmaCallback(OamDmaCallback callback);
	void setSpriteLimitMode(SpriteLimitMode mode);
	[[nodiscard]] SpriteLimitMode spriteLimitMode() const;
	[[nodiscard]] SpriteEvaluation evaluateSpritesForScanline(
		quint16 scanline);
	[[nodiscard]] bool renderSprite(
		const SpriteEntry &sprite,
		SpriteRender &renderedSprite);
	[[nodiscard]] QVector<SpriteOutput> renderSpritesForFrame();
	[[nodiscard]] SpritePixel composeSpritePixel(
		quint8 backgroundPixel,
		quint8 backgroundPaletteIndex,
		quint8 spritePixel,
		quint8 spritePaletteIndex,
		bool spriteBehindBackground,
		bool spriteZero) const;
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
	[[nodiscard]] bool renderBackgroundFrame(BackgroundFrame &frame);
	[[nodiscard]] ScrollSnapshot scrollSnapshot() const;
	[[nodiscard]] QVector<ScrollSnapshot> rasterScroll() const;
	[[nodiscard]] quint64 totalTicks() const;
	[[nodiscard]] quint16 scanline() const;
	[[nodiscard]] quint16 dot() const;
	[[nodiscard]] quint64 frame() const;
	[[nodiscard]] quint8 universalBackgroundColor() const;

	[[nodiscard]] Bus &bus();
	[[nodiscard]] const Bus &bus() const;
	void writeOamDmaByte(quint8 value);
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
	QVector<ScrollSnapshot> rasterScrollValue;
	NmiCallback nmiCallback;
	OamDmaCallback oamDmaCallback;
	SpriteLimitMode spriteLimitModeValue = SpriteLimitMode::HardwareAccurate;
	NametableMirroring nametableMirroringValue = NametableMirroring::Horizontal;
	std::array<bool, 4 * 32 * 30> dirtyTiles{};
	WriteStats writeStats;
	quint16 sprite0EvaluationScanline = 0;
	bool sprite0EvaluationValid = false;
	bool sprite0Present = false;
	SpriteEntry cachedSprite0;
	bool sprite0RenderValid = false;
	SpriteRender cachedSprite0Render;

	void incrementAddress();
	void advanceTiming();
	void markMemoryWrite(quint16 address);
	void markAllTilesDirty();
	void markNametableTileDirty(quint16 address);
	void markAttributeDirty(quint16 address);
	void markLogicalTileDirty(int nametable, int tileX, int tileY);
	void invalidateSprite0Cache();
	void checkSprite0Hit(quint16 scanline, quint16 dot);
	[[nodiscard]] bool sampleBackgroundPixel(
		int screenX,
		int screenY,
		quint8 &pixel,
		quint8 &paletteIndex);
	[[nodiscard]] int dirtyTileIndex(int nametable, int tileX, int tileY) const;
	[[nodiscard]] quint16 translateNametableAddress(quint16 address) const;
};
