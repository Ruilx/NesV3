#pragma once

#include <QByteArray>
#include <QString>
#include <QtGlobal>

struct InesHeader {
    static constexpr qsizetype Size = 16;
    static constexpr qsizetype PrgBankSize = 16 * 1024;
    static constexpr qsizetype ChrBankSize = 8 * 1024;

    quint8 prgRomBanks = 0;
    quint8 chrRomBanks = 0;
    quint8 mapperNumber = 0;
    bool hasTrainer = false;
    bool verticalMirroring = false;
    bool fourScreenMirroring = false;

    [[nodiscard]] qsizetype prgRomSize() const;
    [[nodiscard]] qsizetype chrRomSize() const;

    [[nodiscard]] static bool parse(
            const QByteArray &data,
            InesHeader &header,
            QString &error);
};