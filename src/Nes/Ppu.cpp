#include "Ppu.h"

#include "ChrTileDecoder.h"

#include <algorithm>

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
	this->markAllTilesDirty();
	this->rasterScrollValue.resize(240);
	const ScrollSnapshot initialScroll = this->scrollSnapshot();
	std::fill(this->rasterScrollValue.begin(), this->rasterScrollValue.end(), initialScroll);
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
	if (address == 0x4014) {
		if (this->oamDmaCallback) {
			this->oamDmaCallback(value);
		}
		return true;
	}

	switch (address & 0x0007) {
	case 0x0000:
	{
		++this->writeStats.controlWrites;
		if ((this->control & 0x80) == 0 && (value & 0x80) != 0 &&
			(this->status & 0x80) != 0 && this->nmiCallback) {
			this->nmiCallback();
		}
		const bool patternTableChanged = (this->control & 0x10) != (value & 0x10);
		this->control = value;
		this->invalidateSprite0Cache();
		this->temporaryAddress = static_cast<quint16>(
			(this->temporaryAddress & 0xF3FF) | ((value & 0x03) << 10));
		if (patternTableChanged) {
			this->markAllTilesDirty();
		}
		return true;
	}
	case 0x0001:
		++this->writeStats.maskWrites;
		this->mask = value;
		return true;
	case 0x0003:
		++this->writeStats.oamAddressWrites;
		this->oamAddress = value;
		return true;
	case 0x0004:
		++this->writeStats.oamDataWrites;
		this->oam.setU8(this->oamAddress, value);
		this->invalidateSprite0Cache();
		++this->oamAddress;
		return true;
	case 0x0005:
		++this->writeStats.scrollWrites;
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
		++this->writeStats.addressWrites;
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
		{
			const quint16 address = static_cast<quint16>(this->currentAddress & 0x3FFF);
			if (this->ppuBus.write(address, value) == Bus::AccessResult::Handled) {
				++this->writeStats.dataWrites;
				this->writeStats.lastMemoryAddress = address;
				if (address < 0x2000) {
					++this->writeStats.chrWrites;
					this->sprite0RenderValid = false;
				} else if (address < 0x3F00) {
					++this->writeStats.nametableWrites;
				} else {
					++this->writeStats.paletteWrites;
				}
				this->markMemoryWrite(address);
			}
		}
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
	if (this->rasterScrollValue.size() != 240) {
		this->rasterScrollValue.resize(240);
	}
	const ScrollSnapshot initialScroll = this->scrollSnapshot();
	std::fill(this->rasterScrollValue.begin(), this->rasterScrollValue.end(), initialScroll);
}

void Ppu::setNmiCallback(NmiCallback callback) {
	this->nmiCallback = std::move(callback);
}

void Ppu::setOamDmaCallback(OamDmaCallback callback) {
	this->oamDmaCallback = std::move(callback);
}

void Ppu::setSpriteLimitMode(SpriteLimitMode mode) {
	this->spriteLimitModeValue = mode;
}

Ppu::SpriteLimitMode Ppu::spriteLimitMode() const {
	return this->spriteLimitModeValue;
}

Ppu::SpriteEvaluation Ppu::evaluateSpritesForScanline(quint16 scanline) {
	SpriteEvaluation result;
	if (scanline >= 240) {
		return result;
	}

	const int spriteHeight = (this->control & 0x20) != 0 ? 16 : 8;
	const int maximumSprites = this->spriteLimitModeValue == SpriteLimitMode::Unlimited
		? 64
		: 8;
	for (int index = 0; index < 64; ++index) {
		const quint16 address = static_cast<quint16>(index * 4);
		const int top = static_cast<int>(this->oam.getU8(address)) + 1;
		if (static_cast<int>(scanline) < top
			|| static_cast<int>(scanline) >= top + spriteHeight) {
			continue;
		}

		if (static_cast<int>(result.sprites.size()) >= maximumSprites) {
			result.overflow = true;
			continue;
		}

		result.sprites.append({
			.index = static_cast<quint8>(index),
			.y = this->oam.getU8(address),
			.tile = this->oam.getU8(static_cast<quint16>(address + 1)),
			.attributes = this->oam.getU8(static_cast<quint16>(address + 2)),
			.x = this->oam.getU8(static_cast<quint16>(address + 3)),
		});
	}
	if (result.overflow && this->spriteLimitModeValue == SpriteLimitMode::HardwareAccurate) {
		this->status |= 0x20;
	}
	return result;
}

