#include "InesHeader.h"

qsizetype InesHeader::prgRomSize() const {
    return static_cast<qsizetype>(this->prgRomBanks) * PrgBankSize;
}

qsizetype InesHeader::chrRomSize() const {
    return static_cast<qsizetype>(this->chrRomBanks) * ChrBankSize;
}

bool InesHeader::parse(
        const QByteArray &data,
        InesHeader &header,
        QString &error) {
    if (data.size() < Size) {
        error = QStringLiteral("The ROM file is smaller than an iNES header.");
        return false;
    }
    if (data[0] != 'N' || data[1] != 'E' || data[2] != 'S' || data[3] != 0x1A) {
        error = QStringLiteral("The ROM file does not contain an iNES signature.");
        return false;
    }

    const quint8 flags6 = static_cast<quint8>(data[6]);
    const quint8 flags7 = static_cast<quint8>(data[7]);
    if ((flags7 & 0x0C) == 0x08) {
        error = QStringLiteral("NES 2.0 ROMs are not supported yet.");
        return false;
    }

    header.prgRomBanks = static_cast<quint8>(data[4]);
    header.chrRomBanks = static_cast<quint8>(data[5]);
    header.mapperNumber = static_cast<quint8>((flags6 >> 4) | (flags7 & 0xF0));
    header.hasTrainer = (flags6 & 0x04) != 0;
    header.verticalMirroring = (flags6 & 0x01) != 0;
    header.fourScreenMirroring = (flags6 & 0x08) != 0;

    if (header.prgRomBanks == 0) {
        error = QStringLiteral("The ROM contains no PRG-ROM banks.");
        return false;
    }
    const qsizetype payloadOffset = Size
            + (header.hasTrainer ? 512 : 0);
    const qsizetype payloadSize = header.prgRomSize()
            + (header.chrRomBanks == 0 ? 0 : header.chrRomSize());
    if (data.size() < payloadOffset + payloadSize) {
        error = QStringLiteral("The ROM file is truncated.");
        return false;
    }
    return true;
}