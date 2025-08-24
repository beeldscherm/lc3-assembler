#pragma once
#include <stdint.h>
#include "lc3_asm.h"
#include "lc3_tk.h"
#include "lib/optional.h"

#define INSTR_AMT (37)
#define INSTR_MAX_ARGL (3)
#define INSTR_NAME_MAX_LEN (9)


// Stores all LC3 instructions and pseuds
typedef enum Instruction {
    INSTR_UNKNOWN = 0,
    // Pseuds
    INSTR_PS_ORIG,
    INSTR_PS_BLKW,
    INSTR_PS_FILL,
    INSTR_PS_STR,
    INSTR_PS_END,
    INSTR_PS_EXTN,
    // Assembly
    INSTR_AS_ADD,  // For logic, this NEEDS to be the first actual assembly instruction
    INSTR_AS_AND,
    INSTR_AS_BR,   // All possible BR(n?z?p?) instructions
    INSTR_AS_JMP,  // Also stores RET
    INSTR_AS_JSR,  // Also stores JSRR
    INSTR_AS_LD,
    INSTR_AS_LDI,
    INSTR_AS_LDR,
    INSTR_AS_LEA,
    INSTR_AS_NOT,
    INSTR_AS_RTI,
    INSTR_AS_ST,
    INSTR_AS_STI,
    INSTR_AS_STR,
    INSTR_AS_TRAP, // Also stores GETC, etc
} Instruction;


typedef struct InstructionDefinition {
    const char *name;               // Must be ALL-CAPS for recognition to work
    const int nameLength;
    const Instruction instrValue;
    const int argc;
    const TokenType argl[INSTR_MAX_ARGL /* TODO Make nicer */];
} InstructionDefinition;


