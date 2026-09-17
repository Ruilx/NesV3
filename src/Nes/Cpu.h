#pragma once

#include <QObject>

#include "Bus.h"
#include "Ram.h"

class Ram;

class Cpu : public QObject {
Q_OBJECT

    Bus cpuBus;
    Ram internalRam;
    RamBusDevice internalRamDevice;
    quint64 totalCycles = 0;
    quint64 dmaCycles = 0;

    // zero & negative table
    quint8 znTable[256] = {0};

    // clock process
    bool clockProcess = false;

public:
    Q_FLAGS(CpuFlags)
    enum CpuFlag {
        CFlag = 0x01, // carry flag
        ZFlag = 0x02, // zero flag
        IFlag = 0x04, // irq disabled
        DFlag = 0x08, // decimal mode flag (unused)
        BFlag = 0x10, // break flag
        RFlag = 0x20, // unused (always 1)
        VFlag = 0x40, // overflow flag
        NFlag = 0x80, // negative flag
    };
    Q_DECLARE_FLAGS(CpuFlags, CpuFlag);

    enum CpuInterrupt {
        None = 0x00,        // no interrupt
        NmiFlag = 0x01,     // non-maskable interrupt
        IrqFlag = 0x02,     // maskable interrupt
        IrqFrameIrq = 0x04, // irq frame interrupt
        IrqDpcm = 0x08,     // DPCM interrupt
        IrqMapper = 0x10,   // mapper interrupt
        IrqMapper2 = 0x20,  // mapper interrupt 2
        IrqTrigger = 0x40,  // trigger interrupt (one shot (old IRQ))
        IrqTrigger2 = 0x80, // trigger interrupt 2 (one shot (old IRQ))
        IrqMask = (~(NmiFlag | IrqFlag)),
    };

    const quint16 NmiVector = 0xFFFA;
    const quint16 ResVector = 0xFFFC;
    const quint16 IrqVector = 0xFFFE;

    typedef struct {
        quint16 pc; // program counter
        quint8 a;
        quint8 p;
        quint8 x;
        quint8 y;
        quint8 s;
        CpuInterrupt intPending; // interrupt pending flag
    } CpuReg, R6502;
    struct ExecutionStats {
        quint64 instructions = 0;
        quint64 nmiEntries = 0;
        quint64 killedInstructions = 0;
        quint16 lastPc = 0;
        quint8 lastOpcode = 0;
    };
private:
    quint16 ea;
    quint16 et;
    quint16 wt;
    quint8 dt;
    quint8 instructionCyclesRemaining = 0;
    ExecutionStats executionStats;

    quint8 op8(quint16 addr);
    quint16 op16(quint16 addr);
    quint8 executeInstruction();

    CpuReg reg = {
            .pc = 0x0000,
            .a = 0x00,
            .p = ZFlag | RFlag,
            .x = 0x00,
            .y = 0x00,
            .s = 0xFF,
            .intPending = CpuInterrupt::None,
    };

public:
    explicit Cpu(QObject *parent = nullptr);

    ~Cpu() override = default;

    quint8 readRam8(quint16 addr);

    quint16 readRam16(quint16 addr);

    void writeRam(quint16 addr, quint8 value);

    void reset();

    void clock();

    void nmi();

    void setIrq(quint8 mask);

    void clearIrq(quint8 mask);

    void dma(quint64 cycles);

    quint64 exec(quint64 reqCycles);

    quint8 stepInstruction();

    void tickCpuCycle();

    quint64 getDmaCycles() const;

    void setDmaCycles(quint64 value);

    quint64 getTotalCycles() const;

    void setTotalCycles(quint64 value);

    [[nodiscard]] ExecutionStats takeExecutionStats();

    void setContent(const CpuReg &reg) { this->reg = reg; }

    void getContent(CpuReg &reg) { reg = this->reg; }

    void setClockProcess(bool e) { this->clockProcess = e; }

    [[nodiscard]] Bus &bus() { return this->cpuBus; }
    [[nodiscard]] const Bus &bus() const { return this->cpuBus; }

private:
    // op part
    // zero page read
    quint8 zeroPageRead(quint8 addr);

    quint16 zeroPageReadW(quint8 addr);

