//
// Copyright (c) 2021 Thomas Skibo.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//

#include <stdint.h>
#include <gtkmm.h>

#include "Cpu6502GtkDebug.h"

#include "Cpu6502.h"

#include <cstdio>

#ifdef DEBUGAPP
#  define DPRINTF(l, arg...) \
	do { if ((l) <= DEBUGAPP) printf(arg); } while (0)
#else
#  define DPRINTF(l, arg...)
#endif

Cpu6502GtkDebug::Cpu6502GtkDebug(Cpu6502 *_cpu)
{
	DPRINTF(1, "Cpu6502GtkDebug: %s\n", __func__);

	cpu = _cpu;
	memspace = cpu->getMemSpace();

	showIllOps = false;
	numi = 0;
}

void
Cpu6502GtkDebug::connectSignals(Glib::RefPtr <Gtk::Builder> builder)
{
	DPRINTF(1, "Cpu6502GtkDebug::%s:\n", __func__);

	builder->get_widget("debug_stop", stopButton);
	stopButton->signal_clicked().connect(sigc::mem_fun(*this,
				&Cpu6502GtkDebug::onDebugStop));
	builder->get_widget("debug_step", stepButton);
	stepButton->signal_clicked().connect(sigc::mem_fun(*this,
				&Cpu6502GtkDebug::onDebugStep));
	builder->get_widget("debug_cont", pauseButton);
	pauseButton->signal_clicked().connect(sigc::mem_fun(*this,
				&Cpu6502GtkDebug::onDebugContinue));
	builder->get_widget("debug_showillops", illOpsButton);
	illOpsButton->signal_toggled().connect(sigc::mem_fun(*this,
				&Cpu6502GtkDebug::onIllOpsToggle));

	builder->get_widget("debug_win", debugWin);
	builder->get_widget("debug_memview", memView);
	builder->get_widget("debug_asmview", asmView);

	builder->get_widget("debug_pc", entryPC);
	builder->get_widget("debug_a", entryA);
	builder->get_widget("debug_x", entryX);
	builder->get_widget("debug_y", entryY);
	builder->get_widget("debug_p", entryP);
	builder->get_widget("debug_sp", entrySP);

	builder->get_widget("debug_memaddr", entryMemAddr);

	builder->get_widget("debug_iaddr", entryInstrAddr);

	builder->get_widget("debug_cycle", entryCycle);

	Gtk::Button *button = nullptr;
	builder->get_widget("debug_memupdate", button);
	button->signal_clicked().connect(sigc::mem_fun(*this,
				&Cpu6502GtkDebug::onUpdateMem));
	button->signal_key_press_event().connect(sigc::mem_fun(*this,
			       &Cpu6502GtkDebug::onKeyEventMem));
	builder->get_widget("debug_iupdate", button);
	button->signal_clicked().connect(sigc::mem_fun(*this,
				&Cpu6502GtkDebug::onUpdateInstr));
	button->signal_key_press_event().connect(sigc::mem_fun(*this,
			       &Cpu6502GtkDebug::onKeyEventInstr));;

	for (int i = 0; i < NBPENTRIES; i++) {
		char buf[16];
		sprintf(buf, "debug_bp%d", i);
		builder->get_widget(buf, entryBPAddr[i]);
	}
}

#ifndef CPU65C02
/* Instruction length by opcode: NMOS 6502 including undocumented opcodes. */
static uint8_t instrlen[] = {
	2, 2, 1, 2, 2, 2, 2, 2,  1, 2, 1, 2, 3, 3, 3, 3,
	2, 2, 1, 2, 2, 2, 2, 2,  1, 3, 1, 3, 3, 3, 3, 3,
	3, 2, 1, 2, 2, 2, 2, 2,  1, 2, 1, 2, 3, 3, 3, 3,
	2, 2, 1, 2, 2, 2, 2, 2,  1, 3, 1, 3, 3, 3, 3, 3,
	1, 2, 1, 2, 2, 2, 2, 2,  1, 2, 1, 2, 3, 3, 3, 3,
	2, 2, 1, 2, 2, 2, 2, 2,  1, 3, 1, 3, 3, 3, 3, 3,
	1, 2, 1, 2, 2, 2, 2, 2,  1, 2, 1, 2, 3, 3, 3, 3,
	2, 2, 1, 2, 2, 2, 2, 2,  1, 3, 1, 3, 3, 3, 3, 3,

	2, 2, 2, 2, 2, 2, 2, 2,  1, 2, 1, 2, 3, 3, 3, 3,
	2, 2, 1, 2, 2, 2, 2, 2,  1, 3, 1, 3, 3, 3, 3, 3,
	2, 2, 2, 2, 2, 2, 2, 2,  1, 2, 1, 2, 3, 3, 3, 3,
	2, 2, 1, 2, 2, 2, 2, 2,  1, 3, 1, 3, 3, 3, 3, 3,
	2, 2, 2, 2, 2, 2, 2, 2,  1, 2, 1, 2, 3, 3, 3, 3,
	2, 2, 1, 2, 2, 2, 2, 2,  1, 3, 1, 3, 3, 3, 3, 3,
	2, 2, 2, 2, 2, 2, 2, 2,  1, 2, 1, 2, 3, 3, 3, 3,
	2, 2, 1, 2, 2, 2, 2, 2,  1, 3, 1, 3, 3, 3, 3, 3
};
#else
/* Instruction length by opcode: WDC 65C02. */
static uint8_t instrlen[] = {
	2, 2, 2, 1, 2, 2, 2, 1,  1, 2, 1, 1, 3, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 3, 1, 1, 3, 3, 3, 1,
	3, 2, 2, 1, 2, 2, 2, 1,  1, 2, 1, 1, 3, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 3, 1, 1, 3, 3, 3, 1,
	1, 2, 2, 1, 2, 2, 2, 1,  1, 2, 1, 1, 3, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 3, 1, 1, 1, 3, 3, 1,
	1, 2, 2, 1, 2, 2, 2, 1,  1, 2, 1, 1, 3, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 3, 1, 1, 3, 3, 3, 1,

	2, 2, 2, 1, 2, 2, 2, 1,  1, 2, 1, 1, 3, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 3, 1, 1, 3, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 2, 1, 1, 3, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 3, 1, 1, 3, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 2, 1, 1, 3, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 3, 1, 1, 1, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 2, 1, 1, 3, 3, 3, 1,
	2, 2, 2, 1, 2, 2, 2, 1,  1, 3, 1, 1, 1, 3, 3, 1
};
#endif // CPU65C02

