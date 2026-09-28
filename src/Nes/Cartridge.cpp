#include "Cartridge.h"

#include "Mapper/Mapper000.h"
#include "Mapper/Mapper001.h"
#include "Mapper/Mapper003.h"

#include <QFile>

Cartridge::Cartridge(size_t prgRomSize, size_t chrRomSize)
    : prgRomStorage(prgRomSize),
    prgRamStorage(8 * 1024),
      chrRomStorage(chrRomSize),
      mapper(std::make_unique<Mapper000>(
          this->prgRomStorage, this->chrRomStorage, false)),
      cpuDevice(this->mapper.get()),
    ppuDevice(this->mapper.get()),
    prgRamDevice(this->prgRamStorage) {
}

std::unique_ptr<Mapper> Cartridge::createMapper(
    quint8 mapperNumber, bool chrRam) {
    switch (mapperNumber) {
    case 0:
        return std::make_unique<Mapper000>(
            this->prgRomStorage, this->chrRomStorage, chrRam);
    case 3:
        return std::make_unique<Mapper003>(
            this->prgRomStorage, this->chrRomStorage, chrRam);
    case 1:
        return std::make_unique<Mapper001>(
            this->prgRomStorage, this->prgRamStorage,
            this->chrRomStorage, chrRam);
    default:
        return nullptr;
    }
}

bool Cartridge::loadFromFile(const QString &path, QString &error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        error = file.errorString();
        return false;
    }
    const QByteArray data = file.readAll();
    InesHeader parsedHeader;
    if (!InesHeader::parse(data, parsedHeader, error)) {
        return false;
    }
    const bool chrRam = parsedHeader.chrRomBanks == 0;
    std::unique_ptr<Mapper> parsedMapper = this->createMapper(
        parsedHeader.mapperNumber, chrRam);
    if (parsedMapper == nullptr) {
        error = QStringLiteral("Mapper %1 is not supported yet.")
                .arg(parsedHeader.mapperNumber);
        return false;
    }

    const qsizetype dataOffset = InesHeader::Size
            + (parsedHeader.hasTrainer ? 512 : 0);
    const qsizetype prgSize = parsedHeader.prgRomSize();
    const qsizetype chrSize = parsedHeader.chrRomBanks == 0
            ? InesHeader::ChrBankSize
            : parsedHeader.chrRomSize();
    Bus *cpuBus = this->connectedCpuBus;
    Bus *ppuBus = this->connectedPpuBus;
    this->disconnect();
    this->prgRomStorage.resize(static_cast<size_t>(prgSize));
    this->chrRomStorage.resize(static_cast<size_t>(chrSize));
    for (qsizetype index = 0; index < prgSize; ++index) {
        this->prgRomStorage.setU8(index, static_cast<quint8>(data[dataOffset + index]));
    }
    if (parsedHeader.chrRomBanks != 0) {
        const qsizetype chrOffset = dataOffset + prgSize;
        for (qsizetype index = 0; index < parsedHeader.chrRomSize(); ++index) {
            this->chrRomStorage.setU8(index, static_cast<quint8>(data[chrOffset + index]));
        }
    }
    this->mapper = std::move(parsedMapper);
    this->mapper->setMirroringCallback(this->mirroringCallback);
    this->mapper->setMappingChangedCallback(this->mappingChangedCallback);
    if (this->mirroringCallback) {
        this->mirroringCallback(this->mapper->mirroring());
    }
    this->cpuDevice.setMapper(this->mapper.get());
    this->ppuDevice.setMapper(this->mapper.get());
    this->inesHeader = parsedHeader;
    this->loaded = true;
    if (cpuBus != nullptr && ppuBus != nullptr) {
        this->connect(*cpuBus, *ppuBus);
    }
    return true;
}

void Cartridge::unload() {
    this->disconnect();
    this->prgRomStorage.clear();
    this->chrRomStorage.clear();
    this->prgRamStorage.resize(8 * 1024);
    this->mapper = std::make_unique<Mapper000>(
        this->prgRomStorage, this->chrRomStorage, false);
    this->cpuDevice.setMapper(this->mapper.get());
    this->ppuDevice.setMapper(this->mapper.get());
    this->mapper->setMirroringCallback(this->mirroringCallback);
    this->mapper->setMappingChangedCallback(this->mappingChangedCallback);
    this->inesHeader = InesHeader();
    this->loaded = false;
}