    // zero page write
    void zeroPageWrite(quint8 addr, quint8 value);

    void zeroPageWriteW(quint8 addr, quint16 value);

    // effective address page boundary beyond check
    quint8 checkEa() const;

    // flag operation
    // check the setting of the zero / negative flag
    void setZnFlag(quint8 flag);

    void setFlag(quint8 flag);

    void clearFlag(quint8 flag);

    void testFlag(bool ok, quint8 flag);

    bool checkFlag(quint8 flag) const;

    // WT: word temp
    // EA: effective address
    // ET: effective address temp
    // DT: data
    void mrIm();

    void mrZp();

    void mrZx();

    void mrZy();

    void mrAb();

    void mrAx();

    void mrAy();

    void mrIx();

    void mrIy();

    // effective address
    void eaZp();

    void eaZx();

    void eaZy();

    void eaAb();

    void eaAx();

    void eaAy();

    void eaIx();

    void eaIy();

    // memory write
    void mwZp();

    void mwEa();

    // stack operations
    void push(quint8 data);

    quint8 pop();

    // pop & set zn flag
    quint8 popAndSetZnFlag();

    // arithmetic operations
    // flags(NVRBDIZC)
    // ADC  (NV----ZC)
    void adc();

    // SBC  (NV----ZC)
    void sbc();

    // INC  (N-----Z-)
    void inc();

    // INX  (N-----Z-)
    void inx();

    // INY  (N-----Z-)
    void iny();

    // DEC  (N-----Z-)
    void dec();

    // DEX  (N-----Z-)
    void dex();

    // DEY  (N-----Z-)
    void dey();

    // Logical operations
    // AND  (N-----Z-)
    void _and();

    // ORA  (N-----Z-)
    void ora();

    // EOR  (N-----Z-)
    void eor();

    // ASLA (N-----ZC)
    void asla();

    // ASL  (N-----ZC)
    void asl();

    // LSRA (N-----ZC)
    void lsra();

    // LSR  (N-----ZC)
    void lsr();

    // ROLA (N-----ZC)
    void rola();

    // ROL  (N-----ZC)
    void rol();

    // RORA (N-----ZC)
    void rora();

    // ROR  (N-----ZC)
    void ror();

    // BIT  (NV----Z-)
    void bit();

    // Load / store operations
    // LDA  (N-----Z-)
    void lda();

    // LDX  (N-----Z-)
    void ldx();

    // LDY  (N-----Z-)
    void ldy();

    // STA  (--------)
    void sta();

    // STX  (--------)
    void stx();

    // STY  (--------)
    void sty();

    // TAX  (N-----Z-)
    void tax();

    // TXA  (N-----Z-)
    void txa();

    // TAY  (N-----Z-)
    void tay();

    // TYA  (N-----Z-)
    void tya();

    // TSX  (N-----Z-)
    void tsx();

    // TXS  (--------)
    void txs();

    // Compare operations
    // CMP  (N-----ZC)
    void cmp();

    // CPX  (N-----ZC)
    void cpx();

    // CPY  (N-----ZC)
    void cpy();

    // Jump / return operations
    // JMP_ID
    void jmpId();

    // JMP
    void jmp();

    // JSR
    void jsr();

    // RTS
    void rts();

    // RTI
    void rti();

    // _NMI
    quint8 _nmi();

    // _IRQ
    quint8 _irq();

    // BRK
    void brk();

    // REL_JUMP
    quint8 relJump();

    // BCC
    quint8 bcc();

    // BCS
    quint8 bcs();

    // BNE
    quint8 bne();

    // BEQ
    quint8 beq();

    // BPL
    quint8 bpl();

    // BMI
    quint8 bmi();

    // BVC
    quint8 bvc();

    // BVS
    quint8 bvs();

    // Flag control operations
    // CLC
    void clc();

    // CLD
    void cld();

    // CLI
    void cli();

    // CLV
    void clv();

    // SEC
    void sec();

    // SED
    void sed();

    // SEI
    void sei();

    // Unofficial operations
    // ANC
    void anc();

    // ANE
    void ane();

    // ARR
    void arr();

    // ASR
    void asr();