/**
 * rel_addr() - compute destination address of a relative branch instruction.
 *
 * @op: one-byte operand of branch instruction.
 * @addr: address of branch instruction.
 *
 * Return: 16-bit destination address of branch instruction.
 */
static uint16_t
rel_addr(uint8_t op, uint16_t addr)
{
	if (op > 0x7f)
		return addr - 0xfe + op;
	else
		return addr + 2 + op;
}

/**
 * disinstr() - disassemble a single 6502 instruction.
 *
 * @instrp: points to instruction in memory.
 * @addr: points to 16-bit address of instruction in memory.
 * @line: destination buffer of C-string of disassembled instruction.  Must
 *        be 12 bytes long.
 *
 * Return: returns the number of characters put in line.
 */
int
Cpu6502GtkDebug::disinstr(uint8_t *instrp, uint16_t addr, char *line)
{
	uint8_t opcode = instrp[0];
	uint16_t operand = 0;
	int ilen = instrlen[opcode];
	int len = 0;

	if (ilen == 2)
		operand = instrp[1];
	else if (ilen == 3)
		operand = instrp[1] + ((uint16_t)instrp[2] << 8);

	switch (opcode) {
	case 0x00:        /* BRK */
		len = sprintf(line, "BRK $%02X", operand);
		break;
	case 0x01:        /* ORA (ind,X) */
		len = sprintf(line, "ORA ($%02X,X)", operand);
		break;
	case 0x05:        /* ORA zero page */
		len = sprintf(line, "ORA $%02X", operand);
		break;
	case 0x06:        /* ASL zero page */
		len = sprintf(line, "ASL $%02X", operand);
		break;
	case 0x08:        /* PHP */
		len = sprintf(line, "PHP");
		break;
	case 0x09:        /* ORA imm */
		len = sprintf(line, "ORA #$%02X", operand);
		break;
	case 0x0a:        /* ASL A */
		len = sprintf(line, "ASL A");
		break;
	case 0x0d:        /* ORA absolute */
		len = sprintf(line, "ORA $%04X", operand);
		break;
	case 0x0e:        /* ASL absolute */
		len = sprintf(line, "ASL $%04X", operand);
		break;
	case 0x10:        /* BPL */
		len = sprintf(line, "BPL $%04X", rel_addr(operand, addr));
		break;
	case 0x11:        /* ORA (ind),Y */
		len = sprintf(line, "ORA ($%02X),Y", operand);
		break;
	case 0x15:        /* ORA zero,X */
		len = sprintf(line, "ORA $%02X,X", operand);
		break;
	case 0x16:        /* ASL zero,X */
		len = sprintf(line, "ASL $%02X,X", operand);
		break;
	case 0x18:        /* CLC */
		len = sprintf(line, "CLC");
		break;
	case 0x19:        /* ORA absolute,Y */
		len = sprintf(line, "ORA $%04X,Y", operand);
		break;
	case 0x1d:        /* ORA absolute,X */
		len = sprintf(line, "ORA $%04X", operand);
		break;
	case 0x1e:        /* ASL absolute,X */
		len = sprintf(line, "ASL $%04X,X", operand);
		break;
	case 0x20:        /* JSR absolute */
		len = sprintf(line, "JSR $%04X", operand);
		break;
	case 0x21:        /* AND (ind,X) */
		len = sprintf(line, "AND ($%02X,X)", operand);
		break;
	case 0x24:        /* BIT zero page */
		len = sprintf(line, "BIT $%02X", operand);
		break;
	case 0x25:        /* AND zero page */
		len = sprintf(line, "AND $%02X", operand);
		break;
	case 0x26:        /* ROL zero page */
		len = sprintf(line, "ROL $%02X", operand);
		break;
	case 0x28:        /* PLP */
		len = sprintf(line, "PLP");
		break;
	case 0x29:        /* AND imm */
		len = sprintf(line, "AND #$%02X", operand);
		break;
	case 0x2a:        /* ROL A */
		len = sprintf(line, "ROL A");
		break;
	case 0x2c:        /* BIT absolute */
		len = sprintf(line, "BIT $%04X", operand);
		break;
	case 0x2d:        /* AND absolute */
		len = sprintf(line, "AND $%04X", operand);
		break;
	case 0x2e:        /* ROL absolute */
		len = sprintf(line, "ROL $%04X", operand);
		break;
	case 0x30:        /* BMI */
		len = sprintf(line, "BMI $%04X", rel_addr(operand, addr));
		break;
	case 0x31:        /* AND (ind),Y */
		len = sprintf(line, "AND ($%02X),Y", operand);
		break;
	case 0x35:        /* AND zero, X */
		len = sprintf(line, "AND $%02X,X", operand);
		break;
	case 0x36:        /* ROL zero, X */
		len = sprintf(line, "ROL $%02X,X", operand);
		break;
	case 0x38:        /* SEC */
		len = sprintf(line, "SEC");
		break;
	case 0x39:        /* AND absolute, Y */
		len = sprintf(line, "AND $%04X,Y", operand);
		break;
	case 0x3d:        /* AND absolute, X */
		len = sprintf(line, "AND $%04X,X", operand);
		break;
	case 0x3e:        /* ROL absolute, X */
		len = sprintf(line, "ROL $%04X,X", operand);
		break;
	case 0x40:        /* RTI */
		len = sprintf(line, "RTI");
		break;
	case 0x41:        /* EOR (ind, X) */
		len = sprintf(line, "EOR ($%02X,X)", operand);
		break;
	case 0x45:        /* EOR zero page */
		len = sprintf(line, "EOR $%02X", operand);
		break;
	case 0x46:        /* LSR zero page */
		len = sprintf(line, "LSR $%02X", operand);
		break;
	case 0x48:        /* PHA */
		len = sprintf(line, "PHA");
		break;
	case 0x49:        /* EOR imm */
		len = sprintf(line, "EOR #$%02X", operand);
		break;
	case 0x4a:        /* LSR A */
		len = sprintf(line, "LSR A");
		break;
	case 0x4c:        /* JMP absolute */
		len = sprintf(line, "JMP $%04X", operand);
		break;
	case 0x4d:        /* EOR absolute */
		len = sprintf(line, "EOR $%04X", operand);
		break;
	case 0x4e:        /* LSR absolute */
		len = sprintf(line, "LSR $%04X", operand);
		break;
	case 0x50:        /* BVC */
		len = sprintf(line, "BVC $%04X", rel_addr(operand, addr));
		break;
	case 0x51:        /* EOR (indirect), Y */
		len = sprintf(line, "EOR ($%02X),Y", operand);
		break;
	case 0x55:        /* EOR zero, X */
		len = sprintf(line, "EOR $%02X,X", operand);
		break;
	case 0x56:        /* LSR zero, X */
		len = sprintf(line, "LSR $%02X,X", operand);
		break;
	case 0x58:        /* CLI */
		len = sprintf(line, "CLI");
		break;
	case 0x59:        /* EOR absolute,Y */
		len = sprintf(line, "EOR $%04X,Y", operand);
		break;
	case 0x5d:        /* EOR absolute,X */
		len = sprintf(line, "EOR $%04X,X", operand);
		break;
	case 0x5e:        /* LSR absolute,X */
		len = sprintf(line, "LSR $%04X,X", operand);
		break;
	case 0x60:        /* RTS */
		len = sprintf(line, "RTS");
		break;
	case 0x61:        /* ADC (ind,X) */
		len = sprintf(line, "ADC ($%02X,X)", operand);
		break;
	case 0x65:        /* ADC zero page */
		len = sprintf(line, "ADC $%02X", operand);
		break;
	case 0x66:        /* ROR zero page */
		len = sprintf(line, "ROR $%02X", operand);
		break;
	case 0x68:        /* PLA */
		len = sprintf(line, "PLA");
		break;
	case 0x69:        /* ADC imm */
		len = sprintf(line, "ADC #$%02X", operand);
		break;
	case 0x6a:        /* ROR A */
		len = sprintf(line, "ROR A");
		break;
	case 0x6c:        /* JMP (ind) */
		len = sprintf(line, "JMP ($%04X)", operand);
		break;
	case 0x6d:        /* ADC absolute */
		len = sprintf(line, "ADC $%04X", operand);
		break;
	case 0x6e:        /* ROR absolute */
		len = sprintf(line, "ROR $%04X", operand);
		break;
	case 0x70:        /* BVS */
		len = sprintf(line, "BVS $%04X", rel_addr(operand, addr));
		break;
	case 0x71:        /* ADC (indirect),Y */
		len = sprintf(line, "ADC ($%02X),Y", operand);
		break;
	case 0x75:        /* ADC zero, X */
		len = sprintf(line, "ADC $%02X,X", operand);
		break;
	case 0x76:        /* ROR zero, X */
		len = sprintf(line, "ROR $%02X,X", operand);
		break;
	case 0x78:        /* SEI */
		len = sprintf(line, "SEI");
		break;
	case 0x79:        /* ADC absolute,Y */
		len = sprintf(line, "ADC $%04X,Y", operand);
		break;
	case 0x7d:        /* ADC absolute,X */
		len = sprintf(line, "ADC $%04X,X", operand);
		break;
	case 0x7e:        /* ROR absolute,X */
		len = sprintf(line, "ROR $%04X,X", operand);
		break;
	case 0x81:        /* STA (ind, X) */
		len = sprintf(line, "STA ($%02X,X)", operand);
		break;
	case 0x84:        /* STY zero page */
		len = sprintf(line, "STY $%02X", operand);
		break;
	case 0x85:        /* STA zero page */
		len = sprintf(line, "STA $%02X", operand);
		break;
	case 0x86:        /* STX zero page */
		len = sprintf(line, "STX $%02X", operand);
		break;
	case 0x88:        /* DEY */
		len = sprintf(line, "DEY");
		break;
	case 0x8a:        /* TXA */
		len = sprintf(line, "TXA");
		break;
	case 0x8c:        /* STY absolute */
		len = sprintf(line, "STY $%04X", operand);
		break;
	case 0x8d:        /* STA absolute */
		len = sprintf(line, "STA $%04X", operand);
		break;
	case 0x8e:        /* STX absolute */
		len = sprintf(line, "STX $%04X", operand);
		break;
	case 0x90:        /* BCC */
		len = sprintf(line, "BCC $%04X", rel_addr(operand, addr));
		break;
	case 0x91:        /* STA (ind),Y */
		len = sprintf(line, "STA ($%02X),Y", operand);
		break;
	case 0x94:        /* STY zero, X */
		len = sprintf(line, "STY $%02X,X", operand);
		break;
	case 0x95:        /* STA zero, X */
		len = sprintf(line, "STA $%02X,X", operand);
		break;
	case 0x96:        /* STX zero, Y */
		len = sprintf(line, "STX $%02X,Y", operand);
		break;
	case 0x98:        /* TYA */
		len = sprintf(line, "TYA");
		break;
	case 0x99:        /* STA absolute, Y */
		len = sprintf(line, "STA $%04X,Y", operand);
		break;
	case 0x9a:        /* TXS */
		len = sprintf(line, "TXS");
		break;
	case 0x9d:        /* STA absolute, X */
		len = sprintf(line, "STA $%04X,X", operand);
		break;
	case 0xa0:        /* LDY imm */
		len = sprintf(line, "LDY #$%02X", operand);
		break;
	case 0xa1:        /* LDA (ind, X) */
		len = sprintf(line, "LDA ($%02X,X)", operand);
		break;
	case 0xa2:        /* LDX imm */
		len = sprintf(line, "LDX #$%02X", operand);
		break;
	case 0xa4:        /* LDY zero page */
		len = sprintf(line, "LDY $%02X", operand);
		break;
	case 0xa5:        /* LDA zero page */
		len = sprintf(line, "LDA $%02X", operand);
		break;
	case 0xa6:        /* LDX zero page */
		len = sprintf(line, "LDX $%02X", operand);
		break;
	case 0xa8:        /* TAY */
		len = sprintf(line, "TAY");
		break;
	case 0xa9:        /* LDA imm */
		len = sprintf(line, "LDA #$%02X", operand);
		break;
	case 0xaa:        /* TAX */
		len = sprintf(line, "TAX");
		break;
	case 0xac:        /* LDY absolute */
		len = sprintf(line, "LDY $%04X", operand);
		break;
	case 0xad:        /* LDA absolute */
		len = sprintf(line, "LDA $%04X", operand);
		break;
	case 0xae:        /* LDX absolute */
		len = sprintf(line, "LDX $%04X", operand);
		break;
	case 0xb0:        /* BCS */
		len = sprintf(line, "BCS $%04X", rel_addr(operand, addr));
		break;
	case 0xb1:        /* LDA (ind),Y */
		len = sprintf(line, "LDA ($%02X),Y", operand);
		break;
	case 0xb4:        /* LDY zero,X */
		len = sprintf(line, "LDY $%02X,X", operand);
		break;
	case 0xb5:        /* LDA zero,X */
		len = sprintf(line, "LDA $%02X,X", operand);
		break;
	case 0xb6:        /* LDX zero,Y */
		len = sprintf(line, "LDX $%02X,Y", operand);
		break;
	case 0xb8:        /* CLV */
		len = sprintf(line, "CLV");
		break;
	case 0xb9:        /* LDA absolute, Y */
		len = sprintf(line, "LDA $%04X,Y", operand);
		break;
	case 0xba:        /* TSX */
		len = sprintf(line, "TSX");
		break;
	case 0xbc:        /* LDY absolute, X */
		len = sprintf(line, "LDY $%04X,X", operand);
		break;
	case 0xbd:        /* LDA absolute, X */
		len = sprintf(line, "LDA $%04X,X", operand);
		break;
	case 0xbe:        /* LDX absolute, Y */
		len = sprintf(line, "LDX $%04X,Y", operand);
		break;
	case 0xc0:        /* CPY imm */
		len = sprintf(line, "CPY #$%02X", operand);
		break;
	case 0xc1:        /* CMP (ind, X) */
		len = sprintf(line, "CMP ($%02X,X)", operand);
		break;
	case 0xc4:        /* CPY zero */
		len = sprintf(line, "CPY $%02X", operand);
		break;
	case 0xc5:        /* CMP zero */
		len = sprintf(line, "CMP $%02X", operand);
		break;
	case 0xc6:        /* DEC zero */
		len = sprintf(line, "DEC $%02X", operand);
		break;
	case 0xc8:        /* INY */
		len = sprintf(line, "INY");
		break;
	case 0xc9:        /* CMP imm */
		len = sprintf(line, "CMP #$%02X", operand);
		break;
	case 0xca:        /* DEX */
		len = sprintf(line, "DEX");
		break;
	case 0xcc:        /* CPY absolute */
		len = sprintf(line, "CPY $%04X", operand);
		break;
	case 0xcd:        /* CMP absolute */
		len = sprintf(line, "CMP $%04X", operand);
		break;
	case 0xce:        /* DEC absolute */
		len = sprintf(line, "DEC $%04X", operand);
		break;
	case 0xd0:        /* BNE */
		len = sprintf(line, "BNE $%04X", rel_addr(operand, addr));
		break;
	case 0xd1:        /* CMP (ind),Y */
		len = sprintf(line, "CMP ($%02X),Y", operand);
		break;
	case 0xd5:        /* CMP zero,X */
		len = sprintf(line, "CMP $%02X,X", operand);
		break;
	case 0xd6:        /* DEC zero,X */
		len = sprintf(line, "DEC $%02X,X", operand);
		break;
	case 0xd8:        /* CLD */
		len = sprintf(line, "CLD");
		break;
	case 0xd9:        /* CMP absolute,Y */
		len = sprintf(line, "CMP $%04X,Y", operand);
		break;
	case 0xdd:        /* CMP absolute,X */
		len = sprintf(line, "CMP $%04X,X", operand);
		break;
	case 0xde:        /* DEC absolute,X */
		len = sprintf(line, "DEC $%04X,X", operand);
		break;
	case 0xe0:        /* CPX imm */
		len = sprintf(line, "CPX #$%02X", operand);
		break;
	case 0xe1:        /* SBC (ind,X) */
		len = sprintf(line, "SBC ($%02X,X)", operand);
		break;
	case 0xe4:        /* CPX zero */
		len = sprintf(line, "CPX $%02X", operand);
		break;
	case 0xe5:        /* SBC zero */
		len = sprintf(line, "SBC $%02X", operand);
		break;
	case 0xe6:        /* INC zero */
		len = sprintf(line, "INC $%02X", operand);
		break;
	case 0xe8:        /* INX */
		len = sprintf(line, "INX");
		break;
	case 0xe9:        /* SBC imm */
		len = sprintf(line, "SBC #$%02X", operand);
		break;
	case 0xea:        /* NOP */
		len = sprintf(line, "NOP");
		break;
	case 0xec:        /* CPX absolute */
		len = sprintf(line, "CPX $%04X", operand);
		break;
	case 0xed:        /* SBC absolute */
		len = sprintf(line, "SBC $%04X", operand);
		break;
	case 0xee:        /* INC absolute */
		len = sprintf(line, "INC $%04X", operand);
		break;
	case 0xf0:        /* BEQ */
		len = sprintf(line, "BEQ $%04X", rel_addr(operand, addr));
		break;
	case 0xf1:        /* SBC (ind),Y */
		len = sprintf(line, "SBC ($%02X),Y", operand);
		break;
	case 0xf5:        /* SBC zero,X */
		len = sprintf(line, "SBC $%02X,X", operand);
		break;
	case 0xf6:        /* INC zero,X */
		len = sprintf(line, "INC $%02X,X", operand);
		break;
	case 0xf8:        /* SED */
		len = sprintf(line, "SED");
		break;
	case 0xf9:        /* SBC absolute,Y */
		len = sprintf(line, "SBC $%04X,Y", operand);
		break;
	case 0xfd:        /* SBC absolute,X */
		len = sprintf(line, "SBC $%04X,X", operand);
		break;
	case 0xfe:        /* INC absolute,X */
		len = sprintf(line, "INC $%04X,X", operand);
		break;

#ifdef CPU65C02
	case 0x04:        /* TSB zero page */
		len = sprintf(line, "TSB $%02X", operand);
		break;
	case 0x07:        /* RMB0 zero page */
		len = sprintf(line, "RMB0 $%02X", operand);
		break;
	case 0xff:        /* BBS7 zero */
		len = sprintf(line, "BBS7 $%02X", operand);
		break;
	case 0x0c:        /* TSB absolute */
		len = sprintf(line, "TSB $%04X", operand);
		break;
	case 0x0f:        /* BBR0 zero */
		len = sprintf(line, "BBR0 $%02X", operand);
		break;
	case 0x12:        /* ORA (ind) */
		len = sprintf(line, "ORA ($%02X)", operand);
		break;
	case 0x14:        /* TRB zero page */
		len = sprintf(line, "TRB $%02X", operand);
		break;
	case 0x17:        /* RMB1 zero page */
		len = sprintf(line, "RMB1 $%02X", operand);
		break;
	case 0x1a:        /* INC A */
		len = sprintf(line, "INC A");
		break;
	case 0x1c:        /* TRB absolute */
		len = sprintf(line, "TRB $%04X", operand);
		break;
	case 0x1f:        /* BBR1 zero */
		len = sprintf(line, "BBR1 $%02X", operand);
		break;
	case 0x27:        /* RMB2 zero page */
		len = sprintf(line, "RMB2 $%02X", operand);
		break;
	case 0x2f:        /* BBR2 zero */
		len = sprintf(line, "BBR $%02X", operand);
		break;
	case 0x32:        /* AND (ind) */
		len = sprintf(line, "AND ($%02X)", operand);
		break;
	case 0x34:        /* BIT zero, X */
		len = sprintf(line, "BIT $%02X,X", operand);
		break;
	case 0x37:        /* RMB3 zero page */
		len = sprintf(line, "RMB3 $%02X", operand);
		break;
	case 0x3a:        /* DEC A */
		len = sprintf(line, "DEC A");
		break;
	case 0x3c:        /* BIT absolute, X */
		len = sprintf(line, "BIT $%04X,X", operand);
		break;
	case 0x3f:        /* BBR3 zero */
		len = sprintf(line, "BBR3 $%02X", operand);
		break;
	case 0x47:        /* RMB4 zero page */
		len = sprintf(line, "RMB4 $%02X", operand);
		break;
	case 0x4f:        /* BBR4 zero */
		len = sprintf(line, "BBR4 $%02X", operand);
		break;
	case 0x52:        /* EOR (ind) */
		len = sprintf(line, "EOR ($%02X)", operand);
		break;
	case 0x57:        /* RMB5 zero page */
		len = sprintf(line, "RMB5 $%02X", operand);
		break;
	case 0x5a:        /* PHY */
		len = sprintf(line, "PHY");
		break;
	case 0x5f:        /* BBR5 zero */
		len = sprintf(line, "BBR5 $%02X", operand);
		break;
	case 0x64:        /* STZ zero page */
		len = sprintf(line, "STZ $%02X", operand);
		break;
	case 0x67:        /* RMB6 zero page */
		len = sprintf(line, "RMB6 $%02X", operand);
		break;
	case 0x6f:        /* BBR6 zero */
		len = sprintf(line, "BBR6 $%02X", operand);
		break;
	case 0x72:        /* ADC (ind) */
		len = sprintf(line, "ADC ($%02X)", operand);
		break;
	case 0x74:        /* STZ zero, X */
		len = sprintf(line, "STZ $%02X,X", operand);
		break;
	case 0x77:        /* RMB7 zero page */
		len = sprintf(line, "RMB7 $%02X", operand);
		break;
	case 0x7a:        /* PLY */
		len = sprintf(line, "PLY");
		break;
	case 0x7c:        /* JMP (abs,X) */
		len = sprintf(line, "JMP ($%04X,X)", operand);
		break;
	case 0x7f:        /* BBR7 zero */
		len = sprintf(line, "BBR7 $%02X", operand);
		break;
	case 0x80:        /* BRA (always) */
		len = sprintf(line, "BRA $%04X", rel_addr(operand, addr));
		break;
	case 0x87:        /* SMB0 zero page */
		len = sprintf(line, "SMB0 $%02X", operand);
		break;
	case 0x89:        /* BIT imm */
		len = sprintf(line, "BIT #$%02X", operand);
		break;
	case 0x8f:        /* BBS0 zero */
		len = sprintf(line, "BBS0 $%02X", operand);
		break;
	case 0x92:        /* STA (ind) */
		len = sprintf(line, "STA ($%02X)", operand);
		break;
	case 0x97:        /* SMB1 zero page */
		len = sprintf(line, "SMB1 $%02X", operand);
		break;
	case 0x9c:        /* STZ absolute */
		len = sprintf(line, "STZ $%04X", operand);
		break;
	case 0x9e:        /* STZ absolute, X */
		len = sprintf(line, "STZ $%04X,X", operand);
		break;
	case 0x9f:        /* BBS1 zero */
		len = sprintf(line, "BBS1 $%02X", operand);
		break;
	case 0xa7:        /* SMB2 zero page */
		len = sprintf(line, "SMB2 $%02X", operand);
		break;
	case 0xaf:        /* BBS2 zero */
		len = sprintf(line, "BBS2 $%02X", operand);
		break;
	case 0xb2:        /* LDA (ind) */
		len = sprintf(line, "LDA ($%02X)", operand);
		break;
	case 0xb7:        /* SMB3 zero page */
		len = sprintf(line, "SMB3 $%02X", operand);
		break;
	case 0xbf:        /* BBS3 zero */
		len = sprintf(line, "BBS3 $%02X", operand);
		break;
	case 0xc7:        /* SMB4 zero page */
		len = sprintf(line, "SMB4 $%02X", operand);
		break;
	case 0xcf:        /* BBS4 zero */
		len = sprintf(line, "BBS4 $%02X", operand);
		break;
	case 0xd2:        /* CMP (ind) */
		len = sprintf(line, "CMP ($%02X)", operand);
		break;
	case 0xd7:        /* SMB5 zero page */
		len = sprintf(line, "SMB5 $%02X", operand);
		break;
	case 0xda:        /* PHX */
		len = sprintf(line, "PHX");
		break;
	case 0xdf:        /* BBS5 zero */
		len = sprintf(line, "BBS5 $%02X", operand);
		break;
	case 0xe7:        /* SMB6 zero page */
		len = sprintf(line, "SMB6 $%02X", operand);
		break;
	case 0xef:        /* BBS6 zero */
		len = sprintf(line, "BBS6 $%02X", operand);
		break;
	case 0xf2:        /* SBC (ind) */
		len = sprintf(line, "SBC ($%02X)", operand);
		break;
	case 0xf7:        /* SMB7 zero page */
		len = sprintf(line, "SMB7 $%02X", operand);
		break;
	case 0xfa:        /* PLX */
		len = sprintf(line, "PLX");
		break;

	default:
		if (ilen > 1)
			len = sprintf(line, "NOPx #$%02X", operand);
		else
			len = sprintf(line, "NOPx");
		break;
#endif // CPU65C02
	}

#ifndef CPU65C02
	if (len == 0) {
		if (showIllOps) {
			switch (opcode) {
			case 0x02:
			case 0x12:
			case 0x22:
			case 0x32:
			case 0x42:
			case 0x52:
			case 0x62:
			case 0x72:
				len = sprintf(line, "JAM");
				break;
			case 0x1a:
			case 0x3a:
			case 0x5a:
			case 0x7a:
			case 0xda:
			case 0xfa:
				len = sprintf(line, "NOPx");
				break;
			case 0x03:
				len = sprintf(line, "SLO ($%02X,X)", operand);
				break;
			case 0x04:
			case 0x44:
			case 0x64:
				len = sprintf(line, "NOPx $%02X", operand);
				break;
			case 0x07:
				len = sprintf(line, "SLO $%02X", operand);
				break;
			case 0x0b:
				len = sprintf(line, "ANC #$%02X", operand);
				break;
			case 0x0c:
				len = sprintf(line, "NOPx $%04X", operand);
				break;
			case 0x0f:
				len = sprintf(line, "SLO $%04X", operand);
				break;
			case 0x13:
				len = sprintf(line, "SLO ($%02X),Y", operand);
				break;
			case 0x14:
			case 0x34:
			case 0x54:
			case 0x74:
			case 0xd4:
			case 0xf4:
				len = sprintf(line, "NOPx $%02X,X", operand);
				break;
			case 0x17:
				len = sprintf(line, "SLO $%02X,X", operand);
				break;
			case 0x1b:
				len = sprintf(line, "SLO $%04X,Y", operand);
				break;
			case 0x1c:
			case 0x3c:
			case 0x5c:
			case 0x7c:
			case 0xdc:
			case 0xfc:
				len = sprintf(line, "NOPx $%04X,X", operand);
				break;
			case 0x1f:
				len = sprintf(line, "SLO $%04X,X", operand);
				break;
			case 0x23:
				len = sprintf(line, "RLA ($%02X,X)", operand);
				break;
			case 0x27:
				len = sprintf(line, "RLA #$%02X", operand);
				break;
			case 0x2b:
				len = sprintf(line, "ANC #$%02X", operand);
				break;
			case 0x2f:
				len = sprintf(line, "RLA $%04X", operand);
				break;
			case 0x33:
				len = sprintf(line, "RLA ($%02X),Y", operand);
				break;
			case 0x37:
				len = sprintf(line, "RLA $%02X,X", operand);
				break;
			case 0x3f:
				len = sprintf(line, "RLA $%04X,X", operand);
				break;
			case 0x43:
				len = sprintf(line, "SRE ($%02X,X)", operand);
				break;
			case 0x47:
				len = sprintf(line, "SRE $%02X", operand);
				break;
			case 0x4b:
				len = sprintf(line, "ASR #$%02X", operand);
				break;
			case 0x4f:
				len = sprintf(line, "SRA $%04X", operand);
				break;
			case 0x53:
				len = sprintf(line, "($%02X),Y", operand);
				break;
			case 0x57:
				len = sprintf(line, "SRE $%02X,X", operand);
				break;
			case 0x5b:
				len = sprintf(line, "SRE $%04X,Y", operand);
				break;
			case 0x5f:
				len = sprintf(line, "SRE $%04X,X", operand);
				break;
			case 0x63:
				len = sprintf(line, "RRA ($%02X,X)", operand);
				break;
			case 0x67:
				len = sprintf(line, "RRA $%02X", operand);
				break;
			case 0x6b:
				len = sprintf(line, "ARR #$%02X", operand);
				break;
			case 0x6f:
				len = sprintf(line, "RRA $%04X", operand);
				break;
			case 0x73:
				len = sprintf(line, "RRA ($%02X),Y", operand);
				break;
			case 0x7b:
				len = sprintf(line, "RRa $%04X,Y", operand);
				break;
			case 0x80:
			case 0x82:
			case 0x89:
			case 0xc2:
			case 0xe2:
				len = sprintf(line, "NOPx #$%02X", operand);
				break;
			case 0x83:
				len = sprintf(line, "SAX ($%02X,X)", operand);
				break;
			case 0x87:
				len = sprintf(line, "SAX $%02X", operand);
				break;
			case 0x8b:
				len = sprintf(line, "XAA #$%02X", operand);
				break;
			case 0x8f:
				len = sprintf(line, "SAX $%04X", operand);
				break;
			case 0x93:
				len = sprintf(line, "SHA ($%02X),Y", operand);
				break;
			case 0x97:
				len = sprintf(line, "SAX $%02X,Y", operand);
				break;
			case 0x9b:
				len = sprintf(line, "SHS $%04X,Y", operand);
				break;
			case 0x9c:
				len = sprintf(line, "SHY $%04X,X", operand);
				break;
			case 0x9e:
				len = sprintf(line, "SHX $%04X,Y", operand);
				break;
			case 0x9f:
				len = sprintf(line, "SHA $%04X,Y", operand);
				break;
			case 0xa3:
				len = sprintf(line, "LAX ($%02X,X)", operand);
				break;
			case 0xa7:
				len = sprintf(line, "LAX $%02X", operand);
				break;
			case 0xab:
				len = sprintf(line, "LAX #$%02X", operand);
				break;
			case 0xaf:
				len = sprintf(line, "LAX $%04X", operand);
				break;
			case 0xb3:
				len = sprintf(line, "LAX ($%02X),Y", operand);
				break;
			case 0xb7:
				len = sprintf(line, "LAX $%02X,Y", operand);
				break;
			case 0xbb:
				len = sprintf(line, "LAS $%04X,Y", operand);
				break;
			case 0xbf:
				len = sprintf(line, "LAX #$%04X,Y", operand);
				break;
			case 0xc3:
				len = sprintf(line, "DCP ($%02X,X)", operand);
				break;
			case 0xc7:
				len = sprintf(line, "DCP $%02X", operand);
				break;
			case 0xcb:
				len = sprintf(line, "SBX #$%02X", operand);
				break;
			case 0xcf:
				len = sprintf(line, "DCP $%04X", operand);
				break;
			case 0xe3:
				len = sprintf(line, "ISC ($%02X,X)", operand);
				break;
			case 0xe7:
				len = sprintf(line, "ISC $%02X", operand);
				break;
			case 0xeb:
				len = sprintf(line, "SBCx #$%02X", operand);
				break;
			case 0xef:
				len = sprintf(line, "ISC $%04X", operand);
				break;
			case 0xf3:
				len = sprintf(line, "ISC ($%02X),Y", operand);
				break;
			case 0xf7:
				len = sprintf(line, "ISC $%02X,X", operand);
				break;
			case 0xfb:
				len = sprintf(line, "ISC $%04X,Y", operand);
				break;
			case 0xff:
				len = sprintf(line, "ISC $%04X,X", operand);
				break;
			}
		}

		if (len == 0)
			len = sprintf(line, "???");
	}
#endif // !CPU65C02

	return len;
}

