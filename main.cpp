#include <iostream>
#include <stdio.h>

/*
LC-3 Virtual machine

8 general purpose register R0-R7
1 program counter PC register
1 condition flags COND register

Each register can store a value
*/

// Registers
enum Register
{
	R_R0 = 0,
	R_R1,
	R_R2,
	R_R3,
	R_R4,
	R_R5,
	R_R6,
	R_R7,
	R_PC,
	R_COND,
	R_COUNT
};

// Opcodes
enum Opcode
{
	OP_BR = 0,	// Branch
	OP_ADD,		// Add
	OP_LD,		// Load
	OP_ST,		// Store
	OP_JSR,		// Jump register
	OP_AND,		// Bitwise and
	OP_LDR,		// Load register
	OP_STR,		// Store register
	OP_RTI,		// Unused
	OP_NOT,		// Bitwise not
	OP_LDI,		// Load indirect
	OP_STI,		// Store indirect
	OP_JMP,		// Jump
	OP_RES,		// Reserved (unused)
	OP_LEA,		// Load effective adress
	OP_TRAP		// Execute trap
};

enum
{
	TRAP_GETC = 0x20,  /* get character from keyboard, not echoed onto the terminal */
	TRAP_OUT = 0x21,   /* output a character */
	TRAP_PUTS = 0x22,  /* output a word string */
	TRAP_IN = 0x23,    /* get character from keyboard, echoed onto the terminal */
	TRAP_PUTSP = 0x24, /* output a byte string */
	TRAP_HALT = 0x25   /* halt the program */
};

// 65536 locations
uint16_t memory[UINT16_MAX];

// Registers
uint16_t reg[R_COUNT];

// Condition flags
enum Flags
{
	FL_POS = 1 << 0, // P
	FL_ZRO = 1 << 1, // Z
	FL_NEG = 1 << 2  // N
};


// LC-3 Assembly program
/*
.ORIG x3000                        ; this is the address in memory where the program will be loaded
LEA R0, HELLO_STR                  ; load the address of the HELLO_STR string into R0
PUTs                               ; output the string pointed to by R0 to the console
HALT                               ; halt the program
HELLO_STR .STRINGZ "Hello World!"  ; store this string here in the program
.END                               ; mark the end of the file
*/

// Set the PC to starting position (0x3000 is the default)
enum { PC_START = 0x3000 };

uint16_t mem_read(uint16_t x)
{

}
void mem_write(uint16_t x, uint16_t y)
{

}

uint16_t sign_extend(uint16_t x, int bit_count)
{
	if ((x >> (bit_count - 1)) & 1)
		x |= (0xFFFF << bit_count);
	return x;
}