bool Cartridge::isLoaded() const {
    return this->loaded;
}

const InesHeader &Cartridge::header() const {
    return this->inesHeader;
}

Cartridge::~Cartridge() {
    this->disconnect();
}

void Cartridge::connect(Bus &cpuBus, Bus &ppuBus) {
    this->disconnect();

    const Bus::Mapping cpuMapping{
        .start = 0x8000,
        .end = 0xFFFF,
        .priority = 0,
        .flags = AccessFlags::Readable | AccessFlags::Writable
            | AccessFlags::Executable,
        .name = QStringLiteral("Cartridge PRG-ROM"),
        .device = &this->cpuDevice,
        .translate = [](quint16 address) {
            return static_cast<quint16>(address - 0x8000);
        },
    };
    const Bus::Mapping prgRamMapping{
        .start = 0x6000,
        .end = 0x7FFF,
        .priority = 0,
        .flags = AccessFlags::Readable | AccessFlags::Writable,
        .name = QStringLiteral("Cartridge PRG-RAM"),
        .device = &this->prgRamDevice,
    };
    const Bus::Mapping ppuMapping{
        .start = 0x0000,
        .end = 0x1FFF,
        .priority = 0,
        .flags = this->mapper->chrRam()
            ? AccessFlags::Readable | AccessFlags::Writable
            : AccessFlags::Readable,
        .name = QStringLiteral("Cartridge CHR-ROM"),
        .device = &this->ppuDevice,
    };

    this->cpuMappingId = cpuBus.registerMapping(cpuMapping);
    this->prgRamMappingId = cpuBus.registerMapping(prgRamMapping);
    this->ppuMappingId = ppuBus.registerMapping(ppuMapping);
    if (this->cpuMappingId == 0 || this->prgRamMappingId == 0
        || this->ppuMappingId == 0) {
        if (this->cpuMappingId != 0) {
            cpuBus.unregisterMapping(this->cpuMappingId);
        }
        if (this->ppuMappingId != 0) {
            ppuBus.unregisterMapping(this->ppuMappingId);
        }
        if (this->prgRamMappingId != 0) {
            cpuBus.unregisterMapping(this->prgRamMappingId);
        }
        this->cpuMappingId = 0;
        this->prgRamMappingId = 0;
        this->ppuMappingId = 0;
        return;
    }

    this->connectedCpuBus = &cpuBus;
    this->connectedPpuBus = &ppuBus;
}

void Cartridge::disconnect() {
    if (this->connectedCpuBus != nullptr && this->cpuMappingId != 0) {
        this->connectedCpuBus->unregisterMapping(this->cpuMappingId);
    }
    if (this->connectedCpuBus != nullptr && this->prgRamMappingId != 0) {
        this->connectedCpuBus->unregisterMapping(this->prgRamMappingId);
    }
    if (this->connectedPpuBus != nullptr && this->ppuMappingId != 0) {
        this->connectedPpuBus->unregisterMapping(this->ppuMappingId);
    }

    this->connectedCpuBus = nullptr;
    this->connectedPpuBus = nullptr;
    this->cpuMappingId = 0;
    this->prgRamMappingId = 0;
    this->ppuMappingId = 0;
}

Ram &Cartridge::prgRom() {
    return this->prgRomStorage;
}

Ram &Cartridge::prgRam() {
    return this->prgRamStorage;
}

Ram &Cartridge::chrRom() {
    return this->chrRomStorage;
}

const Ram &Cartridge::prgRom() const {
    return this->prgRomStorage;
}

const Ram &Cartridge::prgRam() const {
    return this->prgRamStorage;
}

const Ram &Cartridge::chrRom() const {
    return this->chrRomStorage;
}

void Cartridge::setMirroringCallback(Mapper::MirroringCallback callback) {
    this->mirroringCallback = std::move(callback);
    this->mapper->setMirroringCallback(this->mirroringCallback);
    if (this->mirroringCallback) {
        this->mirroringCallback(this->mapper->mirroring());
    }
}

void Cartridge::setMappingChangedCallback(
        Mapper::MappingChangedCallback callback) {
    this->mappingChangedCallback = std::move(callback);
    this->mapper->setMappingChangedCallback(this->mappingChangedCallback);
}

Mapper001 *Cartridge::mapper001() {
    return dynamic_cast<Mapper001 *>(this->mapper.get());
}