// Stop button pressed
void
Cpu6502GtkDebug::onDebugStop(void)
{
	DPRINTF(1, "Cpu6502GtkDebug::%s:\n", __func__);

	if (debug_cb)
		debug_cb(CALLBACK_STOP);
}

// Step button pressed
void
Cpu6502GtkDebug::onDebugStep(void)
{
	DPRINTF(1, "Cpu6502GtkDebug::%s:\n", __func__);

	if (debug_cb)
		debug_cb(CALLBACK_STEP);

	updateDebugger();
	updateMemDisp();
}

// Continue button pressed
void
Cpu6502GtkDebug::onDebugContinue(void)
{
	DPRINTF(1, "Cpu6502GtkDebug::%s:\n", __func__);

	int nbps = 0;
	for (int i = 0; i < NBPENTRIES; i++) {
		uint16_t addr = strtoul(entryBPAddr[i]->get_text().c_str(),
					NULL, 16);
		DPRINTF(1, "Cput6502GtkDebug::%s: i=%d BP: 0x%04x\n", __func__,
			i, addr);
		if (addr != 0)
			cpu->setBreak(nbps++, addr);
	}

	cpu->setNumBreaks(nbps);

	if (debug_cb)
		debug_cb(CALLBACK_CONT);
}

// Called by app to change state of debug window.
void
Cpu6502GtkDebug::setState(bool visible, bool _running)
{
	DPRINTF(1, "Cpu6502GtkDebug::%s: visible=%d running=%d\n", __func__,
		visible, running);

	if (visible) {
		debugWin->set_visible(true);
		this->running = _running;

		updateDebugger();
		updateMemDisp();
	} else
		debugWin->set_visible(false);
}

