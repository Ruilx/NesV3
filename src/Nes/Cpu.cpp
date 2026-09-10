#include "Cpu.h"

Cpu::Cpu(Nes *nes, QObject *parent)
    : QObject(parent),
      cpuBus(0x10000),
    internalRam(0x0800),
    internalRamDevice(this->internalRam),
      nes(nes) {
    const Bus::Mapping internalRamMapping{
        .start = 0x0000,
        .end = 0x1FFF,
        .priority = 0,
        .flags = AccessFlags::Readable | AccessFlags::Writable,
        .name = QStringLiteral("Internal RAM"),
        .device = &this->internalRamDevice,
        .translate = [](quint16 address) {
            return static_cast<quint16>(address & 0x07FF);
        },
    };
    this->cpuBus.registerMapping(internalRamMapping);
}

quint8 Cpu::readRam8(quint16 addr) {
    if (addr < 0x2000) {
        quint8 value = this->cpuBus.openBusValue();
        this->cpuBus.read(addr, value);
        return value;
    } else if (addr < 0x8000) {
        // others
        //return this->nes->read(addr);
    } else {
        quint8 value = this->cpuBus.openBusValue();
        this->cpuBus.read(addr, value);
        return value;
    }
    return 0;
}

quint16 Cpu::readRam16(quint16 addr) {
    if (addr < 0x2000) {
        quint8 low = this->cpuBus.openBusValue();
        quint8 high = this->cpuBus.openBusValue();
        this->cpuBus.read(addr, low);
        this->cpuBus.read(static_cast<quint16>(addr + 1), high);
        return static_cast<quint16>(low | (high << 8));
    } else if (addr < 0x8000) {
        // others
        //return this->nes->read(addr);
        return 0;
    } else {
        quint8 low = this->cpuBus.openBusValue();
        quint8 high = this->cpuBus.openBusValue();
        this->cpuBus.read(addr, low);
        this->cpuBus.read(static_cast<quint16>(addr + 1), high);
        return static_cast<quint16>(low | (high << 8));
    }

}

void Cpu::writeRam(quint16 addr, quint8 value) {
    if (addr < 0x2000) {
        this->cpuBus.write(addr, value);
    } else {
        // others
        // nes->write(addr, data);
    }
}

void Cpu::reset() {
    this->reg.a = 0x00;
    this->reg.x = 0x00;
    this->reg.y = 0x00;
    this->reg.s = 0xFF;
    this->reg.p = ZFlag | RFlag;
    this->reg.pc = this->readRam16(Cpu::ResVector);
    this->reg.intPending = CpuInterrupt::None;

    this->totalCycles = 0;
    this->dmaCycles = 0;

    // stack quick access
    this->stack = &this->internalRam;
    this->stack_offset = 0x0100;

    // zero/negative flag
    this->znTable[0] = ZFlag;
    for (quint16 i = 1; i < 256; i++) {
        this->znTable[i] = (i & 0x80) ? NFlag : 0;
    }
}

void Cpu::clock() {
    ++this->totalCycles;
}

quint8 Cpu::op8(quint16 addr) {
    quint8 value = this->cpuBus.openBusValue();
    this->cpuBus.read(addr, value);
    return value;
}

quint16 Cpu::op16(quint16 addr) {
    quint8 low = this->cpuBus.openBusValue();
    quint8 high = this->cpuBus.openBusValue();
    this->cpuBus.read(addr, low);
    this->cpuBus.read(static_cast<quint16>(addr + 1), high);
    return static_cast<quint16>(low | (high << 8));
}

quint8 Cpu::zeroPageRead(quint8 addr) {
    quint8 value = this->cpuBus.openBusValue();
    this->cpuBus.read(addr, value);
    return value;
}

//--------------- 以下是我从之前的cpu.cpp 文件实现指令的部分代码

#include <Nes/Mmu.h>

quint8 Cpu::opCpu(quint16 addr) { return mmu->getCpuMemBank()[addr >> 13][addr & 0x1FFF];}
quint16 Cpu::opCpuW(quint16 addr) { return *((quint16) &mmu->getCpuMemBank()[addr >> 13][addr & 0x1FFF]);}

quint8 Cpu::zeroPageRead(quint8 addr) const { return mmu->getRam()[addr]; }
quint16 Cpu::zeroPageReadW(quint8 addr) const { return (quint16)mmu->getRam()[addr] | ((quint16)mmu->getRam()[addr + 1] << 8); }
void Cpu::zeroPageWrite(quint8 addr, quint8 value) { mmu->getRam()[addr] = value; }
void Cpu::zeroPageWriteW(quint8 addr, quint16 value) { mmu->getRam()[addr] = value & 0xFF; mmu->getRam()[addr + 1] = (value >> 8);}

quint8 Cpu::checkEa() const { return (this->et & 0xFF00) != (this->ea & 0xFF00) ? 1 : 0; }

void Cpu::setZnFlag(quint8 flag) {
    this->r.p &= ~(Cpu::ZFlag | Cpu::NFlag);
    this->r.p |= this->znTable[flag];
}

void Cpu::setFlag(quint8 flag) { this->r.p |= flag; }
void Cpu::clearFlag(quint8 flag) { this->r.p &= ~flag; }
void Cpu::testFlag(bool ok, quint8 flag) {
    this->clearFlag(flag);
    if(ok){
        this->setFlag(flag);
    }
}
bool Cpu::checkFlag(quint8 flag) const { return this->r.p & flag; }

void Cpu::mrIm() {
    this->dt = this->opCpu(this->r.pc++);
}

void Cpu::mrZp() {
    this->ea = this->opCpu(this->r.pc++);
    this->dt = this->zeroPageRead(this->ea);
}

void Cpu::mrZx() {
    this->dt = this->opCpu(this->r.pc++);
    this->ea = (quint8)(this->dt + this->r.x);
    this->dt = this->zeroPageRead(this->ea);
}

void Cpu::mrZy() {
    this->dt = this->opCpu(this->r.pc++);
    this->ea = (quint8)(this->dt + this->r.y);
    this->dt = this->zeroPageRead(this->ea);
}

void Cpu::mrAb() {
    this->ea = this->opCpuW(this->r.pc);
    this->r.pc += 2;
    this->dt = this->rdCpu(this->ea);
}

void Cpu::mrAx() {
    this->et = this->opCpuW(this->r.pc);
    this->r.pc += 2;
    this->ea = this->et + this->r.x;
    this->dt = this->rdCpu(this->ea);
}

void Cpu::mrAy() {
    this->et = this->opCpuW(this->r.pc);
    this->r.pc += 2;
    this->ea = this->et + this->r.y;
    this->dt = this->rdCpu(this->ea);
}