    // DCP
    void dcp();

//    // DOP
//    inline void dop();
    // ISB
    void isb();

    // LAS
    void las();

    // LAX
    void lax();

    // LXA
    void lxa();

    // RLA
    void rla();

    // RRA
    void rra();

    // SAX
    void sax();

    // SBX
    void sbx();

    // SHA
    void sha();

    // SHS
    void shs();

    // SHX
    void shx();

    // SHY
    void shy();

    // SLO
    void slo();

    // SRE
    void sre();
//    // TOP
//    inline void top();

    // opcodes
    typedef quint8(Cpu::*opFunc)();

    // https://www.nesdev.org/wiki/CPU_unofficial_opcodes
    // https://www.nesdev.org/wiki/Fixed_cycle_delay#49_cycles
    // https://www.oxyron.de/html/opcodes02.html
    // name of opcode:
    // split in 2 parts:
    // code name in left 3 letters
    // arguments name in right 3 letters:
    // right:
    // (EMPTY)  => 'REL', 'IMP'
    // #$??     => 'IMM'
    // $??      => 'ZP_'
    // $??, X   => 'ZPX'
    // $??, Y   => 'ZPY'
    // ($??, X) => 'IZX'
    // ($??), Y => 'IZY'
    // $????    => 'ABS'
    // $????, X => 'ABX'
    // $????, Y => 'ABY'
    // ($????)  => 'IZA', 'IND'
    // A        => 'AAA'
    quint8 BRKREL(); // 0x00
    quint8 ORAIZX(); // 0x01
    quint8 SLOIZX(); // 0x03
    quint8 ORAZP_(); // 0x05
    quint8 ASLZP_(); // 0x06
    quint8 SLOZP_(); // 0x07
    quint8 PHPREL(); // 0x08
    quint8 ORAIMM(); // 0x09
    quint8 ASLAAA(); // 0x0A
    quint8 ANCIMM(); // 0x0B, 0x2B
    quint8 ORAABS(); // 0x0D
    quint8 ASLABS(); // 0x0E
    quint8 SLOABS(); // 0x0F
    quint8 BPLREL(); // 0x10
    quint8 ORAIZY(); // 0x11
    quint8 SLOIZY(); // 0x13
    quint8 ORAZPX(); // 0x15
    quint8 ASLZPX(); // 0x16
    quint8 SLOZPX(); // 0x17
    quint8 CLCREL(); // 0x18
    quint8 ORAABY(); // 0x19
    quint8 SLOABY(); // 0x1B
    quint8 ORAABX(); // 0x1D
    quint8 ASLABX(); // 0x1E
    quint8 SLOABX(); // 0x1F
    quint8 JSRREL(); // 0x20
    quint8 ANDIZX(); // 0x21
    quint8 RLAIZX(); // 0x23
    quint8 BITZP_(); // 0x24
    quint8 ANDZP_(); // 0x25
    quint8 ROLZP_(); // 0x26
    quint8 RLAZP_(); // 0x27
    quint8 PLPREL(); // 0x28
    quint8 ANDIMM(); // 0x29
    quint8 ROLAAA(); // 0x2A
    quint8 BITABS(); // 0x2C
    quint8 ANDABS(); // 0x2D
    quint8 ROLABS(); // 0x2E
    quint8 RLAABS(); // 0x2F
    quint8 BMIREL(); // 0x30
    quint8 ANDIZY(); // 0x31
    quint8 RLAIZY(); // 0x33
    quint8 ANDZPX(); // 0x35
    quint8 ROLZPX(); // 0x36
    quint8 RLAZPX(); // 0x37
    quint8 SECREL(); // 0x38
    quint8 ANDABY(); // 0x39
    quint8 RLAABY(); // 0x3B
    quint8 ANDABX(); // 0x3D
    quint8 ROLABX(); // 0x3E
    quint8 RLAABX(); // 0x3F
    quint8 RTIREL(); // 0x40
    quint8 EORIZX(); // 0x41
    quint8 SREIZX(); // 0x43
    quint8 EORZP_(); // 0x45
    quint8 LSRZP_(); // 0x46
    quint8 SREZP_(); // 0x47
    quint8 PHAREL(); // 0x48
    quint8 EORIMM(); // 0x49
    quint8 LSRAAA(); // 0x4A
    quint8 ASRIMM(); // 0x4B
    quint8 JMPABS(); // 0x4C
    quint8 EORABS(); // 0x4D
    quint8 LSRABS(); // 0x4E
    quint8 SREABS(); // 0x4F
    quint8 BVCREL(); // 0x50
    quint8 EORIZY(); // 0x51
    quint8 SREIZY(); // 0x53
    quint8 EORZPX(); // 0x55
    quint8 LSRZPX(); // 0x56
    quint8 SREZPX(); // 0x57
    quint8 CLIREL(); // 0x58
    quint8 EORABY(); // 0x59
    quint8 SREABY(); // 0x5B
    quint8 EORABX(); // 0x5D
    quint8 LSRABX(); // 0x5E
    quint8 SREABX(); // 0x5F
    quint8 RTSREL(); // 0x60
    quint8 ADCIZX(); // 0x61
    quint8 RRAIZX(); // 0x63
    quint8 ADCZP_(); // 0x65
    quint8 RORZP_(); // 0x66
    quint8 RRAZP_(); // 0x67
    quint8 PLAREL(); // 0x68
    quint8 ADCIMM(); // 0x69
    quint8 RORAAA(); // 0x6A
    quint8 ARRIMM(); // 0x6B
    quint8 JMPIZA(); // 0x6C
    quint8 ADCABS(); // 0x6D
    quint8 RORABS(); // 0x6E
    quint8 RRAABS(); // 0x6F
    quint8 BVSREL(); // 0x70
    quint8 ADCIZY(); // 0x71
    quint8 RRAIZY(); // 0x73
    quint8 ADCZPX(); // 0x75
    quint8 RORZPX(); // 0x76
    quint8 RRAZPX(); // 0x77
    quint8 SEIREL(); // 0x78
    quint8 ADCABY(); // 0x79
    quint8 RRAABY(); // 0x7B
    quint8 ADCABX(); // 0x7D
    quint8 RORABX(); // 0x7E
    quint8 RRAABX(); // 0x7F
    quint8 STAIZX(); // 0x81
    quint8 SAXIZX(); // 0x83
    quint8 STYZP_(); // 0x84
    quint8 STAZP_(); // 0x85
    quint8 STXZP_(); // 0x86
    quint8 SAXZP_(); // 0x87
    quint8 DEYREL(); // 0x88
    quint8 TXAREL(); // 0x8A
    quint8 ANEIMM(); // 0x8B
    quint8 STYABS(); // 0x8C
    quint8 STAABS(); // 0x8D
    quint8 STXABS(); // 0x8E
    quint8 SAXABS(); // 0x8F
    quint8 BCCREL(); // 0x90
    quint8 STAIZY(); // 0x91
    quint8 SHAIZY(); // 0x93
    quint8 STYZPX(); // 0x94
    quint8 STAZPX(); // 0x95
    quint8 STXZPY(); // 0x96
    quint8 SAXZPY(); // 0x97
    quint8 TYAREL(); // 0x98
    quint8 STAABY(); // 0x99
    quint8 TXSREL(); // 0x9A
    quint8 SHSABY(); // 0x9B
    quint8 SHYABX(); // 0x9C
    quint8 STAABX(); // 0x9D
    quint8 SHXABY(); // 0x9E
    quint8 SHAABY(); // 0x9F
    quint8 LDYIMM(); // 0xA0
    quint8 LDAIZX(); // 0xA1
    quint8 LDXIMM(); // 0xA2
    quint8 LAXIZX(); // 0xA3
    quint8 LDYZP_(); // 0xA4
    quint8 LDAZP_(); // 0xA5
    quint8 LDXZP_(); // 0xA6
    quint8 LAXZP_(); // 0xA7
    quint8 TAYREL(); // 0xA8
    quint8 LDAIMM(); // 0xA9
    quint8 TAXREL(); // 0xAA
    quint8 LXAIMM(); // 0xAB
    quint8 LDYABS(); // 0xAC
    quint8 LDAABS(); // 0xAD
    quint8 LDXABS(); // 0xAE
    quint8 LAXABS(); // 0xAF
    quint8 BCSREL(); // 0xB0
    quint8 LDAIZY(); // 0xB1
    quint8 LAXIZY(); // 0xB3
    quint8 LDYZPX(); // 0xB4
    quint8 LDAZPX(); // 0xB5
    quint8 LDXZPY(); // 0xB6
    quint8 LAXZPY(); // 0xB7
    quint8 CLVREL(); // 0xB8
    quint8 LDAABY(); // 0xB9
    quint8 TSXREL(); // 0xBA
    quint8 LASABY(); // 0xBB
    quint8 LDYABX(); // 0xBC
    quint8 LDAABX(); // 0xBD
    quint8 LDXABY(); // 0xBE
    quint8 LAXABY(); // 0xBF
    quint8 CPYIMM(); // 0xC0
    quint8 CMPIZX(); // 0xC1
    quint8 DCPIZX(); // 0xC3
    quint8 CPYZP_(); // 0xC4
    quint8 CMPZP_(); // 0xC5
    quint8 DECZP_(); // 0xC6
    quint8 DCPZP_(); // 0xC7
    quint8 INYREL(); // 0xC8
    quint8 CMPIMM(); // 0xC9
    quint8 DEXREL(); // 0xCA
    quint8 SBXIMM(); // 0xCB
    quint8 CPYABS(); // 0xCC
    quint8 CMPABS(); // 0xCD
    quint8 DECABS(); // 0xCE
    quint8 DCPABS(); // 0xCF
    quint8 BNEREL(); // 0xD0
    quint8 CMPIZY(); // 0xD1
    quint8 DCPIZY(); // 0xD3
    quint8 CMPZPX(); // 0xD5
    quint8 DECZPX(); // 0xD6
    quint8 DCPZPX(); // 0xD7
    quint8 CLDREL(); // 0xD8
    quint8 CMPABY(); // 0xD9
    quint8 DCPABY(); // 0xDB
    quint8 CMPABX(); // 0xDD
    quint8 DECABX(); // 0xDE
    quint8 DCPABX(); // 0xDF
    quint8 CPXIMM(); // 0xE0
    quint8 SBCIZX(); // 0xE1
    quint8 ISBIZX(); // 0xE3
    quint8 CPXZP_(); // 0xE4
    quint8 SBCZP_(); // 0xE5
    quint8 INCZP_(); // 0xE6
    quint8 ISBZP_(); // 0xE7
    quint8 INXREL(); // 0xE8
    quint8 SBCIMM(); // 0xE9, 0xEB
    quint8 CPXABS(); // 0xEC
    quint8 SBCABS(); // 0xED
    quint8 INCABS(); // 0xEE
    quint8 ISBABS(); // 0xEF
    quint8 BEQREL(); // 0xF0
    quint8 SBCIZY(); // 0xF1
    quint8 ISBIZY(); // 0xF3
    quint8 SBCZPX(); // 0xF5
    quint8 INCZPX(); // 0xF6
    quint8 ISBZPX(); // 0xF7
    quint8 SEDREL(); // 0xF8
    quint8 SBCABY(); // 0xF9
    quint8 ISBABY(); // 0xFB
    quint8 SBCABX(); // 0xFD
    quint8 INCABX(); // 0xFE
    quint8 ISBABX(); // 0xFF
    quint8 NOPREL(); // 0x1A, 0x3A, 0x5A, 0x7A, 0xDA, 0xFA
    quint8 DOP__2(); // 0x80, 0x82, 0x89, 0xC2, 0xE2
    quint8 DOP__3(); // 0x04, 0x44, 0x64
    quint8 DOP__4(); // 0x14, 0x34, 0x54, 0x74, 0xD4, 0xF4
    quint8 TOPREL(); // 0x0C, 0x1C, 0x3C, 0x5C, 0x7C, 0xDC, 0xFC
    quint8 KILLED(); // 0x02, 0x12, 0x22, 0x32, 0x42, 0x52, 0x62, 0x72, 0x92, 0xB2, 0xD2, 0xF2