// Display button for memory window is pressed
void
Cpu6502GtkDebug::onUpdateMem(void)
{
	memaddr = strtoul(entryMemAddr->get_text().c_str(), NULL, 16);

	DPRINTF(1, "Cpu6502GtkDebug::%s: memaddr=0x%04x\n", __func__, memaddr);

	updateMemDisp();
}

// Display button for instruction window is pressed
void
Cpu6502GtkDebug::onUpdateInstr(void)
{
	uint16_t addr = strtoul(entryInstrAddr->get_text().c_str(), NULL, 16);

	DPRINTF(1, "Cpu6502GtkDebug::%s: addr=0x%04x\n", __func__, addr);

	updateInstrDisp(addr, cpu->getPc(), true);
}

// Toggle the illegal ops button
void
Cpu6502GtkDebug::onIllOpsToggle(void)
{
	bool state = illOpsButton->get_active();
	uint16_t addr = strtoul(entryInstrAddr->get_text().c_str(), NULL, 16);

	DPRINTF(1, "Cpu6502GtkDebug::%s: state=%d\n", __func__, state);

	showIllOps = state;
	updateInstrDisp(addr, cpu->getPc(), true);
}

// A Key event when memory window Display button is the focus. This allows
// page up and page down key presses to affect display.
bool
Cpu6502GtkDebug::onKeyEventMem(GdkEventKey *event)
{
	int keyval = event->keyval;

	if (event->type != GDK_KEY_PRESS)
		return false;

	DPRINTF(1, "Cpu6502GtkDebug: %s: keyval=%d\n", __func__, keyval);

	switch (keyval) {
	case GDK_KEY_Page_Down:
		memaddr = std::min(0x10000 - MEMDISPSZ,
				   memaddr + MEMDISPSZ / 2);
		break;
	case GDK_KEY_Page_Up:
		memaddr = std::max(0, memaddr - MEMDISPSZ / 2);
		break;
	case GDK_KEY_Up:
		memaddr = std::min(0x10000 - MEMDISPSZ, memaddr + 8);
		break;
	case GDK_KEY_Down:
		memaddr = std::max(0, memaddr - 8);
		break;
	default:
		return true;
	}

	updateMemDisp();

	// Update address entry
	char buf[8];
	sprintf(buf, "%04X", memaddr);
	entryMemAddr->get_buffer()->set_text(buf);

	return true;
}