void Cpu::mrIx() {
    this->dt = this->opCpu(this->r.pc++);
    this->ea = this->zeroPageReadW(this->dt + this->r.x);
    this->dt = this->rdCpu(this->ea);
}

void Cpu::mrIy() {
    this->dt = this->opCpu(this->r.pc++);
    this->et = this->zeroPageReadW(this->dt);
    this->ea = this->et + this->r.y;
    this->dt = this->rdCpu(this->ea);
}

void Cpu::eaZp() {
    this->ea = this->opCpu(this->r.pc++);
}

void Cpu::eaZx() {
    this->dt = this->opCpu(this->r.pc++);
    this->ea = (quint8)(this->dt + this->r.x);
}

void Cpu::eaZy() {
    this->dt = this->opCpu(this->r.pc++);
    this->ea = (quint8)(this->dt + this->r.y);
}

void Cpu::eaAb() {
    this->ea = this->opCpuW(this->r.pc);
    this->r.pc += 2;
}

void Cpu::eaAx() {
    this->et = this->opCpuW(this->r.pc);
    this->r.pc += 2;
    this->ea = this->et + this->r.x;
}

void Cpu::eaAy() {
    this->et = this->opCpuW(this->r.pc);
    this->r.pc += 2;
    this->ea = this->et + this->r.y;
}

void Cpu::eaIx() {
    this->dt = this->opCpu(this->r.pc++);
    this->ea = this->zeroPageReadW(this->dt + this->r.x);
}

void Cpu::eaIy() {
    this->dt = this->opCpu(this->r.pc++);
    this->et = this->zeroPageReadW(this->dt);
    this->ea = this->et + (quint16)this->r.y;
}

void Cpu::mwZp() {
    this->zeroPageWrite(this->ea, this->dt);
}

void Cpu::mwEa() {
    this->wrCpu(this->ea, this->dt);
}

void Cpu::push(quint8 data){ this->stack[(this->r.s--) & 0xFF] = data; }
quint8 Cpu::pop(){ return this->stack[(++this->r.s) & 0xFF]; }
quint8 Cpu::popAndSetZnFlag() { quint8 t = this->pop(); this->setZnFlag(t); return t;}

void Cpu::adc(){
    this->wt = this->r.a + this->dt + (this->r.p & Cpu::CFlag);
    this->testFlag(this->wt > 0xFF, Cpu::CFlag);
    this->testFlag(((~(this->r.a ^ this->dt)) & (this->r.a ^ this->wt) & 0x80), Cpu::VFlag);
    this->r.a = (quint8)this->wt;
    this->setZnFlag(this->r.a);
}

void Cpu::sbc(){
    this->wt = this->r.a - this->dt - (~this->r.p & Cpu::CFlag);
    this->testFlag(((this->r.a ^ this->dt) & (this->r.a ^ this->wt) & 0x80), Cpu::VFlag);
    this->testFlag(this->wt < 0x100, Cpu::CFlag);
    this->r.a = (quint8)this->wt;
    this->setZnFlag(this->r.a);
}

void Cpu::inc(){
    this->dt++;
    this->setZnFlag(this->dt);
}

void Cpu::inx(){
    this->r.x++;
    this->setZnFlag(this->r.x);
}

void Cpu::iny(){
    this->r.y++;
    this->setZnFlag(this->r.y);
}

void Cpu::dec(){
    this->dt--;
    this->setZnFlag(this->dt);
}

void Cpu::dex(){
    this->r.x--;
    this->setZnFlag(this->r.x);
}

void Cpu::dey(){
    this->r.y--;
    this->setZnFlag(this->r.y);
}

void Cpu::_and(){
    this->r.a &= this->dt;
    this->setZnFlag(this->r.a);
}

void Cpu::ora() {
    this->r.a |= this->dt;
    this->setZnFlag(this->r.a);
}

void Cpu::eor(){
    this->r.a ^= this->dt;
    this->setZnFlag(this->r.a);
}

void Cpu::asla(){
    this->testFlag(this->r.a & 0x80, Cpu::CFlag);
    this->r.a <<= 1;
    this->setZnFlag(this->r.a);
}

void Cpu::asl(){
    this->testFlag(this->dt & 0x80, Cpu::CFlag);
    this->dt <<= 1;
    this->setZnFlag(this->dt);
}

void Cpu::lsra(){
    this->testFlag(this->r.a & 0x01, Cpu::CFlag);
    this->r.a >>= 1;
    this->setZnFlag(this->r.a);
}

void Cpu::lsr(){
    this->testFlag(this->dt & 0x01, Cpu::CFlag);
    this->dt >>= 1;
    this->setZnFlag(this->dt);
}

void Cpu::rola(){
    if(this->r.p & Cpu::CFlag){
        this->testFlag(this->r.a & 0x80, Cpu::CFlag);
        this->r.a = (this->r.a << 1) | 0x01;
    }else{
        this->testFlag(this->r.a & 0x80, Cpu::CFlag);
        this->r.a <<= 1;
    }
    this->setZnFlag(this->r.a);
}

void Cpu::rol(){
    if(this->r.p & Cpu::CFlag){
        this->testFlag(this->dt & 0x80, Cpu::CFlag);
        this->dt = (this->dt << 1) | 0x01;
    }else{
        this->testFlag(this->dt & 0x80, Cpu::CFlag);
        this->dt <<= 1;
    }
    this->setZnFlag(this->dt);
}

void Cpu::rora(){
    if(this->r.p & Cpu::CFlag){
        this->testFlag(this->r.a & 0x01, Cpu::CFlag);
        this->r.a = (this->r.a >> 1) | 0x80;
    }else{
        this->testFlag(this->r.a & 0x01, Cpu::CFlag);
        this->r.a >>= 1;
    }
    this->setZnFlag(this->r.a);
}

void Cpu::ror(){
    if(this->r.p & Cpu::CFlag){
        this->testFlag(this->dt & 0x01, Cpu::CFlag);
        this->dt = (this->dt >> 1) | 0x80;
    }else{
        this->testFlag(this->dt & 0x01, Cpu::CFlag);
        this->dt >>= 1;
    }
    this->setZnFlag(this->dt);
}

void Cpu::bit(){
    this->testFlag((this->dt & this->r.a) == 0, Cpu::ZFlag);
    this->testFlag(this->dt & 0x80, Cpu::NFlag);
    this->testFlag(this->dt & 0x40, Cpu::VFlag);
}

void Cpu::lda(){
    this->r.a = this->dt;
    this->setZnFlag(this->r.a);
}

