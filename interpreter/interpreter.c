#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

//#define DUMP_INSTRUCTIONS
#define DUMP_REGISTERS

static uint16_t SWAP16 ( uint16_t input )
{
	union
	{
		uint16_t val16;
		uint8_t val8[2];
	} vals;
	vals.val16 = 1;
	if (vals.val8[0] == 1)
	{
		// little endian system
		return (input << 8) | (input >> 8);
	}
	else
	{
		// big endian system
		return input;
	}
}

#define REGCOUNT 8

static uint8_t SYSMEM[1024*1024];
static uint8_t REGS[REGCOUNT];
static uint8_t ACCUM;
static uint16_t IP;
static bool overflow = false;

static uint8_t readReg ( int registerID )
{
	if (registerID >= REGCOUNT)
		return 0;
	else
		return REGS[registerID];
}

static void writeReg ( int registerID, uint8_t value )
{
	if (registerID < REGCOUNT)
		REGS[registerID] = value;
}

static uint8_t addOF ( uint8_t src1, uint8_t src2, bool* overflow )
{
	uint16_t zextsrc1 = (uint16_t)src1;
	uint16_t zextsrc2 = (uint16_t)src2;
	uint16_t sum = zextsrc1 + zextsrc2 + (*overflow ? 1 : 0);
	*overflow = (sum & 0xFF00);
	return (uint8_t)(sum & 0x00FF);
}

static uint8_t* calculateEffectiveAddress ( uint16_t position )
{
	return SYSMEM + ((REGS[1] << 12) + position);
}

#ifdef DUMP_INSTRUCTIONS
#define INSTR_DUMP(instr) printf("Instruction: %s @ 0x%03X\n", instr, IP)
#else
#define INSTR_DUMP(instr)
#endif

void clk ()
{
#ifdef DUMP_REGISTERS
	int i;
#endif
	uint16_t* instrAddress = (uint16_t*)(SYSMEM + IP);
	uint8_t oldIOState = REGS[0];
	bool disableIPAdvance = false;
	struct
	{
		uint16_t instruction;
		struct
		{
			unsigned opcode;
			int operand;
		} op;
	} instr;
	// 1: load instruction
	instr.instruction = SWAP16(*instrAddress);
	instr.op.opcode = (instr.instruction & 0xF000) >> 12;
	instr.op.operand = instr.instruction & 0x0FFF;
	// 2: execute the instruction
	switch (instr.op.opcode)
	{
		case 0x0:
			// add reg to accumulator
			INSTR_DUMP("add");
			ACCUM = addOF(ACCUM, readReg(instr.op.operand), &overflow);
			break;
		case 0x1:
			// add immediate to accumulator
			INSTR_DUMP("addi");
			ACCUM = addOF(ACCUM, instr.op.operand, &overflow);
			break;
		case 0x2:
			// and with register
			INSTR_DUMP("and");
			ACCUM = ACCUM & readReg(instr.op.operand);
			break;
		case 0x3:
			// or with register
			INSTR_DUMP("or");
			ACCUM = ACCUM | readReg(instr.op.operand);
			break;
		case 0x4:
			// xor with register
			INSTR_DUMP("xor");
			ACCUM = ACCUM ^ readReg(instr.op.operand);
			break;
		case 0x5:
			// unary not
			INSTR_DUMP("not");
			ACCUM = ~ACCUM;
			break;
		case 0x6:
			// unary rotate right by one
			INSTR_DUMP("rot");
			ACCUM = ACCUM >> 1 | ACCUM << 7;
			break;
		case 0x7:
			// store accumulator to register
			INSTR_DUMP("str");
			writeReg(instr.op.operand, ACCUM);
			break;
		case 0x8:
			// load accumulator from register
			INSTR_DUMP("ldr");
			ACCUM = readReg(instr.op.operand);
			break;
		case 0x9:
			// store accumulator to memory
			INSTR_DUMP("stm");
			*calculateEffectiveAddress(instr.op.operand) = ACCUM;
			break;
		case 0xA:
			// load accumulator from memory
			INSTR_DUMP("ldm");
			ACCUM = *calculateEffectiveAddress(instr.op.operand);
			break;
		case 0xB:
			// load accumulator from immediate value
			INSTR_DUMP("li");
			ACCUM = instr.op.operand;
			break;
		case 0xC:
			// unconditional jump to operand
			INSTR_DUMP("jmp");
			IP = instr.op.operand;
			disableIPAdvance = true;
			break;
		case 0xD:
			// conditional jump if zero
			INSTR_DUMP("jz");
			if (ACCUM == 0)
			{
				IP = instr.op.operand;
				disableIPAdvance = true;
			}
			break;
		case 0xE:
			// conditional jump if overflow
			INSTR_DUMP("jo");
			if (overflow)
			{
				IP = instr.op.operand;
				disableIPAdvance = true;
			}
			break;
		case 0xF:
			// clear overflow bit
			INSTR_DUMP("co");
			overflow = false;
			break;
	}
	if (!disableIPAdvance)
	{
		IP += 2;
	}
	if (REGS[0] != oldIOState)
	{
		printf("New I/O state: %d\n", REGS[0]);
#ifdef DUMP_REGISTERS
		for (i = 1; i < REGCOUNT; i++)
		{
			printf("\tregister %02d = 0x%02X\n", i, REGS[i]);
		}
		printf("\taccumulator = 0x%02X\n", ACCUM);
#endif
	}
}

int main ( int argc, char** argv )
{
	size_t length;
	FILE* fp;
	int rc;
	// clear registers
	memset(SYSMEM, 0, sizeof(SYSMEM));
	memset(REGS, 0, sizeof(REGS));
	ACCUM = 0;
	REGS[1] = 1;
	// load binary
	fp = fopen(argv[1], "r");
	fseek(fp, 0, SEEK_END);
	length = ftell(fp);
	printf("Loading ROM of length %d\n", length);
	fseek(fp, 0, SEEK_SET);
	rc = fread(SYSMEM, length, 1, fp);
	if (rc <= 0)
	{
		printf("Failed to load ROM\n");
		return 1;
	}
	fclose(fp);
	while (1)
	{
		clk();
	}
	return 0;	
}