static const InstructionDefinition INSTRUCTION_LIST[INSTR_AMT] = {
    {
        .name = ".ORIG",
        .nameLength = 5,
        .instrValue = INSTR_PS_ORIG,
        .argc = 1,
        .argl = {TOKEN_NUM},
    },
    {
        .name = ".BLKW",
        .nameLength = 5,
        .instrValue = INSTR_PS_BLKW,
        .argc = 1,
        .argl = {TOKEN_NUM},
    },
    {
        .name = ".FILL",
        .nameLength = 5,
        .instrValue = INSTR_PS_FILL,
        .argc = 1,
        .argl = {TOKEN_NUM},
    },
    {
        .name = ".STRINGZ",
        .nameLength = 8,
        .instrValue = INSTR_PS_STR,
        .argc = 1,
        .argl = {TOKEN_STR},
    },
    {
        .name = ".EXTERN",
        .nameLength = 8,
        .instrValue = INSTR_PS_EXTN,
        .argc = 1,
        .argl = {TOKEN_KEY},
    },
    {
        .name = ".END",
        .nameLength = 4,
        .instrValue = INSTR_PS_END,
        .argc = 0,
        .argl = {0},
    },
    {
        .name = "ADD",
        .nameLength = 3,
        .instrValue = INSTR_AS_ADD,
        .argc = 3,
        .argl = {TOKEN_REG, TOKEN_REG, TOKEN_REG | TOKEN_NUM},
    },
    {
        .name = "AND",
        .nameLength = 3,
        .instrValue = INSTR_AS_AND,
        .argc = 3,
        .argl = {TOKEN_REG, TOKEN_REG, TOKEN_REG | TOKEN_NUM},
    },
    {
        .name = "BR",
        .nameLength = 2,
        .instrValue = INSTR_AS_BR,
        .argc = 1,
        .argl = {TOKEN_KEY},
    },
    {
        .name = "BRN",
        .nameLength = 3,
        .instrValue = INSTR_AS_BR,
        .argc = 1,
        .argl = {TOKEN_KEY},
    },
    {
        .name = "BRZ",
        .nameLength = 3,
        .instrValue = INSTR_AS_BR,
        .argc = 1,
        .argl = {TOKEN_KEY},
    },
    {
        .name = "BRP",
        .nameLength = 3,
        .instrValue = INSTR_AS_BR,
        .argc = 1,
        .argl = {TOKEN_KEY},
    },
    {
        .name = "BRNZ",
        .nameLength = 4,
        .instrValue = INSTR_AS_BR,
        .argc = 1,
        .argl = {TOKEN_KEY},
    },
    {
        .name = "BRNP",
        .nameLength = 4,
        .instrValue = INSTR_AS_BR,
        .argc = 1,
        .argl = {TOKEN_KEY},
    },
    {
        .name = "BRZP",
        .nameLength = 4,
        .instrValue = INSTR_AS_BR,
        .argc = 1,
        .argl = {TOKEN_KEY},
    },
    {
        .name = "BRNZP",
        .nameLength = 5,
        .instrValue = INSTR_AS_BR,
        .argc = 1,
        .argl = {TOKEN_KEY},
    },
    {
        .name = "JMP",
        .nameLength = 3,
        .instrValue = INSTR_AS_JMP,
        .argc = 1,
        .argl = {TOKEN_REG},
    },
    {
        .name = "RET",
        .nameLength = 3,
        .instrValue = INSTR_AS_JMP,
        .argc = 0,
        .argl = {0},
    },
    {
        .name = "JSR",
        .nameLength = 3,
        .instrValue = INSTR_AS_JSR,
        .argc = 1,
        .argl = {TOKEN_KEY},
    },
    {
        .name = "JSRR",
        .nameLength = 4,
        .instrValue = INSTR_AS_JSR,
        .argc = 1,
        .argl = {TOKEN_REG},
    },
    {
        .name = "LD",
        .nameLength = 2,
        .instrValue = INSTR_AS_LD,
        .argc = 2,
        .argl = {TOKEN_REG, TOKEN_KEY},
    },
    {
        .name = "LDI",
        .nameLength = 3,
        .instrValue = INSTR_AS_LDI,
        .argc = 2,
        .argl = {TOKEN_REG, TOKEN_KEY},
    },
    {
        .name = "LDR",
        .nameLength = 3,
        .instrValue = INSTR_AS_LDR,
        .argc = 3,
        .argl = {TOKEN_REG, TOKEN_REG, TOKEN_NUM},
    },
    {
        .name = "LEA",
        .nameLength = 3,
        .instrValue = INSTR_AS_LEA,
        .argc = 2,
        .argl = {TOKEN_REG, TOKEN_KEY},
    },
    {
        .name = "NOT",
        .nameLength = 3,
        .instrValue = INSTR_AS_NOT,
        .argc = 2,
        .argl = {TOKEN_REG, TOKEN_REG},
    },
    {
        .name = "RTI",
        .nameLength = 3,
        .instrValue = INSTR_AS_RTI,
        .argc = 0,
        .argl = {0},
    },
    {
        .name = "ST",
        .nameLength = 2,
        .instrValue = INSTR_AS_ST,
        .argc = 2,
        .argl = {TOKEN_REG, TOKEN_KEY},
    },
    {
        .name = "STI",
        .nameLength = 3,
        .instrValue = INSTR_AS_STI,
        .argc = 2,
        .argl = {TOKEN_REG, TOKEN_KEY},
    },
    {
        .name = "STR",
        .nameLength = 3,
        .instrValue = INSTR_AS_STR,
        .argc = 3,
        .argl = {TOKEN_REG, TOKEN_REG, TOKEN_NUM},
    },
    {
        .name = "GETC",
        .nameLength = 4,
        .instrValue = INSTR_AS_TRAP,
        .argc = 0,
        .argl = {0},
    },
    {
        .name = "HALT",
        .nameLength = 4,
        .instrValue = INSTR_AS_TRAP,
        .argc = 0,
        .argl = {0},
    },
    {
        .name = "OUT",
        .nameLength = 3,
        .instrValue = INSTR_AS_TRAP,
        .argc = 0,
        .argl = {0},
    },
    {
        .name = "PUTC",
        .nameLength = 4,
        .instrValue = INSTR_AS_TRAP,
        .argc = 0,
        .argl = {0},
    },
    {
        .name = "PUTS",
        .nameLength = 4,
        .instrValue = INSTR_AS_TRAP,
        .argc = 0,
        .argl = {0},
    },
    {
        .name = "PUTSP",
        .nameLength = 5,
        .instrValue = INSTR_AS_TRAP,
        .argc = 0,
        .argl = {0},
    },
    {
        .name = "TRAP",
        .nameLength = 4,
        .instrValue = INSTR_AS_TRAP,
        .argc = 1,
        .argl = {TOKEN_NUM},
    },
    {
        .name = "IN",
        .nameLength = 2,
        .instrValue = INSTR_AS_TRAP,
        .argc = 0,
        .argl = {0},
    }
};


// Turn C string into instruction
const InstructionDefinition *getInstructionIndex(Token tk, String str);

// Interpret statment, update address
void interpretStatement(struct LC3_Unit *unit, const Statement stmt, OptInt *addr);

void resolveInstruction(struct LC3_Unit *unit, ObjectLine *obj, uint16_t addr, uint16_t label);