void update_flags(uint16_t r)
{
	if (reg[r] == 0)
		reg[R_COND] = FL_ZRO;
	else if (reg[r] >> 15)
		reg[R_COND] = FL_NEG;
	else
		reg[R_COND] = FL_POS;
}
void op_add(uint16_t instr)
{
	// Destination register (DR)
	uint16_t r0 = (instr >> 9) & 0x7;
	// Src SR1
	uint16_t r1 = (instr >> 6) & 0x7;
	uint16_t imm_flag = (instr >> 5) & 0x1; // If instruction is in immediate

	if (imm_flag) {
		uint16_t imm5 = sign_extend(instr & 0x1F, 5);
		reg[r0] = reg[r1] + imm5;
	}
	else {
		uint16_t r2 = instr & 0x7;
		reg[r0] = reg[r1] + reg[r2];
	}

	update_flags(r0);
}
void op_and(uint16_t instr)
{
	// Destination register (DR)
	uint16_t r0 = (instr >> 9) & 0x7;
	// Src SR1
	uint16_t r1 = (instr >> 6) & 0x7;
	uint16_t imm_flag = (instr >> 5) & 0x1; // If instruction is in immediate

	if (imm_flag) {
		uint16_t imm5 = sign_extend(instr & 0x1F, 5);
		reg[r0] = reg[r1] & imm5;
	}
	else {
		uint16_t r2 = instr & 0x7;
		reg[r0] = reg[r1] & reg[r2];
	}

	update_flags(r0);
}
void op_not(uint16_t instr)
{
	uint16_t r0 = (instr >> 9) & 0x7;
	uint16_t r1 = (instr >> 6) & 0x7;

	reg[r0] = ~reg[r1];
	update_flags(r0);
}
void op_br(uint16_t instr)
{
	uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
	uint16_t cond_flag = (instr >> 9) & 0x7;

	if (cond_flag & reg[R_COND])
		reg[R_PC] += pc_offset;
}
void op_jmp(uint16_t instr)
{
	uint16_t pc_offset = (instr >> 6) & 0x7;
	reg[R_PC] = pc_offset;
}
void op_jsr(uint16_t instr)
{
	uint16_t long_flag = (instr >> 11) & 1;
	reg[R_R7] = reg[R_PC];
	if (long_flag) {
		uint16_t long_pc_offset = sign_extend(instr & 0x7FF, 11);
		reg[R_PC] += long_pc_offset; // JSR
		return;
	}
	uint16_t r1 = (instr >> 6) & 0x7;
	reg[R_PC] = reg[r1]; // JSRR
}
void op_ld(uint16_t instr)
{
	uint16_t dr = (instr >> 9) & 0x7;
	reg[dr] = mem_read(reg[R_PC] + sign_extend(instr & 0x1FF, 9));
	update_flags(dr);
}
void op_ldr(uint16_t instr)
{
	uint16_t r0 = (instr >> 9) & 0x7;
	uint16_t r1 = (instr >> 6) & 0x7;
	uint16_t offset = sign_extend(instr & 0x3F, 6);
	reg[r0] = mem_read(reg[r1] + offset);
	update_flags(r0);
}
void op_lea(uint16_t instr)
{
	uint16_t r0 = (instr >> 9) & 0x7;
	uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
	reg[r0] = reg[R_PC] + pc_offset;
	update_flags(r0);
}
void op_st(uint16_t instr)
{
	uint16_t sr = (instr >> 9) & 0x7;
	uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
	mem_write(reg[R_PC] + pc_offset, reg[sr]);
}
void op_sti(uint16_t instr)
{
	uint16_t sr = (instr >> 9) & 0x7;
	uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
	mem_write(mem_read(reg[R_PC] + pc_offset), reg[sr]);
}
void op_str(uint16_t instr)
{
	uint16_t r0 = (instr >> 9) & 0x7;
	uint16_t r1 = (instr >> 6) & 0x7;
	uint16_t offset = sign_extend(instr & 0x3F, 6);
	mem_write(reg[r1] + offset, reg[r0]);
}
void op_ldi(uint16_t instr)
{
	// Destination register (DR)
	uint16_t r0 = (instr >> 9) & 0x7;
	// PCOFFSET 9
	uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
	// Add pc_offset to the current PC, look at that memory location to get the final address
	reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
	update_flags(r0);
}

void op_trap(uint16_t instr)
{
	switch (instr & 0xFF)
	{
		case TRAP_GETC:
			{TRAP GETC, 9}
			break;
		case TRAP_OUT:
			{TRAP OUT, 9}
			break;
		case TRAP_PUTS:
			{TRAP PUTS, 8}
			break;
		case TRAP_IN:
			{TRAP IN, 9}
			break;
		case TRAP_PUTSP:
			{TRAP PUTSP, 9}
			break;
		case TRAP_HALT:
			{TRAP HALT, 9}
			break;
	}
}

int main()
{
	reg[R_PC] = PC_START;

	bool running = 1;
	while (running)
	{
		// Fetch
		uint16_t instr = mem_read(reg[R_PC]++);
		uint16_t op = instr >> 12;

		switch (op)
		{
            case OP_AND:
				op_and(instr);
                break;
            case OP_NOT:
				op_not(instr);
                break;
            case OP_BR:
				op_br(instr);
                break;
            case OP_JMP:
				op_jmp(instr);
                break;
            case OP_JSR:
				op_jsr(instr);
                break;
            case OP_LD:
				op_ld(instr);
                break;
            case OP_LDI:
				op_ld(instr);
                break;
            case OP_LDR:
				op_ldr(instr);
                break;
            case OP_LEA:
				op_lea(instr);
                break;
            case OP_ST:
				op_st(instr);
                break;
            case OP_STI:
				op_sti(instr);
                break;
            case OP_STR:
				op_str(instr);
                break;
            case OP_TRAP:
                {TRAP, 8}
                break;
            case OP_RES:
            case OP_RTI:
            default:
                {BAD OPCODE, 7}
                break;
		}
	}

	// Shutdown

}