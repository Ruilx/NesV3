#pragma once

#include "Mapper.h"

class Ram;

/**
 * Mapper001 (MMC1)
 * The Nintendo MMC1 is a mapper ASIC used in Nintendo's SxROM, NES-EVENT and 2ME circuit boards.
 * It first appeared in April of 1987.
 * 
 * iNES Mapper 1 and 155 denote SxROM and 2ME circuit boards mounting the MMC1B(iNES Mapper 1) or MMC1A (iNES Mapper 155)
 * revision of the AVIC.
 * Mapper 1 is used if the revision is not known or irrelevant.
 * 
 * Three submappers of mapper 1/155 are valid:
 * * Submapper 5 denotes the SEROM/SHROM/SH1ROM PCB with 32KiB of unbanked PRG-ROM
 * * Submapper 6 denotes the Famicom Network System's 2ME PCB
 * * Submapper 7 denotes the Kaiser KS-7058 that mounts the KS 203 MMC1 clone, with hard-wired nametable arrangement.
 * All other SxROM variants are denoted by their functional PRG/CHR-ROM/RAM sizes in the NES2.0 header.
 * Without NES2.0, the PRG-RAM size has to be assumed; 32KiB are sufficient for compatibility with all known titles.
 * INES Mapper 105 denotes the NES-EVENT PCB.
 * 
 * Banks:
 * CPU $6000-$7FFF: 8KB PRG-RAM bank, optional.
 * CPU $8000-$BFFF: 16KB PRG-ROM bank, either switchable or fixed to the first bank.
 * CPU $C000-$FFFF: 16KB PRG-ROM bank, either fixed to the last bank or switchable.
 * PPU $0000-$0FFF: 4KB switchable CHR bank.
 * PPU $1000-$1FFF: 4KB switchable CHR bank.
 * 
 * Through writes to the MMC1 control register, it's possible for the program to swap the fixed and switchable PRG-ROM 
 * bank or setup 32KB PRG bankswitching (like BNROM), but most games use the default setup, which is similar to that 
 * of UxROM.
 * 
 * MMC1是通过串行端口配置的：
 * * MMC1寄存器复位值为0b10000。控制寄存器被设置为0b01100
 * * 当CPU向$8000-$FFFF的任意地址写入高位（bit7）为1的字节会清空移位移存器，PRG bank设置为默认模式。
 * * 当CPU向$8000-$FFFF的任意地址写入高位（bit7）为0的字节会将MMC1寄存器原有数据右移，并将数据中低位（bit0）存入寄存器高位（bit4）。
 * * 连续写入5次的同时，MMC1会根据当前地址的A13、A14，将5位数据写入对应目标寄存器中，然后清空寄存器。
 *   * [14:13]=0b00：控制寄存器
 *   * [14:13]=0b01：CHR Bank 0
 *   * [14:13]=0b10：CHR Bank 1
 *   * [14:13]=0b11：PRG Bank
 * 
 * * 写入控制寄存器的数据：
 *   * [4]：CHR bank 模式（C）：
 *     * 0b0：8KB模式，CHR空间作为整体切换，仅CHR Bank0寄存器生效
 *     * 0b1：4KB模式，CHR分为两个4KB区域，分别由CHR bank0、CHR bank1独立控制
 *   * [3:2]：PRG bank 模式（PP）：
 *     * 0b00/0b01：32KB模式，$8000-$FFFF整体作为32KB bank切换。
 *     * 0b10：固定$8000-$BFFF为第0个bank，$C000-$FFFF为可切换16KB bank
 *     * 0b11；固定$C000-$FFFF为最后一个bank，$8000-$BFFF为可切换16KB bank（default）
 *   * [1:0]: nametable mirroring（MM）:
 *     * 0b00：单屏A
 *     * 0b01：单屏B
 *     * 0b10：垂直镜像
 *     * 0b11：水平镜像
 * 
 * * 写入PRG bank寄存器的数据：
 *   * [4]：不同版本功能差异：
 *     * MMC1A：直接控制PRG A17，配合外部电路扩展PRG到512KB
 *     * MMC1B：PRG RAM写入保护：1:RO；0:RW
 *   * [3:0]：16KB PRG bank号码，支持16个bank 256KB PRG-ROM，32KB时忽略最高位，达到8个bank。
 * 
 * 版型和连接变种
 * * SKROM/SLROM：标准配置 256KB PRG-ROM + 128KB CHR-ROM + 8KB PRG-RAM
 * * SNROM：8KB CHR-ROM，复用CHR A16作为PRG-RAM的写保护
 * * SUROM：复用CHR A16作为PRG A18，将PRG-ROM扩展到512KB
 * * SOROM/SXROM：复用CHR地址线扩展PRG-RAM到16/32KB
 * * SEROM/SHROM：32KB PRG直接连接CPU，MMC1只控制CHR和镜像
 */

class Mapper001 final : public Mapper {
public:
	struct Stats {
		quint64 serialWrites = 0;
		quint64 chrBankCommits = 0;
		quint64 effectiveChrChanges = 0;
		quint64 resetWrites = 0;
	};

	Mapper001(Ram &prgRom, Ram &prgRam, Ram &chrRom, bool chrRam = false);

	[[nodiscard]] bool chrRam() const override;
	[[nodiscard]] Mirroring mirroring() const override;
	[[nodiscard]] Stats takeStats();

	bool readCpu(quint16 address, quint8 &value) override;
	bool writeCpu(quint16 address, quint8 value) override;
	bool readPpu(quint16 address, quint8 &value) override;
	bool writePpu(quint16 address, quint8 value) override;

private:
	void commitRegister(quint16 address, quint8 value);
	[[nodiscard]] qsizetype prgOffset(quint16 address) const;
	[[nodiscard]] qsizetype chrOffset(quint16 address) const;

	Ram &prgRom;
	Ram &prgRam;
	Ram &chrRom;
	bool chrRamEnabled = false;
	quint8 shiftRegister = 0x10;
	quint8 controlRegister = 0x0C;
	quint8 chrBank0 = 0;
	quint8 chrBank1 = 0;
	quint8 prgBank = 0;
	Stats stats;
};