void Cpu::ldx(){
    this->r.x = this->dt;
    this->setZnFlag(this->r.x);
}

void Cpu::ldy(){
    this->r.y = this->dt;
    this->setZnFlag(this->r.y);
}

void Cpu::sta(){ this->dt = this->r.a; }

void Cpu::stx(){ this->dt = this->r.x; }

void Cpu::sty(){ this->dt = this->r.y; }

void Cpu::tax(){
    this->r.x = this->r.a;
    this->setZnFlag(this->r.x);
}

void Cpu::txa(){
    this->r.a = this->r.x;
    this->setZnFlag(this->r.a);
}

void Cpu::tay(){
    this->r.y = this->r.a;
    this->setZnFlag(this->r.y);
}

void Cpu::tya(){
    this->r.a = this->r.y;
    this->setZnFlag(this->r.a);
}

void Cpu::tsx(){
    this->r.x = this->r.s;
    this->setZnFlag(this->r.x);
}

void Cpu::txs(){
    this->r.s = this->r.x;
}

void Cpu::cmp(){
    this->wt = (quint16)this->r.a - (quint16)this->dt;
    this->testFlag((this->wt & 0x8000) == 0, Cpu::CFlag);
    this->setZnFlag((quint8)this->wt);
};

void Cpu::cpx(){
    this->wt = (quint16)this->r.x - (quint16)this->dt;
    this->testFlag((this->wt & 0x8000) == 0, Cpu::CFlag);
    this->setZnFlag((quint8)this->wt);
}

void Cpu::cpy(){
    this->wt = (quint16)this->r.y - (quint16)this->dt;
    this->testFlag((this->wt & 0x8000) == 0, Cpu::CFlag);
    this->setZnFlag((quint8)this->wt);
}

void Cpu::jmpId(){
    this->wt = this->opCpuW(this->r.pc);
    this->ea = this->rdCpu(this->wt);
    this->wt = (this->wt & 0xFF00) | ((this->wt + 1) & 0x00FF);
    this->r.pc = this->ea + this->rdCpu(this->wt) * 0x100;
}

void Cpu::jmp(){
    this->r.pc = this->opCpuW(this->r.pc);
}

void Cpu::jsr(){
    this->ea = this->opCpuW(this->r.pc);
    this->r.pc++;
    this->push(this->r.pc >> 8);
    this->push(this->r.pc & 0xFF);
    this->r.pc = this->ea;
}

void Cpu::rts(){
    this->r.pc = this->pop();
    this->r.pc |= this->pop() << 2;
    this->r.pc++;
}

void Cpu::rti(){
    this->r.p = this->pop() | Cpu::RFlag;
    this->r.pc = this->pop();
    this->r.pc |= this->pop() << 4;
}

quint8 Cpu::_nmi(){
    this->push(this->r.pc >> 8);
    this->push(this->r.pc & 0xFF);
    this->clearFlag(Cpu::BFlag);
    this->push(this->r.p);
    this->setFlag(Cpu::IFlag);
    this->r.pc = this->rdCpuW(Cpu::NmiVector);
    return 7;
}

quint8 Cpu::_irq(){
    this->push(this->r.pc >> 8);
    this->push(this->r.pc & 0xFF);
    this->clearFlag(Cpu::BFlag);
    this->push(this->r.p);
    this->setFlag(Cpu::IFlag);
    this->r.pc = this->rdCpuW(Cpu::IrqVector);
    return 7;
}

void Cpu::brk(){
    this->r.pc++;
    this->push(this->r.pc >> 8);
    this->push(this->r.pc & 0xFF);
    this->setFlag(Cpu::BFlag);
    this->push(this->r.p);
    this->setFlag(Cpu::IFlag);
    this->r.pc = this->rdCpuW(Cpu::IrqVector);
}

void Cpu::relJump(){
    this->et = this->r.pc;
    this->ea = this->r.pc + (qint8)this->dt;
    this->r.pc = this->ea;
    return 1;
    this->checkEa();
}

void Cpu::bcc(){
    if(!(this->r.p & Cpu::CFlag)){
        this->relJump();
    }
}

void Cpu::bcs(){
    if(this->r.p & Cpu::CFlag){
        this->relJump();
    }
}

void Cpu::bne(){
    if(!(this->r.p & Cpu::ZFlag)){
        this->relJump()
    }
}

void Cpu::beq(){
    if(this->r.p & Cpu::ZFlag){
        this->relJump();
    }
}

void Cpu::bpl(){
    if(!(this->r.p & Cpu::NFlag)){
        this->relJump();
    }
}

void Cpu::bmi(){
    if(this->r.p & Cpu::NFlag){
        this->relJump();
    }
}

void Cpu::bvc(){
    if(!(this->r.p & Cpu::VFlag)){
        this->relJump();
    }
}

void Cpu::bvs(){
    if(this->r.p & Cpu::VFlag){
        this->relJump();
    }
}

void Cpu::clc(){ this->r.p &= ~Cpu::CFlag; }
void Cpu::cld(){ this->r.p &= ~Cpu::DFlag; }
void Cpu::cli(){ this->r.p &= ~Cpu::IFlag; }
void Cpu::clv(){ this->r.p &= ~Cpu::VFlag; }
void Cpu::sec(){ this->r.p |= Cpu::CFlag; }
void Cpu::sed(){ this->r.p |= Cpu::DFlag; }
void Cpu::sei(){ this->r.p |= Cpu::IFlag; }

void Cpu::anc(){
    this->r.a &= this->dt;
    this->setZnFlag(this->r.a);
    this->testFlag(this->r.p & Cpu::NFlag, Cpu::CFlag);
}

void Cpu::ane(){
    this->r.a = (this->r.a | 0xEE) & this->r.x & this->dt;
    this->setZnFlag(this->r.a);
}

void Cpu::arr(){
    this->dt &= this->r.a;
    this->r.a = (this->dt >> 1) | ((this->r.p & Cpu::CFlag) << 7);
    this->setZnFlag(this->r.a);
    this->testFlag(this->r.a & 0x40, Cpu::CFlag);
    this->testFlag((this->r.a >> 6) ^ (this->r.a >> 5), Cpu::VFlag);
}

void Cpu::asr(){
    this->dt &= this->r.a;
    this->testFlag(this->dt & 0x01, Cpu::CFlag);
    this->r.a = this->dt >> 1;
    this->setZnFlag(this->r.a);
}

void Cpu::dcp(){
    this->dt--;
    this->cmp();
}

void Cpu::isb(){
    this->dt++;
    this->sbc();
}

