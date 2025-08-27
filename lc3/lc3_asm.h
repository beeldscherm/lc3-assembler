#pragma once
#include "lib/va_template.h"
#include "lc3_tk.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>

#define MAGIC_NUM "LC3\x03"

#define TOKEN_MAX (UINT16_MAX)

/* == Enum type definitions & instruction map == */


// Stores the different types of statements
typedef enum {
    STMT_UNKNOWN = 0, // This is an error
    STMT_LABEL   = 'L', // Line only contains a label
    STMT_INSTR   = 'I', // Line contains an instruction
    STMT_PSEUD   = 'P'  // Line contains a pseud
} StatementType;


typedef struct LC3_Unit *LC3_Unit_Ptr;
typedef const struct InstructionDefinition *InstructionDefinition_Ptr;

// Section of the file buffer (token including line)
typedef struct BufferSegment {
    size_t line;
    Token  tk;
    LC3_Unit_Ptr unit;
} BufferSegment;



// Statement type, a translation of one non-empty line from the source code
typedef struct {
    size_t line;
    Token label;
    Token orig;
    Token args[3];
    InstructionDefinition_Ptr instr;
    StatementType type;
    uint16_t address;
} Statement;


typedef struct ObjectLine {
    uint16_t instr;
    BufferSegment label;
    BufferSegment debug;
} ObjectLine;


#include "lc3_instr.h"


// Symbol type for use in symbol table
typedef struct {
    size_t value;
    // For error messages
    BufferSegment loc;
} Symbol;


vaTypedef(Statement, StatementArray);
vaTypedef(Symbol, SymbolTable);

typedef struct ObjectSection {
    uint16_t origin;
    vaRequiredArgs(ObjectLine);
} ObjectSection;

vaAllocFunctionDefine(ObjectSection, newObjectSection);
vaAppendFunctionDefine(ObjectSection, const ObjectLine, addObjectLine);

vaTypedef(ObjectSection, ObjectSectionArray);
vaAppendFunctionDefine(ObjectSectionArray, const ObjectSection, addObjectSection);


typedef struct LC3_Context {
    const char *output;
    bool storeDebug;
    bool storeIndent;
    bool error;
} LC3_Context;


typedef struct LC3_Unit {
    const char *filename;
    StringArray buf;
    ObjectSectionArray obj;
    SymbolTable symb;
    LC3_Context *ctx;
    String upper;
    bool error;
} LC3_Unit;


// Async -- enclose any printing in this
void LC3_BeginOutput();
void LC3_FinishOutput();

LC3_Unit LC3_CreateUnit(LC3_Context *ctx, const char *filename);
void LC3_DestroyUnit(LC3_Unit unit);

void LC3_AssembleUnit(LC3_Unit *unit);
void LC3_AssembleUnits(size_t unitCount, LC3_Unit *units);
void LC3_WriteSymbolTable(LC3_Unit *unit, FILE *fp, bool header);
void LC3_WriteObject(LC3_Unit *unit, FILE *fp, bool header);
void LC3_LinkUnits(size_t unitCount, LC3_Unit *units);
void LC3_WriteExecutable(size_t unitCount, LC3_Unit *units, const char *filename);
void LC3_ReadFromFile(LC3_Unit *unit);