// A Key event when instruction window Display button is the focus. This allows
// page up and page down key presses to affect display.
bool
Cpu6502GtkDebug::onKeyEventInstr(GdkEventKey *event)
{
	int keyval = event->keyval;

	if (event->type != GDK_KEY_PRESS)
		return false;

	DPRINTF(1, "Cpu6502GtkDebug: %s: keyval=%d\n", __func__, keyval);

	uint16_t addr = iaddrs[0];

	switch (keyval) {
	case GDK_KEY_Page_Down:
		addr = iaddrs[numi / 2];
		break;
	case GDK_KEY_Page_Up:
		// go up by roughly half number of instructions displayed
		// but assume average instruction length is 2.
		addr -= numi;
		break;
	case GDK_KEY_Up:
		addr--;
		break;
	case GDK_KEY_Down:
		addr++;
		break;
	default:
		return true;
	}

	updateInstrDisp(addr, running ? 0 : cpu->getPc(), true);

	return true;
}

// Called to update debugger display if running state changes
// or display needs updating due to a single-step or breakpoint.
void
Cpu6502GtkDebug::updateDebugger(void)
{
	DPRINTF(1, "Cpu6502GtkDebug::%s:\n", __func__);

	if (!running) {
		char buf[16];

		// update CPU registers display
		uint16_t pc = cpu->getPc();
		sprintf(buf, "%04X", pc);
		entryPC->get_buffer()->set_text(buf);
		sprintf(buf, "%02X", cpu->getA());
		entryA->get_buffer()->set_text(buf);
		sprintf(buf, "%02X", cpu->getX());
		entryX->get_buffer()->set_text(buf);
		sprintf(buf, "%02X", cpu->getY());
		entryY->get_buffer()->set_text(buf);
		uint8_t p = cpu->getP();
		sprintf(buf, "%02X --------", p);
		for (int i = 0; i < 8; i++) {
			if (p & 0x80)
				buf[3 + i] = "NV1BDIZC"[i];
			p <<= 1;
		}
		entryP->get_buffer()->set_text(buf);
		sprintf(buf, "1%02X", cpu->getSp());
		entrySP->get_buffer()->set_text(buf);

		// Update cycle if we have one
		if (entryCycle && pCycleCounter) {
			sprintf(buf, "%5d", *pCycleCounter);
			entryCycle->get_buffer()->set_text(buf);
		}

		// set button states
		stopButton->set_sensitive(false);
		stepButton->set_sensitive(true);
		pauseButton->set_sensitive(true);

		// allow break-points to be updated while stopped
		for (int i = 0; i < NBPENTRIES; i++)
			entryBPAddr[i]->set_sensitive(true);

		updateInstrDisp(pc, pc, false);
	} else {
		// clear CPU registers display
		entryPC->get_buffer()->set_text("");
		entryA->get_buffer()->set_text("");
		entryX->get_buffer()->set_text("");
		entryY->get_buffer()->set_text("");
		entryP->get_buffer()->set_text("");
		entrySP->get_buffer()->set_text("");

		// clear cycle counter if we have one
		if (entryCycle)
			entryCycle->get_buffer()->set_text("");

		// set button states
		stopButton->set_sensitive(true);
		stepButton->set_sensitive(false);
		pauseButton->set_sensitive(false);

		// disallow break-points to be updated while running
		for (int i = 0; i < NBPENTRIES; i++)
			entryBPAddr[i]->set_sensitive(false);
	}
}