void Cpu::las(){
    this->r.s = this->r.s & this->dt;
    this->r.x = this->r.s;
    this->r.a = this->r.s;
    this->setZnFlag(this->r.a);
}

void Cpu::lax(){
    this->r.a = this->dt;
    this->r.x = this->r.a;
    this->setZnFlag(this->r.a);
}

void Cpu::lxa(){
    this->r.x = ((this->r.a | 0xEE) & this->dt);
    this->r.a = this->r.x;
    this->setZnFlag(this->r.a);
}

void Cpu::rla(){
    if(this->r.p & Cpu::CFlag){
        this->testFlag(this->dt & 0x80, Cpu::CFlag);
        this->dt = (this->dt << 1) | 1;
    }else{
        this->testFlag(this->dt & 0x80, Cpu::CFlag);
        this->dt <<= 1;
    }
    this->r.a &= this->dt;
    this->setZnFlag(this->r.a);
}

void Cpu::rra(){
    if(this->r.p & Cpu::CFlag){
        this->testFlag(this->dt & 0x01, Cpu::CFlag);
        this->dt = (this->dt >> 1) | 0x80;
    }else{
        this->testFlag(this->dt & 0x01, Cpu::CFlag);
        this->dt >>= 1;
    }
    this->adc();
}

void Cpu::sax(){ this->dt = this->r.a & this->r.x; }

void Cpu::sbx(){
    this->wt = (this->r.a & this->r.x) - this->dt;
    this->testFlag(this->wt < 0x100, Cpu::CFlag);
    this->r.x = this->wt & 0xFF;
    this->setZnFlag(this->r.x);
}

void Cpu::sha(){
    this->dt = this->r.a & this->r.x & (quint8)((this->ea >> 8) + 1);
}

void Cpu::shs(){
    this->r.s = this->r.a & this->r.x;
    this->dt = this->r.s & (quint8)((this->ea >> 8) + 1);
}

void Cpu::shx(){
    this->dt = this->r.x & (quint8)((this->ea >> 8) + 1);
}

void Cpu::shy(){
    this->dt = this->r.y & (quint8)((this->ea >> 8) + 1);
}

void Cpu::slo(){
    this->testFlag(this->dt & 0x80, Cpu::CFlag);
    this->dt <<= 1;
    this->r.a |= this->dt;
    this->setZnFlag(this->r.a);
}

void Cpu::sre(){
    this->testFlag(this->dt & 0x01, Cpu::CFlag);
    this->dt >>= 1;
    this->r.a ^= this->dt;
    this->setZnFlag(this->r.a);
}