bool Ppu::renderSprite(
	const SpriteEntry &sprite,
	SpriteRender &renderedSprite) {
	const bool tallSprite = (this->control & 0x20) != 0;
	const int width = 8;
	const int height = tallSprite ? 16 : 8;
	const bool flipHorizontal = (sprite.attributes & 0x40) != 0;
	const bool flipVertical = (sprite.attributes & 0x80) != 0;
	const quint8 paletteNumber = static_cast<quint8>(sprite.attributes & 0x03);
	QVector<quint8> firstTile;
	QVector<quint8> secondTile;

	if (tallSprite) {
		const ChrTileDecoder::PatternTable patternTable =
			(sprite.tile & 0x01) != 0
			? ChrTileDecoder::PatternTable::Upper
			: ChrTileDecoder::PatternTable::Lower;
		const quint16 tileIndex = static_cast<quint16>(sprite.tile & 0xFE);
		if (!ChrTileDecoder::decodeTile(
				this->ppuBus, tileIndex, patternTable, firstTile)
			|| !ChrTileDecoder::decodeTile(
				this->ppuBus, static_cast<quint16>(tileIndex + 1),
				patternTable, secondTile)) {
			renderedSprite = {};
			return false;
		}
	} else {
		const ChrTileDecoder::PatternTable patternTable =
			(this->control & 0x08) != 0
			? ChrTileDecoder::PatternTable::Upper
			: ChrTileDecoder::PatternTable::Lower;
		if (!ChrTileDecoder::decodeTile(
				this->ppuBus, sprite.tile, patternTable, firstTile)) {
			renderedSprite = {};
			return false;
		}
	}

	renderedSprite.width = static_cast<quint8>(width);
	renderedSprite.height = static_cast<quint8>(height);
	renderedSprite.pixels.resize(width * height);
	renderedSprite.paletteIndices.resize(width * height);
	for (int y = 0; y < height; ++y) {
		const int sourceY = flipVertical ? height - 1 - y : y;
		const QVector<quint8> &tile = sourceY < 8 ? firstTile : secondTile;
		const int tileY = sourceY & 0x07;
		for (int x = 0; x < width; ++x) {
			const int sourceX = flipHorizontal ? width - 1 - x : x;
			const quint8 pixel = tile[tileY * width + sourceX];
			const int outputIndex = y * width + x;
			renderedSprite.pixels[outputIndex] = pixel;
			if (pixel == 0) {
				renderedSprite.paletteIndices[outputIndex] = 0;
				continue;
			}

			quint8 paletteIndex = this->ppuBus.openBusValue();
			const quint16 paletteAddress = static_cast<quint16>(
				0x3F10 + paletteNumber * 4 + pixel);
			if (this->ppuBus.read(paletteAddress, paletteIndex)
					!= Bus::AccessResult::Handled) {
				renderedSprite = {};
				return false;
			}
			renderedSprite.paletteIndices[outputIndex] = paletteIndex;
		}
	}
	return true;
}

QVector<Ppu::SpriteOutput> Ppu::renderSpritesForFrame() {
	QVector<SpriteOutput> outputs;
	if ((this->mask & 0x10) == 0) {
		return outputs;
	}

	outputs.reserve(64);
	for (int index = 0; index < 64; ++index) {
		const quint16 address = static_cast<quint16>(index * 4);
		const SpriteEntry entry{
			.index = static_cast<quint8>(index),
			.y = this->oam.getU8(address),
			.tile = this->oam.getU8(static_cast<quint16>(address + 1)),
			.attributes = this->oam.getU8(static_cast<quint16>(address + 2)),
			.x = this->oam.getU8(static_cast<quint16>(address + 3)),
		};
		SpriteRender renderedSprite;
		if (!this->renderSprite(entry, renderedSprite)) {
			continue;
		}
		outputs.append({
			.entry = entry,
			.render = renderedSprite,
			.screenX = entry.x,
			.screenY = static_cast<quint16>(entry.y + 1),
		});
	}
	return outputs;
}

Ppu::SpritePixel Ppu::composeSpritePixel(
	quint8 backgroundPixel,
	quint8 backgroundPaletteIndex,
	quint8 spritePixel,
	quint8 spritePaletteIndex,
	bool spriteBehindBackground,
	bool spriteZero) const {
	SpritePixel result;
	result.spriteOpaque = spritePixel != 0;
	result.sprite0Hit = spriteZero
		&& result.spriteOpaque
		&& backgroundPixel != 0;
	if (!result.spriteOpaque
		|| (spriteBehindBackground && backgroundPixel != 0)) {
		result.paletteIndex = backgroundPaletteIndex;
		return result;
	}
	result.paletteIndex = spritePaletteIndex;
	return result;
}