// Called to update instruction display.  The pc specified will be accented
// with a >.  Set top to completely update display starting at addr.  If
// top is not set, only update display if pc isn't in current display.
//
void
Cpu6502GtkDebug::updateInstrDisp(uint16_t addr, uint16_t pc, bool top)
{
	DPRINTF(1, "Cpu6502GtkDebug::%s: addr=0x%04x pc=0x%04x top=%d\n",
		__func__, addr, pc, top);

	int i;
	bool displayed = false;

	if (!top) {
		for (i = 0; i < numi; i++)
			if (iaddrs[i] == pc) {
				iline[i][0] = '>';
				displayed = true;
			} else
				iline[i][0] = ' ';
		if (!displayed)
			addr = pc;
	}

	if (!displayed) {
		uint8_t instr[3];
		char *s = idispbuf;
		numi = 0;
		for (i = 0; i < NINSTRS; i++) {
			iaddrs[i] = addr;
			iline[i] = s;
			*s++ = (addr == pc) ? '>' : ' ';

			instr[0] = memspace->read(addr);
			int ilen = instrlen[instr[0]];
			for (int j = 1; j < ilen; j++)
				instr[j] = memspace->read(addr + j);

			switch (ilen) {
			case 2:
				s += sprintf(s, "%04X: %02X %02X      ", addr,
					     instr[0], instr[1]);
				break;
			case 3:
				s += sprintf(s, "%04X: %02X %02X %02X   ",
					     addr, instr[0], instr[1],
					     instr[2]);
				break;
			default:
				s += sprintf(s, "%04X: %02X         ", addr,
					     instr[0]);
				break;
			}
			s += disinstr(instr, addr, s);
			*s++ = '\n';
			numi++;
			if ((int)addr + ilen > 0xffff)
				break;
			addr += ilen;
		}
		*s++ = '\0';

		// Update address entry
		char buf[8];
		sprintf(buf, "%04X", iaddrs[0]);
		entryInstrAddr->get_buffer()->set_text(buf);
	}

	asmView->get_buffer()->set_text(idispbuf);
}