quint8 Cpu::BRKREL(){               this->brk();                   return 7; } // 0x00
quint8 Cpu::ORAIZX(){ this->mrIx(); this->ora();                   return 6; } // 0x01
quint8 Cpu::SLOIZX(){ this->mrIx(); this->slo();  this->mwEa();    return 8; } // 0x03
quint8 Cpu::ORAZP_(){ this->mrZp(); this->ora();                   return 3; } // 0x05
quint8 Cpu::ASLZP_(){ this->mrZp(); this->slo();  this->mwZp();    return 5; } // 0x06
quint8 Cpu::SLOZP_(){ this->mrZp(); this->slo();  this->mwZp();    return 5; } // 0x07
quint8 Cpu::PHPREL(){ this->push(this->r.p | Cpu::BFlag);          return 3; } // 0x08
quint8 Cpu::ORAIMM(){ this->mrIm(); this->ora();                   return 2; } // 0x09
quint8 Cpu::ASLAAA(){               this->asla();                  return 2; } // 0x0A
quint8 Cpu::ANCIMM(){ this->mrIm(); this->anc();                   return 2; } // 0x0B, 0x2B
quint8 Cpu::ORAABS(){ this->mrAb(); this->ora();                   return 4; } // 0x0D
quint8 Cpu::ASLABS(){ this->mrAb(); this->asl();  this->mwEa();    return 6; } // 0x0E
quint8 Cpu::SLOABS(){ this->mrAb(); this->slo();  this->mwEa();    return 6; } // 0x0F
quint8 Cpu::BPLREL(){ this->mrIm(); this->bpl();                   return 2; } // 0x10
quint8 Cpu::ORAIZY(){ this->mrIy(); this->ora();  return this->checkEa() +5; } // 0x11
quint8 Cpu::SLOIZY(){ this->mrIy(); this->slo();  this->mwEa();    return 8; } // 0x13
quint8 Cpu::ORAZPX(){ this->mrZx(); this->ora();                   return 3; } // 0x15
quint8 Cpu::ASLZPX(){ this->mrZx(); this->asl();  this->mwZp();    return 6; } // 0x16
quint8 Cpu::SLOZPX(){ this->mrZx(); this->slo();  this->mwZp();    return 5; } // 0x17
quint8 Cpu::CLCREL(){               this->clc();                   return 2; } // 0x18
quint8 Cpu::ORAABY(){ this->mrAy(); this->ora();  return this->checkEa() +4; } // 0x19
quint8 Cpu::SLOABY(){ this->mrAy(); this->slo();  this->mwEa();    return 7; } // 0x1B
quint8 Cpu::ORAABX(){ this->mrAx(); this->ora();  return this->checkEa() +4; } // 0x1D
quint8 Cpu::ASLABX(){ this->mrAx(); this->asl();  this->mwEa();    return 7; } // 0x1E
quint8 Cpu::SLOABX(){ this->mrAx(); this->slo();  this->mwEa();    return 7; } // 0x1F
quint8 Cpu::JSRREL(){               this->jsr();                   return 6; } // 0x20
quint8 Cpu::ANDIZX(){ this->mrIx(); this->_and();                  return 6; } // 0x21
quint8 Cpu::RLAIZX(){ this->mrIx(); this->rla();  this->mwEa();    return 8; } // 0x23
quint8 Cpu::BITZP_(){ this->mrZp(); this->bit();                   return 3; } // 0x24
quint8 Cpu::ANDZP_(){ this->mrZp(); this->_and();                  return 3; } // 0x25
quint8 Cpu::ROLZP_(){ this->mrZp(); this->rol();  this->mwZp();    return 5; } // 0x26
quint8 Cpu::RLAZP_(){ this->mrZp(); this->rla();  this->mwZp();    return 5; } // 0x27
quint8 Cpu::PLPREL(){ this->r.p = this->pop() | Cpu::RFlag;        return 4; } // 0x28
quint8 Cpu::ANDIMM(){ this->mrIm(); this->_and();                  return 2; } // 0x29
quint8 Cpu::ROLAAA(){               this->rola();                  return 2; } // 0x2A
quint8 Cpu::BITABS(){ this->mrAb(); this->bit();                   return 4; } // 0x2C
quint8 Cpu::ANDABS(){ this->mrAb(); this->_and();                  return 4; } // 0x2D
quint8 Cpu::ROLABS(){ this->mrAb(); this->rol();  this->mwEa();    return 6; } // 0x2E
quint8 Cpu::RLAABS(){ this->mrAb(); this->rla();  this->mwEa();    return 6; } // 0x2F
quint8 Cpu::BMIREL(){ this->mrIm(); this->bmi();                   return 2; } // 0x30
quint8 Cpu::ANDIZY(){ this->mrIy(); this->_and(); return this->checkEa() +5; } // 0x31
quint8 Cpu::RLAIZY(){ this->mrIy(); this->rla();  this->mwEa();    return 8; } // 0x33
quint8 Cpu::ANDZPX(){ this->mrZx(); this->_and();                  return 4; } // 0x35
quint8 Cpu::ROLZPX(){ this->mrZx(); this->rol();  this->mwZp();    return 6; } // 0x36
quint8 Cpu::RLAZPX(){ this->mrZx(); this->rla();  this->mwZp();    return 6; } // 0x37
quint8 Cpu::SECREL(){               this->sec();                   return 2; } // 0x38
quint8 Cpu::ANDABY(){ this->mrAy(); this->_and(); return this->checkEa() +4; } // 0x39
quint8 Cpu::RLAABY(){ this->mrAy(); this->rla();  this->mwEa();    return 7; } // 0x3B
quint8 Cpu::ANDABX(){ this->mrAx(); this->_and(); return this->checkEa() +4; } // 0x3D
quint8 Cpu::ROLABX(){ this->mrAb(); this->rol();  this->mwEa();    return 7; } // 0x3E
quint8 Cpu::RLAABX(){ this->mrAx(); this->rla();  this->mwEa();    return 7; } // 0x3F
quint8 Cpu::RTIREL(){               this->rti();                   return 6; } // 0x40
quint8 Cpu::EORIZX(){ this->mrIx(); this->eor();                   return 6; } // 0x41
quint8 Cpu::SREIZX(){ this->mrIx(); this->sre();  this->mwEa();    return 8; } // 0x43
quint8 Cpu::EORZP_(){ this->mrZp(); this->eor();                   return 3; } // 0x45
quint8 Cpu::LSRZP_(){ this->mrZp(); this->lsr();  this->mwZp();    return 5; } // 0x46
quint8 Cpu::SREZP_(){ this->mrZp(); this->sre();  this->mwZp();    return 5; } // 0x47
quint8 Cpu::PHAREL(){ this->push(this->r.a);                       return 3; } // 0x48
quint8 Cpu::EORIMM(){ this->mrIm(); this->eor();                   return 2; } // 0x49
quint8 Cpu::LSRAAA(){               this->lsra();                  return 2; } // 0x4A
quint8 Cpu::ASRIMM(){ this->mrIm(); this->asr();                   return 2; } // 0x4B
quint8 Cpu::JMPABS(){               this->jmp();                   return 3; } // 0x4C
quint8 Cpu::EORABS(){ this->mrAb(); this->eor();                   return 4; } // 0x4D
quint8 Cpu::LSRABS(){ this->mrAb(); this->lsr();  this->mwEa();    return 6; } // 0x4E
quint8 Cpu::SREABS(){ this->mrAb(); this->sre();  this->mwEa();    return 6; } // 0x4F
quint8 Cpu::BVCREL(){ this->mrIm(); this->bvc();                   return 2; } // 0x50
quint8 Cpu::EORIZY(){ this->mrIy(); this->eor();  return this->checkEa() +5; } // 0x51
quint8 Cpu::SREIZY(){ this->mrIy(); this->sre();  this->mwEa();    return 8; } // 0x53
quint8 Cpu::EORZPX(){ this->mrZx(); this->eor();                   return 4; } // 0x55
quint8 Cpu::LSRZPX(){ this->mrZx(); this->lsr();  this->mwZp();    return 6; } // 0x56
quint8 Cpu::SREZPX(){ this->mrZx(); this->sre();  this->mwZp();    return 6; } // 0x57
quint8 Cpu::CLIREL(){               this->cli();                   return 2; } // 0x58
quint8 Cpu::EORABY(){ this->mrAy(); this->eor();  return this->checkEa() +4; } // 0x59
quint8 Cpu::SREABY(){ this->mrAy(); this->sre();  this->mwEa();    return 7; } // 0x5B
quint8 Cpu::EORABX(){ this->mrAx(); this->eor();  return this->checkEa() +4; } // 0x5D
quint8 Cpu::LSRABX(){ this->mrAx(); this->lsr();  this->mwEa();    return 7; } // 0x5E
quint8 Cpu::SREABX(){ this->mrAx(); this->sre();  this->mwEa();    return 7; } // 0x5F
quint8 Cpu::RTSREL(){               this->rts();                   return 6; } // 0x60
quint8 Cpu::ADCIZX(){ this->mrIx(); this->adc();                   return 6; } // 0x61
quint8 Cpu::RRAIZX(){ this->mrIx(); this->rra();  this->mwEa();    return 8; } // 0x63
quint8 Cpu::ADCZP_(){ this->mrZp(); this->adc();                   return 3; } // 0x65
quint8 Cpu::RORZP_(){ this->mrZp(); this->ror();  this->mwZp();    return 5; } // 0x66
quint8 Cpu::RRAZP_(){ this->mrZp(); this->rra();  this->mwZp();    return 5; } // 0x67
quint8 Cpu::PLAREL(){ this->r.a = this->popAndSetZnFlag();         return 4; } // 0x68
quint8 Cpu::ADCIMM(){ this->mrIm(); this->adc();                   return 2; } // 0x69
quint8 Cpu::RORAAA(){               this->rora();                  return 2; } // 0x6A
quint8 Cpu::ARRIMM(){ this->mrIm(); this->arr();                   return 2; } // 0x6B
quint8 Cpu::JMPIZA(){               this->jmpId();                 return 5; } // 0x6C
quint8 Cpu::ADCABS(){ this->mrAb(); this->adc();                   return 4; } // 0x6D
quint8 Cpu::RORABS(){ this->mrAb(); this->ror();  this->mwEa();    return 6; } // 0x6E
quint8 Cpu::RRAABS(){ this->mrAb(); this->rra();  this->mwEa();    return 6; } // 0x6F
quint8 Cpu::BVSREL(){ this->mrIm(); this->bvs();                   return 2; } // 0x70
quint8 Cpu::ADCIZY(){ this->mrIy(); this->adc();  return this->checkEa() +4; } // 0x71
quint8 Cpu::RRAIZY(){ this->mrIy(); this->rra();  this->mwEa();    return 8; } // 0x73
quint8 Cpu::ADCZPX(){ this->mrZx(); this->adc();                   return 4; } // 0x75
quint8 Cpu::RORZPX(){ this->mrZx(); this->ror();  this->mwEa();    return 6; } // 0x76
quint8 Cpu::RRAZPX(){ this->mrZx(); this->rra();  this->mwZp();    return 6; } // 0x77
quint8 Cpu::SEIREL(){               this->sei();                   return 2; } // 0x78
quint8 Cpu::ADCABY(){ this->mrAy(); this->adc();  return this->checkEa() +4; } // 0x79
quint8 Cpu::RRAABY(){ this->mrAy(); this->rra();  this->mwEa();    return 7; } // 0x7B
quint8 Cpu::ADCABX(){ this->mrAx(); this->adc();  return this->checkEa() +4; } // 0x7D
quint8 Cpu::RORABX(){ this->mrAx(); this->ror();  this->mwEa();    return 7; } // 0x7E
quint8 Cpu::RRAABX(){ this->mrAx(); this->rra();  this->mwEa();    return 7; } // 0x7F
quint8 Cpu::STAIZX(){ this->eaIx(); this->sta();  this->mwEa();    return 6; } // 0x81
quint8 Cpu::SAXIZX(){ this->mrIx(); this->sax();  this->mwEa();    return 6; } // 0x83
quint8 Cpu::STYZP_(){ this->eaZp(); this->sty();  this->mwZp();    return 3; } // 0x84
quint8 Cpu::STAZP_(){ this->eaZp(); this->sta();  this->mwZp();    return 3; } // 0x85
quint8 Cpu::STXZP_(){ this->eaZp(); this->stx();  this->mwZp();    return 3; } // 0x86
quint8 Cpu::SAXZP_(){ this->mrZp(); this->sax();  this->mwZp();    return 3; } // 0x87
quint8 Cpu::DEYREL(){               this->dey();                   return 2; } // 0x88
quint8 Cpu::TXAREL(){               this->txa();                   return 2; } // 0x8A
quint8 Cpu::ANEIMM(){ this->mrIm(); this->ane();                   return 2; } // 0x8B
quint8 Cpu::STYABS(){ this->eaAb(); this->sty();  this->mwEa();    return 4; } // 0x8C
quint8 Cpu::STAABS(){ this->eaAb(); this->sta();  this->mwEa();    return 4; } // 0x8D
quint8 Cpu::STXABS(){ this->eaAb(); this->stx();  this->mwEa();    return 4; } // 0x8E
quint8 Cpu::SAXABS(){ this->mrAb(); this->sax();  this->mwEa();    return 4; } // 0x8F
quint8 Cpu::BCCREL(){ this->mrIm(); this->bcc();                   return 2; } // 0x90
quint8 Cpu::STAIZY(){ this->eaIy(); this->sta();  this->mwEa();    return 6; } // 0x91
quint8 Cpu::SHAIZY(){ this->mrIy(); this->sha();  this->mwEa();    return 6; } // 0x93
quint8 Cpu::STYZPX(){ this->eaZx(); this->sty();  this->mwZp();    return 4; } // 0x94
quint8 Cpu::STAZPX(){ this->eaZx(); this->sta();  this->mwZp();    return 4; } // 0x95
quint8 Cpu::STXZPY(){ this->eaZy(); this->stx();  this->mwZp();    return 4; } // 0x96
quint8 Cpu::SAXZPY(){ this->mrZy(); this->sax();  this->mwZp();    return 4; } // 0x97
quint8 Cpu::TYAREL(){               this->tya();                   return 2; } // 0x98
quint8 Cpu::STAABY(){ this->eaAy(); this->sta();  this->mwEa();    return 5; } // 0x99
quint8 Cpu::TXSREL(){               this->txs();                   return 2; } // 0x9A
quint8 Cpu::SHSABY(){ this->mrAy(); this->shs();  this->mwEa();    return 5; } // 0x9B
quint8 Cpu::SHYABX(){ this->mrAx(); this->shy();  this->mwEa();    return 5; } // 0x9C
quint8 Cpu::STAABX(){ this->eaAx(); this->sta();  this->mwEa();    return 5; } // 0x9D
quint8 Cpu::SHXABY(){ this->mrAy(); this->shx();  this->mwEa();    return 5; } // 0x9E
quint8 Cpu::SHAABY(){ this->mrAy(); this->sha();  this->mwEa();    return 5; } // 0x9F
quint8 Cpu::LDYIMM(){ this->mrIm(); this->ldy();                   return 2; } // 0xA0
quint8 Cpu::LDAIZX(){ this->mrIx(); this->lda();                   return 6; } // 0xA1
quint8 Cpu::LDXIMM(){ this->mrIm(); this->ldx();                   return 2; } // 0xA2
quint8 Cpu::LAXIZX(){ this->mrIx(); this->lax();                   return 6; } // 0xA3
quint8 Cpu::LDYZP_(){ this->mrZp(); this->ldy();                   return 3; } // 0xA4
quint8 Cpu::LDAZP_(){ this->mrZp(); this->lda();                   return 3; } // 0xA5
quint8 Cpu::LDXZP_(){ this->mrZp(); this->ldx();                   return 3; } // 0xA6
quint8 Cpu::LAXZP_(){ this->mrZp(); this->lax();                   return 3; } // 0xA7
quint8 Cpu::TAYREL(){               this->tay();                   return 2; } // 0xA8
quint8 Cpu::LDAIMM(){ this->mrIm(); this->lda();                   return 2; } // 0xA9
quint8 Cpu::TAXREL(){               this->tax();                   return 2; } // 0xAA
quint8 Cpu::LXAIMM(){ this->mrIm(); this->lxa();                   return 2; } // 0xAB
quint8 Cpu::LDYABS(){ this->mrAb(); this->ldy();                   return 4; } // 0xAC
quint8 Cpu::LDAABS(){ this->mrAb(); this->lda();                   return 4; } // 0xAD
quint8 Cpu::LDXABS(){ this->mrAb(); this->ldx();                   return 4; } // 0xAE
quint8 Cpu::LAXABS(){ this->mrAb(); this->lax();                   return 4; } // 0xAF
quint8 Cpu::BCSREL(){ this->mrIm(); this->bcs();                   return 2; } // 0xB0
quint8 Cpu::LDAIZY(){ this->mrIy(); this->lda();  return this->checkEa() +5; } // 0xB1
quint8 Cpu::LAXIZY(){ this->mrIy(); this->lax();  return this->checkEa() +5; } // 0xB3
quint8 Cpu::LDYZPX(){ this->mrZx(); this->ldy();                   return 4; } // 0xB4
quint8 Cpu::LDAZPX(){ this->mrZx(); this->lda();                   return 4; } // 0xB5
quint8 Cpu::LDXZPY(){ this->mrZy(); this->ldx();                   return 4; } // 0xB6
quint8 Cpu::LAXZPY(){ this->mrZy(); this->lax();                   return 4; } // 0xB7
quint8 Cpu::CLVREL(){               this->clv();                   return 2; } // 0xB8
quint8 Cpu::LDAABY(){ this->mrAy(); this->lda();  return this->checkEa() +4; } // 0xB9
quint8 Cpu::TSXREL(){               this->tsx();                   return 2; } // 0xBA
quint8 Cpu::LASABY(){ this->mrAy(); this->las();  return this->checkEa() +4; } // 0xBB
quint8 Cpu::LDYABX(){ this->mrAx(); this->ldy();  return this->checkEa() +4; } // 0xBC
quint8 Cpu::LDAABX(){ this->mrAx(); this->lda();  return this->checkEa() +4; } // 0xBD
quint8 Cpu::LDXABY(){ this->mrAy(); this->ldx();  return this->checkEa() +4; } // 0xBE
quint8 Cpu::LAXABY(){ this->mrAy(); this->lax();  return this->checkEa() +4; } // 0xBF
quint8 Cpu::CPYIMM(){ this->mrIm(); this->cpy();                   return 2; } // 0xC0
quint8 Cpu::CMPIZX(){ this->mrIx(); this->cmp();  return this->checkEa() +5; } // 0xC1
quint8 Cpu::DCPIZX(){ this->mrIx(); this->dcp();  this->mwEa();    return 8; } // 0xC3
quint8 Cpu::CPYZP_(){ this->mrZp(); this->cpy();                   return 3; } // 0xC4
quint8 Cpu::CMPZP_(){ this->mrZp(); this->cmp();                   return 3; } // 0xC5
quint8 Cpu::DECZP_(){ this->mrZp(); this->dec();  this->mwZp();    return 5; } // 0xC6
quint8 Cpu::DCPZP_(){ this->mrZp(); this->dcp();  this->mwZp();    return 5; } // 0xC7
quint8 Cpu::INYREL(){               this->iny();                   return 2; } // 0xC8
quint8 Cpu::CMPIMM(){ this->mrIm(); this->cmp();                   return 2; } // 0xC9
quint8 Cpu::DEXREL(){               this->dex();                   return 2; } // 0xCA
quint8 Cpu::SBXIMM(){ this->mrIm(); this->sbx();                   return 2; } // 0xCB
quint8 Cpu::CPYABS(){ this->mrAb(); this->cpy();                   return 4; } // 0xCC
quint8 Cpu::CMPABS(){ this->mrAb(); this->cmp();                   return 4; } // 0xCD
quint8 Cpu::DECABS(){ this->mrAb(); this->dec();  this->mwEa();    return 6; } // 0xCE
quint8 Cpu::DCPABS(){ this->mrAb(); this->dcp();  this->mwEa();    return 6; } // 0xCF
quint8 Cpu::BNEREL(){ this->mrIm(); this->bne();                   return 2; } // 0xD0
quint8 Cpu::CMPIZY(){ this->mrIy(); this->cmp();  return this->checkEa() +5; } // 0xD1
quint8 Cpu::DCPIZY(){ this->mrIy(); this->dcp();  this->mwEa();    return 8; } // 0xD3
quint8 Cpu::CMPZPX(){ this->mrZx(); this->cmp();                   return 4; } // 0xD5
quint8 Cpu::DECZPX(){ this->mrZx(); this->dec();  this->mwZp();    return 6; } // 0xD6
quint8 Cpu::DCPZPX(){ this->mrZx(); this->dcp();  this->mwZp();    return 6; } // 0xD7
quint8 Cpu::CLDREL(){               this->cld();                   return 2; } // 0xD8
quint8 Cpu::CMPABY(){ this->mrAy(); this->cmp();  return this->checkEa() +4; } // 0xD9
quint8 Cpu::DCPABY(){ this->mrAy(); this->dcp();  this->mwEa();    return 7; } // 0xDB
quint8 Cpu::CMPABX(){ this->mrAx(); this->cmp();  return this->checkEa() +4; } // 0xDD
quint8 Cpu::DECABX(){ this->mrAx(); this->dec();  this->mwEa();    return 7; } // 0xDE
quint8 Cpu::DCPABX(){ this->mrAx(); this->dcp();  this->mwEa();    return 7; } // 0xDF
quint8 Cpu::CPXIMM(){ this->mrIm(); this->cpx();                   return 2; } // 0xE0
quint8 Cpu::SBCIZX(){ this->mrIx(); this->sbc();                   return 6; } // 0xE1
quint8 Cpu::ISBIZX(){ this->mrIx(); this->isb();  this->mwEa();    return 5; } // 0xE3
quint8 Cpu::CPXZP_(){ this->mrZp(); this->cpx();                   return 3; } // 0xE4
quint8 Cpu::SBCZP_(){ this->mrZp(); this->sbc();                   return 3; } // 0xE5
quint8 Cpu::INCZP_(){ this->mrZp(); this->inc();  this->mwZp();    return 5; } // 0xE6
quint8 Cpu::ISBZP_(){ this->mrZp(); this->isb();  this->mwZp();    return 5; } // 0xE7
quint8 Cpu::INXREL(){               this->inx();                   return 2; } // 0xE8
quint8 Cpu::SBCIMM(){ this->mrIm(); this->sbc();                   return 2; } // 0xE9, 0xEB
quint8 Cpu::CPXABS(){ this->mrAb(); this->cpx();                   return 4; } // 0xEC
quint8 Cpu::SBCABS(){ this->mrAb(); this->sbc();                   return 4; } // 0xED
quint8 Cpu::INCABS(){ this->mrAb(); this->inc();  this->mwEa();    return 6; } // 0xEE
quint8 Cpu::ISBABS(){ this->mrAb(); this->isb();  this->mwEa();    return 5; } // 0xEF
quint8 Cpu::BEQREL(){ this->mrIm(); this->beq();                   return 2; } // 0xF0
quint8 Cpu::SBCIZY(){ this->mrIy(); this->sbc();  return this->checkEa() +5; } // 0xF1
quint8 Cpu::ISBIZY(){ this->mrIy(); this->isb();  this->mwEa();    return 5; } // 0xF3
quint8 Cpu::SBCZPX(){ this->mrZx(); this->sbc();                   return 4; } // 0xF5
quint8 Cpu::INCZPX(){ this->mrZx(); this->inc();  this->mwZp();    return 6; } // 0xF6
quint8 Cpu::ISBZPX(){ this->mrZx(); this->isb();  this->mwZp();    return 5; } // 0xF7
quint8 Cpu::SEDREL(){               this->sed();                   return 2; } // 0xF8
quint8 Cpu::SBCABY(){ this->mrAy(); this->sbc();  return this->checkEa() +4; } // 0xF9
quint8 Cpu::ISBABY(){ this->mrAy(); this->isb();  this->mwEa();    return 5; } // 0xFB
quint8 Cpu::SBCABX(){ this->mrAx(); this->sbc();  return this->checkEa() +4; } // 0xFD
quint8 Cpu::INCABX(){ this->mrAx(); this->inc();  this->mwEa();    return 7; } // 0xFE
quint8 Cpu::ISBABX(){ this->mrAx(); this->isb();  this->mwEa();    return 5; } // 0xFF
quint8 Cpu::NOPREL(){                                              return 2; } // 0x1A, 0x3A, 0x5A, 0x7A, 0xDA, 0xFA
quint8 Cpu::DOP__2(){ r.pc++;                                      return 2; } // 0x80, 0x82, 0x89, 0xC2, 0xE2
quint8 Cpu::DOP__3(){ r.pc++;                                      return 3; } // 0x04, 0x44, 0x64
quint8 Cpu::DOP__4(){ r.pc++;                                      return 4; } // 0x14, 0x34, 0x54, 0x74, 0xD4, 0xF4
quint8 Cpu::TOPREL(){ r.pc++; r.pc++;                              return 4; } // 0x0C, 0x1C, 0x3C, 0x5C, 0x7C, 0xDC, 0xFC
quint8 Cpu::KILLED(){ this->cpuKilled();                           return 0; } // 0x02, 0x12, 0x22, 0x32, 0x42, 0x52, 0x62, 0x72, 0x92, 0xB2, 0xD2, 0xF2