    opFunc operations[256] = {
//      0       1       2       3       4       5       6       7       8       9       A       B       C       D       E       F
/*0+ */    &Cpu::BRKREL, &Cpu::ORAIZX, &Cpu::KILLED, &Cpu::SLOIZX, &Cpu::DOP__3, &Cpu::ORAZP_, &Cpu::ASLZP_, &Cpu::SLOZP_, &Cpu::PHPREL, &Cpu::ORAIMM, &Cpu::ASLAAA, &Cpu::ANCIMM, &Cpu::TOPREL, &Cpu::ORAABS, &Cpu::ASLABS, &Cpu::SLOABS, //0+
/*10+*/    &Cpu::BPLREL, &Cpu::ORAIZY, &Cpu::KILLED, &Cpu::SLOIZY, &Cpu::DOP__4, &Cpu::ORAZPX, &Cpu::ASLZPX, &Cpu::SLOZPX, &Cpu::CLCREL, &Cpu::ORAABY, &Cpu::NOPREL, &Cpu::SLOABY, &Cpu::TOPREL, &Cpu::ORAABX, &Cpu::ASLABX, &Cpu::SLOABX, //10+
/*20+*/    &Cpu::JSRREL, &Cpu::ANDIZX, &Cpu::KILLED, &Cpu::RLAIZX, &Cpu::BITZP_, &Cpu::ANDZP_, &Cpu::ROLZP_, &Cpu::RLAZP_, &Cpu::PLPREL, &Cpu::ANDIMM, &Cpu::ROLAAA, &Cpu::ANCIMM, &Cpu::BITABS, &Cpu::ANDABS, &Cpu::ROLABS, &Cpu::RLAABS, //20+
/*30+*/    &Cpu::BMIREL, &Cpu::ANDIZY, &Cpu::KILLED, &Cpu::RLAIZY, &Cpu::DOP__4, &Cpu::ANDZPX, &Cpu::ROLZPX, &Cpu::RLAZPX, &Cpu::SECREL, &Cpu::ANDABY, &Cpu::NOPREL, &Cpu::RLAABY, &Cpu::TOPREL, &Cpu::ANDABX, &Cpu::ROLABX, &Cpu::RLAABX, //30+
/*40+*/    &Cpu::RTIREL, &Cpu::EORIZX, &Cpu::KILLED, &Cpu::SREIZX, &Cpu::DOP__3, &Cpu::EORZP_, &Cpu::LSRZP_, &Cpu::SREZP_, &Cpu::PHAREL, &Cpu::EORIMM, &Cpu::LSRAAA, &Cpu::ASRIMM, &Cpu::JMPABS, &Cpu::EORABS, &Cpu::LSRABS, &Cpu::SREABS, //40+
/*50+*/    &Cpu::BVCREL, &Cpu::EORIZY, &Cpu::KILLED, &Cpu::SREIZY, &Cpu::DOP__4, &Cpu::EORZPX, &Cpu::LSRZPX, &Cpu::SREZPX, &Cpu::CLIREL, &Cpu::EORABY, &Cpu::NOPREL, &Cpu::SREABY, &Cpu::TOPREL, &Cpu::EORABX, &Cpu::LSRABX, &Cpu::SREABX, //50+
/*60+*/    &Cpu::RTSREL, &Cpu::ADCIZX, &Cpu::KILLED, &Cpu::RRAIZX, &Cpu::DOP__3, &Cpu::ADCZP_, &Cpu::RORZP_, &Cpu::RRAZP_, &Cpu::PLAREL, &Cpu::ADCIMM, &Cpu::RORAAA, &Cpu::ARRIMM, &Cpu::JMPIZA, &Cpu::ADCABS, &Cpu::RORABS, &Cpu::RRAABS, //60+
/*70+*/    &Cpu::BVSREL, &Cpu::ADCIZY, &Cpu::KILLED, &Cpu::RRAIZY, &Cpu::DOP__4, &Cpu::ADCZPX, &Cpu::RORZPX, &Cpu::RRAZPX, &Cpu::SEIREL, &Cpu::ADCABY, &Cpu::NOPREL, &Cpu::RRAABY, &Cpu::TOPREL, &Cpu::ADCABX, &Cpu::RORABX, &Cpu::RRAABX, //70+
/*80+*/    &Cpu::DOP__2, &Cpu::STAIZX, &Cpu::DOP__2, &Cpu::SAXIZX, &Cpu::STYZP_, &Cpu::STAZP_, &Cpu::STXZP_, &Cpu::SAXZP_, &Cpu::DEYREL, &Cpu::DOP__2, &Cpu::TXAREL, &Cpu::ANEIMM, &Cpu::STYABS, &Cpu::STAABS, &Cpu::STXABS, &Cpu::SAXABS, //80+
/*90+*/    &Cpu::BCCREL, &Cpu::STAIZY, &Cpu::KILLED, &Cpu::SHAIZY, &Cpu::STYZPX, &Cpu::STAZPX, &Cpu::STXZPY, &Cpu::SAXZPY, &Cpu::TYAREL, &Cpu::STAABY, &Cpu::TXSREL, &Cpu::SHSABY, &Cpu::SHYABX, &Cpu::STAABX, &Cpu::SHXABY, &Cpu::SHAABY, //90+
/*A0+*/    &Cpu::LDYIMM, &Cpu::LDAIZX, &Cpu::LDXIMM, &Cpu::LAXIZX, &Cpu::LDYZP_, &Cpu::LDAZP_, &Cpu::LDXZP_, &Cpu::LAXZP_, &Cpu::TAYREL, &Cpu::LDAIMM, &Cpu::TAXREL, &Cpu::LXAIMM, &Cpu::LDYABS, &Cpu::LDAABS, &Cpu::LDXABS, &Cpu::LAXABS, //A0+
/*B0+*/    &Cpu::BCSREL, &Cpu::LDAIZY, &Cpu::KILLED, &Cpu::LAXIZY, &Cpu::LDYZPX, &Cpu::LDAZPX, &Cpu::LDXZPY, &Cpu::LAXZPY, &Cpu::CLVREL, &Cpu::LDAABY, &Cpu::TSXREL, &Cpu::LASABY, &Cpu::LDYABX, &Cpu::LDAABX, &Cpu::LDXABY, &Cpu::LAXABY, //B0+
/*C0+*/    &Cpu::CPYIMM, &Cpu::CMPIZX, &Cpu::DOP__2, &Cpu::DCPIZX, &Cpu::CPYZP_, &Cpu::CMPZP_, &Cpu::DECZP_, &Cpu::DCPZP_, &Cpu::INYREL, &Cpu::CMPIMM, &Cpu::DEXREL, &Cpu::SBXIMM, &Cpu::CPYABS, &Cpu::CMPABS, &Cpu::DECABS, &Cpu::DCPABS, //C0+
/*D0+*/    &Cpu::BNEREL, &Cpu::CMPIZY, &Cpu::KILLED, &Cpu::DCPIZY, &Cpu::DOP__4, &Cpu::CMPZPX, &Cpu::DECZPX, &Cpu::DCPZPX, &Cpu::CLDREL, &Cpu::CMPABY, &Cpu::NOPREL, &Cpu::DCPABY, &Cpu::TOPREL, &Cpu::CMPABX, &Cpu::DECABX, &Cpu::DCPABX, //D0+
/*E0+*/    &Cpu::CPXIMM, &Cpu::SBCIZX, &Cpu::DOP__2, &Cpu::ISBIZX, &Cpu::CPXZP_, &Cpu::SBCZP_, &Cpu::INCZP_, &Cpu::ISBZP_, &Cpu::INXREL, &Cpu::SBCIMM, &Cpu::NOPREL, &Cpu::SBCIMM, &Cpu::CPXABS, &Cpu::SBCABS, &Cpu::INCABS, &Cpu::ISBABS, //E0+
/*F0+*/    &Cpu::BEQREL, &Cpu::SBCIZY, &Cpu::KILLED, &Cpu::ISBIZY, &Cpu::DOP__4, &Cpu::SBCZPX, &Cpu::INCZPX, &Cpu::ISBZPX, &Cpu::SEDREL, &Cpu::SBCABY, &Cpu::NOPREL, &Cpu::ISBABY, &Cpu::TOPREL, &Cpu::SBCABX, &Cpu::INCABX, &Cpu::ISBABX, //F0+
    };
};