void Ppu::setNametableMirroring(NametableMirroring mirroring) {
	if (this->nametableMirroringValue == mirroring) {
		return;
	}
	this->nametableMirroringValue = mirroring;
	this->markAllTilesDirty();
}

Ppu::NametableMirroring Ppu::nametableMirroring() const {
	return this->nametableMirroringValue;
}

QVector<Ppu::DirtyTile> Ppu::takeDirtyTiles() {
	QVector<DirtyTile> result;
	result.reserve(4 * 32 * 30);
	for (int nametable = 0; nametable < 4; ++nametable) {
		for (int tileY = 0; tileY < 30; ++tileY) {
			for (int tileX = 0; tileX < 32; ++tileX) {
				const int index = this->dirtyTileIndex(nametable, tileX, tileY);
				if (!this->dirtyTiles[static_cast<size_t>(index)]) {
					continue;
				}
				result.append({
					static_cast<quint8>(nametable),
					static_cast<quint8>(tileX),
					static_cast<quint8>(tileY)});
				this->dirtyTiles[static_cast<size_t>(index)] = false;
			}
		}
	}
	return result;
}

Ppu::WriteStats Ppu::takeWriteStats() {
	const WriteStats result = this->writeStats;
	this->writeStats = {};
	return result;
}

void Ppu::invalidateAllTiles() {
	this->markAllTilesDirty();
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

bool Ppu::sampleBackgroundPixel(
	int screenX,
	int screenY,
	quint8 &pixel,
	quint8 &paletteIndex) {
	if (screenX < 0 || screenX >= 256 || screenY < 0 || screenY >= 240) {
		return false;
	}

	const int coarseScrollX = this->temporaryAddress & 0x001F;
	const int coarseScrollY = (this->temporaryAddress >> 5) & 0x001F;
	const int fineScrollY = (this->temporaryAddress >> 12) & 0x0007;
	const int baseNametableX = (this->temporaryAddress >> 10) & 0x01;
	const int baseNametableY = (this->temporaryAddress >> 11) & 0x01;
	const int worldX = baseNametableX * 256
		+ coarseScrollX * 8 + this->fineX + screenX;
	const int worldY = baseNametableY * 240
		+ coarseScrollY * 8 + fineScrollY + screenY;
	const int nametableX = (worldX / 256) & 0x01;
	const int nametableY = (worldY / 240) & 0x01;
	const int nametable = (nametableX + nametableY * 2) & 0x03;
	const int localX = worldX & 0x00FF;
	const int localY = worldY % 240;
	const int tileX = localX / 8;
	const int tileY = localY / 8;
	QVector<quint8> tilePixels;
	QVector<quint8> subpalette;
	if (!this->renderNametableTile(
		nametable, tileX, tileY, tilePixels, subpalette)
		|| tilePixels.size() != 64 || subpalette.size() != 4) {
		return false;
	}

	pixel = tilePixels[(localY & 0x07) * 8 + (localX & 0x07)];
	paletteIndex = subpalette[pixel & 0x03];
	return true;
}

bool Ppu::renderBackgroundFrame(BackgroundFrame &frame) {
	frame.width = 256;
	frame.height = 240;
	frame.paletteIndices.resize(frame.width * frame.height);
	std::array<QVector<quint8>, 4 * 32 * 30> tilePixelsCache;
	std::array<QVector<quint8>, 4 * 32 * 30> subpaletteCache;
	std::array<bool, 4 * 32 * 30> tileCacheValid{};
	for (int screenY = 0; screenY < frame.height; ++screenY) {
		for (int screenX = 0; screenX < frame.width; ++screenX) {
			const int coarseScrollX = this->temporaryAddress & 0x001F;
			const int coarseScrollY = (this->temporaryAddress >> 5) & 0x001F;
			const int fineScrollY = (this->temporaryAddress >> 12) & 0x0007;
			const int baseNametableX = (this->temporaryAddress >> 10) & 0x01;
			const int baseNametableY = (this->temporaryAddress >> 11) & 0x01;
			const int worldX = baseNametableX * 256
				+ coarseScrollX * 8 + this->fineX + screenX;
			const int worldY = baseNametableY * 240
				+ coarseScrollY * 8 + fineScrollY + screenY;
			const int nametableX = (worldX / 256) & 0x01;
			const int nametableY = (worldY / 240) & 0x01;
			const int nametable = (nametableX + nametableY * 2) & 0x03;
			const int localX = worldX & 0x00FF;
			const int localY = worldY % 240;
			const int tileX = localX / 8;
			const int tileY = localY / 8;
			const int cacheIndex = (nametable * 30 + tileY) * 32 + tileX;
			if (!tileCacheValid[static_cast<size_t>(cacheIndex)]) {
				if (!this->renderNametableTile(
					nametable, tileX, tileY,
					tilePixelsCache[static_cast<size_t>(cacheIndex)],
					subpaletteCache[static_cast<size_t>(cacheIndex)])) {
					frame.paletteIndices.clear();
					return false;
				}
				tileCacheValid[static_cast<size_t>(cacheIndex)] = true;
			}

			const QVector<quint8> &tilePixels =
				tilePixelsCache[static_cast<size_t>(cacheIndex)];
			const QVector<quint8> &subpalette =
				subpaletteCache[static_cast<size_t>(cacheIndex)];
			if (tilePixels.size() != 64 || subpalette.size() != 4) {
				frame.paletteIndices.clear();
				return false;
			}
			const quint8 pixel = tilePixels[(localY & 0x07) * 8 + (localX & 0x07)];
			frame.paletteIndices[screenY * frame.width + screenX] =
				subpalette[pixel & 0x03];
		}
	}
	return true;
}

Ppu::ScrollSnapshot Ppu::scrollSnapshot() const {
	const int coarseScrollX = this->temporaryAddress & 0x001F;
	const int coarseScrollY = (this->temporaryAddress >> 5) & 0x001F;
	const int fineScrollY = (this->temporaryAddress >> 12) & 0x0007;
	const int baseNametableX = (this->temporaryAddress >> 10) & 0x01;
	const int baseNametableY = (this->temporaryAddress >> 11) & 0x01;
	return {
		.x = baseNametableX * 256 + coarseScrollX * 8 + this->fineX,
		.y = baseNametableY * 240 + coarseScrollY * 8 + fineScrollY,
	};
}

QVector<Ppu::ScrollSnapshot> Ppu::rasterScroll() const {
	return this->rasterScrollValue;
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
	this->writeStats = {};
	this->invalidateSprite0Cache();
	this->resetClock();
	this->markAllTilesDirty();
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
	if (this->scanlineValue < 240 && this->dotValue >= 1
		&& this->dotValue <= 256) {
		this->checkSprite0Hit(
			this->scanlineValue, this->dotValue);
	}
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
	if (this->scanlineValue < 240) {
		this->rasterScrollValue[static_cast<int>(this->scanlineValue)] =
			this->scrollSnapshot();
	}
}

Bus &Ppu::bus() {
	return this->ppuBus;
}

const Bus &Ppu::bus() const {
	return this->ppuBus;
}

void Ppu::checkSprite0Hit(quint16 scanline, quint16 dot) {
	++this->writeStats.sprite0HitChecks;
	if ((this->status & 0x40) != 0
		|| (this->mask & 0x18) != 0x18) {
		return;
	}

	if (!this->sprite0EvaluationValid
		|| this->sprite0EvaluationScanline != scanline) {
		++this->writeStats.sprite0HitEvaluations;
		const SpriteEvaluation evaluation = this->evaluateSpritesForScanline(scanline);
		const auto spriteIterator = std::find_if(
			evaluation.sprites.cbegin(), evaluation.sprites.cend(),
			[](const SpriteEntry &sprite) { return sprite.index == 0; });
		this->sprite0EvaluationScanline = scanline;
		this->sprite0EvaluationValid = true;
		this->sprite0Present = spriteIterator != evaluation.sprites.cend();
		this->sprite0RenderValid = false;
		if (this->sprite0Present) {
			this->cachedSprite0 = *spriteIterator;
		}
	}
	if (!this->sprite0Present) {
		return;
	}

	const SpriteEntry &sprite = this->cachedSprite0;
	const int screenX = static_cast<int>(dot) - 1;
	const int spriteTop = static_cast<int>(sprite.y) + 1;
	const int spriteX = screenX - static_cast<int>(sprite.x);
	const int spriteY = static_cast<int>(scanline) - spriteTop;
	if (spriteX < 0 || spriteX >= 8 || spriteY < 0) {
		return;
	}
	if (screenX < 8 && (this->mask & 0x06) != 0x06) {
		return;
	}

	if (!this->sprite0RenderValid) {
		++this->writeStats.sprite0HitRenders;
		this->sprite0RenderValid = this->renderSprite(
			sprite, this->cachedSprite0Render);
	}
	if (!this->sprite0RenderValid
		|| spriteY >= this->cachedSprite0Render.height) {
		return;
	}

	quint8 backgroundPixel = 0;
	quint8 backgroundPaletteIndex = 0;
	++this->writeStats.sprite0HitSamples;
	if (!this->sampleBackgroundPixel(
		screenX, static_cast<int>(scanline),
		backgroundPixel, backgroundPaletteIndex)) {
		return;
	}
	const int spritePixelIndex = spriteY * 8 + spriteX;
	const SpritePixel composed = this->composeSpritePixel(
		backgroundPixel,
		backgroundPaletteIndex,
		this->cachedSprite0Render.pixels[spritePixelIndex],
		this->cachedSprite0Render.paletteIndices[spritePixelIndex],
		(sprite.attributes & 0x20) != 0,
		true);
	if (composed.sprite0Hit) {
		this->status |= 0x40;
		++this->writeStats.sprite0Hits;
	}
}

void Ppu::invalidateSprite0Cache() {
	this->sprite0EvaluationValid = false;
	this->sprite0Present = false;
	this->sprite0RenderValid = false;
	this->cachedSprite0Render = {};
}

void Ppu::markMemoryWrite(quint16 address) {
	const quint16 canonicalAddress = static_cast<quint16>(address & 0x3FFF);
	if (canonicalAddress >= 0x2000 && canonicalAddress <= 0x2FFF) {
		this->markNametableTileDirty(canonicalAddress);
		return;
	}
	if (canonicalAddress >= 0x3000 && canonicalAddress <= 0x3EFF) {
		this->markNametableTileDirty(static_cast<quint16>(
			0x2000 | (canonicalAddress & 0x0FFF)));
		return;
	}
	if (canonicalAddress >= 0x3F00) {
		this->markAllTilesDirty();
	}
}

void Ppu::markAllTilesDirty() {
	this->dirtyTiles.fill(true);
}

void Ppu::markNametableTileDirty(quint16 address) {
	const quint16 physicalAddress = this->translateNametableAddress(address);
	const quint16 offset = static_cast<quint16>(physicalAddress & 0x03FF);
	const bool attribute = offset >= 0x03C0;
	for (int nametable = 0; nametable < 4; ++nametable) {
		for (int tileY = 0; tileY < 30; ++tileY) {
			for (int tileX = 0; tileX < 32; ++tileX) {
				const quint16 logicalAddress = static_cast<quint16>(
					0x2000 + nametable * 0x0400 +
					(attribute ? 0x03C0 + (tileY / 4) * 8 + tileX / 4
						: tileY * 32 + tileX));
				if (this->translateNametableAddress(logicalAddress) != physicalAddress) {
					continue;
				}
				if (attribute) {
					this->markAttributeDirty(logicalAddress);
				} else {
					this->markLogicalTileDirty(nametable, tileX, tileY);
				}
			}
		}
	}
}

void Ppu::markAttributeDirty(quint16 address) {
	const int nametable = static_cast<int>((address - 0x2000) / 0x0400);
	const int attributeIndex = static_cast<int>((address - 0x2000) & 0x03FF) - 0x03C0;
	const int attributeX = (attributeIndex % 8) * 4;
	const int attributeY = (attributeIndex / 8) * 4;
	for (int tileY = attributeY; tileY < attributeY + 4 && tileY < 30; ++tileY) {
		for (int tileX = attributeX; tileX < attributeX + 4 && tileX < 32; ++tileX) {
			this->markLogicalTileDirty(nametable, tileX, tileY);
		}
	}
}

void Ppu::markLogicalTileDirty(int nametable, int tileX, int tileY) {
	this->dirtyTiles[static_cast<size_t>(
		this->dirtyTileIndex(nametable, tileX, tileY))] = true;
}

int Ppu::dirtyTileIndex(int nametable, int tileX, int tileY) const {
	return (nametable * 30 + tileY) * 32 + tileX;
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

void Ppu::writeOamDmaByte(quint8 value) {
	this->oam.setU8(this->oamAddress, value);
	this->invalidateSprite0Cache();
	++this->oamAddress;
}