// Construct Function
// TODO

quint8 Cpu::rdCpu(quint16 addr) {
    if(addr < 0x2000){
        // RAM (Mirror $0800, $1000, $1800)
        return mmu->getRam()[addr & 0x07FF];
    }else if(addr < 0x8000){
        // others
        return this->nes->read(addr);
    }else{
        // dummy access
        this->mapper->read(addr, mmu->getCpuMemBank()[addr >> 13][addr & 0x1FFF]);
    }
    return mmu->getCpuMemBank()[addr >> 13][addr & 0x1FFF];
}

void Cpu::wrCpu(quint16 addr, quint8 data) {
    if(addr < 0x2000){
        // RAM mirror ($0000, $1000, $1800)
        mmu->getRam()[addr & 0x07FF] = data;
    }else{
        // other
        this->nes->write(addr, data);
    }
}

quint16 Cpu::rdCpuW(quint16 addr) {
    if(addr < 0x2000){
        // RAM (Mirror $0800, $1000, $1800)
        return *((quint16*) &mmu->getRam()[addr & 0x07FF]);
    }else if(addr < 0x8000){
        // others
        return (quint16)this->nes->read(addr) + (quint16)this->nes->read(addr + 1) * 0x100;
    }
    // quick bank read
    return *((quint16*) &mmu->getCpuMemBank()[addr >> 13][addr & 0x1FFF]);
}