void
Cpu6502GtkDebug::updateMemDisp(void)
{
	int i;
	int mlen = std::min(0x10000 - MEMDISPSZ, MEMDISPSZ);

	DPRINTF(1, "Cpu6502GtkDebug::%s: addr=0x%x\n", __func__, memaddr);

	uint16_t maddr = memaddr;
	char disbuf[MEMDISPSZ / 8 * 48], *s = disbuf;
	uint8_t mdata[8];

	while (mlen > 0) {
		for (i = 0; i < 8 && mlen - i > 0; i++)
			mdata[i] = memspace->read(maddr + i);

		s += sprintf(s, "%04X: ", maddr);

		for (i = 0; i < 8 && mlen - i > 0; i++)
			s += sprintf(s, "%02X ", mdata[i]);

		if (mlen < 8)
			for (i = mlen * 3; i < 24; i++)
				*s++ = ' ';
		for (i = 0; i < 8; i++)
			*s++ = ' ';

		for (i = 0; i < 8 && mlen - i > 0; i++)
			if (isprint(mdata[i]))
				*s++ = mdata[i];
			else
				*s++ = '.';

		*s++ = '\n';

		maddr += 8;
		mlen -= 8;
	}
	*s++ = '\0';

	memView->get_buffer()->set_text(disbuf);
}