void Cpu::reset(){
    this->apu = nes->apu;
    this->mapper = nes->mapper;

    this->r.a = 0x00;
    this->r.x = 0x00;
    this->r.y = 0x00;
    this->r.s = 0x00;
    this->r.p = Cpu::ZFlag | Cpu::RFlag;
    this->r.pc = this->rdCpuW(Cpu::ResVector);

    this->r.intPending = 0;

    this->totalCycles = 0;
    this->dmaCycles = 0;

    // stack quick access
    this->stack = &mmu->getRam()[0x100];

    // zero/negative flag
    this->znTable[0] = Cpu::ZFlag;
    for(int i = 1; i < 256; i++){
        this->znTable[i] = (i & 0x80) ? Cpu::NFlag : 0;
    }
}

// interrupt
void Cpu::nmi(){
    this->r.intPending |= Cpu::NmiFlag;
    this->nmiCount = 0;
}

void Cpu::setIrq(quint8 mask) {
    this->r.intPending |= mask;
}

void Cpu::clearIrq(quint8 mask) {
    this->r.intPending &= ~mask;
}

void Cpu::dma(qint32 cycles) {
    this->dmaCycles += cycles;
}

// execution
quint32 Cpu::exec(quint32 requestCycles) {
    quint32 execCycles = 0;
    quint32 oldCycles = this->totalCycles;
    bool clockProcess = this->clockProcess;

    while(requestCycles > 0){
        execCycles = 0;
        if(this->dmaCycles){
            if(requestCycles <= this->dmaCycles){
                this->dmaCycles -= requestCycles;
                this->totalCycles += requestCycles;

                // clock synchronization
                this->mapper->clock(requestCycles);

                if(clockProcess){
                    this->nes->clock(requestCycles);
                }

                // todo: GOTO execute_exit
                break;
            }else{
                execCycles += this->dmaCycles;
                this->dmaCycles = 0;
            }
        }

        quint8 nmi_request = 0;
        quint8 irq_request = 0;
        quint8 opcode = this->opCpu(this->r.pc++);

        if(this->r.intPending){
            if(this->r.intPending & Cpu::NmiFlag){
                nmi_request = 0xFF;
                this->r.intPending &= ~Cpu::NmiFlag;
            }else if(this->r.intPending & Cpu::IrqMask){
                this->r.intPending &= ~Cpu::IrqTrigger2;
                if(!(this->r.p & Cpu::IFlag) && opcode != 0x40){
                    irq_request = 0xFF;
                    this->r.intPending &= ~Cpu::IrqTrigger;
                }
            }
        }
        execCycles += this->operations[opcode]();
        if(nmi_request){
            this->_nmi();
        }else if(irq_request){
            this->_irq();
        }

        requestCycles -= execCycles;
        totalCycles += execCycles;

        // clock sync
        this->mapper->clock(execCycles);
#if DPCM_SYNCCLOCK
        this->apu->syncDpcm(execCycles);
#endif
        if(clockProcess){
            this->nes->clock(execCycles);
        }
    }

#if !DPCM_SYNCCLOCK
    this->apu->syncDpcm(totalCycles - oldCycles);
#endif

    return totalCycles - oldCycles;
}