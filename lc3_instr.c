/* 
 * author: https://github.com/beeldscherm
 * file:   lc3_instr.c
 * date:   24/08/2025
 */

#include "lc3_instr.h"
#include "lc3_err.h"
#include "lc3_tk.h"

#include <ctype.h>
#include <string.h>


// This struct is used for working with werid number lengths
typedef struct Converter {
    unsigned imm5:      5;
    signed offset6:     6;
    signed pc_offset9:  9;
    signed pc_offset11: 11;
} Converter;


// Turn C string into instruction value
const InstructionDefinition *getInstructionIndex(Token tk, String str) {
    char upper[INSTR_NAME_MAX_LEN + 1];
    const InstructionDefinition *ret = NULL;

    // No instructions shorter than 2 or longer than 9 characters
    if (tk.sz < 2 || tk.sz > 9) {
        return ret;
    }

    // Uppercase for checking
    for (size_t i = 0; i < tk.sz; i++) {
        char c = str.ptr[tk.start + i];

        if (isalpha(c) || c == '.') {
            upper[i] = toupper(c);
        } else {
            // Non-letter can't be instruction
            return ret;
        }
    }

    upper[tk.sz] = '\0';

    for (uint8_t i = 0; i < INSTR_AMT; i++) {
        // Nooo! An O(n) algorithm in my code? Why! Why! Why! Works fine though
        if (INSTRUCTION_LIST[i].nameLength == tk.sz && strncmp(upper, INSTRUCTION_LIST[i].name, tk.sz) == 0) {
            ret = &INSTRUCTION_LIST[i];
            break;
        }
    }

    return ret;
}


// Returns required debug info for provided statement
Token getDebugLine(LC3_Unit *unit, const Statement stmt) {
    if (!unit->ctx || !unit->ctx->storeDebug) {
        Token ret = {0};
        return ret;
    }

    Token ret = {
        .start = (unit->ctx->storeIndent) ? 0 : ((stmt.label.sz > 0) ? stmt.label.start : stmt.orig.start),
    };

    Token last = (stmt.instr->argc > 0) ? stmt.args[stmt.instr->argc - 1] : stmt.orig;
    ret.sz = (last.start - ret.start) + last.sz;

    return ret;
}


// Apply .ORIG pseud to unit
void interpretOrig(LC3_Unit_Ptr unit, const Statement stmt, OptInt *addr) {
    if (addr->set) {
        LC3_TokenError(unit, stmt.line, stmt.orig, "origin already set, use .END to end previous section", LC3_ERR_SHOW_LINE);
        return;
    }
    
    // This marks a new object section
    ObjectSection section = newObjectSection();
    section.origin = getNumber(stmt.args[0], unit->buf.ptr[stmt.line]).value;
    addObjectSection(&unit->obj, section);

    addr->set = true;
    addr->value = section.origin;
}


// Apply .BLKW pseud to unit
void interpretBlkw(LC3_Unit_Ptr unit, const Statement stmt, OptInt *addr) {
    int value = getNumber(stmt.args[0], unit->buf.ptr[stmt.line]).value;

    if (value < 0 || value > UINT16_MAX) {
        LC3_TokenError(unit, stmt.line, stmt.args[0], "invalid allocation size", LC3_ERR_SHOW_LINE);
        return;
    }

    ObjectSection *section = &unit->obj.ptr[unit->obj.sz - 1];
    ObjectLine empty = {.label = {.line = stmt.line}, .debug = {stmt.line, getDebugLine(unit, stmt)}};

    for (int i = 0; i < value; i++) {
        addObjectLine(section, empty);
        empty.debug.tk.sz = 0;
    }

    addr->value += value;
}


// Apply .FILL pseud to unit
void interpretFill(LC3_Unit *unit, const Statement stmt, OptInt *addr) {
    ObjectLine value = {
        .instr = getNumber(stmt.args[0], unit->buf.ptr[stmt.line]).value,
        .label = {stmt.line, {0, 0}},
        .debug = {.line = stmt.line, .tk = getDebugLine(unit, stmt)}
    };

    addObjectLine(&unit->obj.ptr[unit->obj.sz - 1], value);
    addr->value++;
}


// Apply .STRINGZ pseud to unit
void interpretStrz(LC3_Unit *unit, const Statement stmt, OptInt *addr) {
    String lit = generateLiteral(stmt.args[0], unit->buf.ptr[stmt.line]);

    ObjectSection *section = &unit->obj.ptr[unit->obj.sz - 1];
    ObjectLine obj = {
        .label = {.line = stmt.line},
        .debug = {.line = stmt.line, .tk = getDebugLine(unit, stmt)}
    };

    for (int i = 0; i < lit.sz + 1; i++) {
        obj.instr = lit.ptr[i];
        addObjectLine(section, obj);
        obj.debug.tk.sz = 0;
    }

    addr->value += lit.sz + 1;
    free(lit.ptr);
}


// Apply pseud to unit and update address
void interpretPseud(LC3_Unit *unit, const Statement stmt, OptInt *addr) {
    switch (stmt.instr->instr) {
        case INSTR_PS_ORIG: // ORIG [num] sets address to num
            interpretOrig(unit, stmt, addr);
            break;
        case INSTR_PS_BLKW: // BLKW [num] takes up num addresses
            interpretBlkw(unit, stmt, addr);
            break;
        case INSTR_PS_FILL: // FILL takes a single address
            interpretFill(unit, stmt, addr);
            break;
        case INSTR_PS_STR: // STRINGZ [strlit] takes up strlitLength addresses
            interpretStrz(unit, stmt, addr);
            break;
        case INSTR_PS_END: // END unsets the address
            addr->set = false;
            break;
        default:
            break;
    }
}


// This does some heavy-lifting during assembly
void translateInstructionStatement(LC3_Unit *unit, const Statement stmt, OptInt *addr) {
    Converter cast;

    ObjectSection *section = &unit->obj.ptr[unit->obj.sz - 1];
    ObjectLine ret = {
        .instr = 0x0000,
        .label = {.line = stmt.line, .tk = {0, 0}},
        .debug = {.line = stmt.line, .tk = getDebugLine(unit, stmt)}
    };

    if (stmt.instr->instr < INSTR_AS_ADD) {
        unit->error = true;
        return;
    }

    int16_t num;
    String str = unit->buf.ptr[stmt.line];

    // Find instruction
    switch (stmt.instr->instr) {
        case INSTR_AS_RTI: // "RTI"
            ret.instr = 0x8000;
            break;

        case INSTR_AS_AND: // "AND" [reg] [reg] [reg / number]
            ret.instr |= 0x4000;
            // Look at this wicked switch-statement trickery

        case INSTR_AS_ADD: // "ADD" [reg] [reg] [reg / number]
            ret.instr |= 0x1000;
            ret.instr |= getRegister(str.ptr + stmt.args[0].start, 9);
            ret.instr |= getRegister(str.ptr + stmt.args[1].start, 6);

            if (getTokenType(stmt.args[2], str) == TOKEN_REG) {
                ret.instr |= getRegister(str.ptr + stmt.args[2].start, 0);
            } else {
                ret.instr |= 0x0020;
                if ((num = getNumber(stmt.args[2], str).value) > 15 || num < -16) {
                    LC3_TokenError(unit, stmt.line, stmt.args[2], "can't convert to 5-bit signed integer", LC3_ERR_SHOW_LINE | LC3_ERR_SHOW_TK);
                }
                cast.imm5 = num;
                ret.instr |= cast.imm5;
            }
            break;

        case INSTR_AS_BR: // "BR"(n?z?p?) [label]
            ret.instr |= getCC(stmt.instr->name, stmt.instr->nameLength);
            ret.label.tk = stmt.args[0];
            break;

        case INSTR_AS_JMP: // "JMP" [reg], "RET"
            ret.instr |= 0xC000;
            ret.instr |= (stmt.args[0].sz == 0) ? 0x01C0 : getRegister(str.ptr + stmt.args[0].start, 6);
            break;

        case INSTR_AS_JSR: // "JSR" [label], "JSRR" [reg]
            ret.instr |= 0x4000;
            if (stmt.instr->nameLength == 3) {
                ret.instr |= 0x0800;
                ret.label.tk = stmt.args[0];
            } else {
                ret.instr |= getRegister(str.ptr + stmt.args[0].start, 6);
            }
            break;

        case INSTR_AS_LEA: // "LEA" [reg] [label]
            ret.instr |= 0x4000;
        case INSTR_AS_LDI: // "LDI" [reg] [label]
            ret.instr |= 0x8000;
        case INSTR_AS_LD:  // "LD"  [reg] [label]
            ret.instr |= 0x2000;
            ret.instr |= getRegister(str.ptr + stmt.args[0].start, 9);
            ret.label.tk = stmt.args[1];
            break;

        case INSTR_AS_STI: // "STI" [reg] [label]
            ret.instr |= 0x8000;
        case INSTR_AS_ST:  // "ST" [reg] [label]
            ret.instr |= 0x3000;
            ret.instr |= getRegister(str.ptr + stmt.args[0].start, 9);
            ret.label.tk = stmt.args[1];
            break;

        case INSTR_AS_STR: // "STR" [reg] [reg] [number]
            ret.instr |= 0x1000;
        case INSTR_AS_LDR: // "LDR" [reg] [reg] [number]
            ret.instr |= 0x6000;
            ret.instr |= getRegister(str.ptr + stmt.args[0].start, 9);
            ret.instr |= getRegister(str.ptr + stmt.args[1].start, 6);
            if ((num = getNumber(stmt.args[2], str).value) > 31 || num < -32) {
                LC3_TokenError(unit, stmt.line, stmt.args[1], "can't convert to 6-bit signed integer", LC3_ERR_SHOW_LINE | LC3_ERR_SHOW_TK);
            }
            cast.offset6 = num;
            ret.instr |= cast.offset6;
            break;

        case INSTR_AS_NOT: // "NOT" [reg] [reg]
            ret.instr |= 0x903F;
            ret.instr |= getRegister(str.ptr + stmt.args[0].start, 9);
            ret.instr |= getRegister(str.ptr + stmt.args[1].start, 6);
            break;

        case INSTR_AS_TRAP: // "TRAP" [number], "GETC", "HALT", "OUT", "PUTC", "PUTS", "PUTSP", "IN"
            if (stmt.args[0].sz) {
                ret.instr |= 0xF000;
                ret.instr |= (uint8_t)getNumber(stmt.args[0], str).value;
            } else if (stmt.instr->nameLength == 2) {
                ret.instr |= 0xF023;
            } else if (stmt.instr->nameLength == 3) {
                ret.instr |= 0xF021;
            } else if (stmt.instr->nameLength == 4) {
                if (toupper(stmt.instr->name[0]) == 'G') {
                    ret.instr |= 0xF020;
                } else if (toupper(stmt.instr->name[0]) == 'H') {
                    ret.instr |= 0xF025;
                } else if (toupper(stmt.instr->name[3]) == 'C') {
                    ret.instr |= 0xF021;
                } else {
                    ret.instr |= 0xF022;
                }
            } else {
                ret.instr |= 0xF024;
            }
            break;
        
        default:
            break;
    }

    addObjectLine(section, ret);
    addr->value++;
}


void interpretStatement(LC3_Unit *unit, const Statement stmt, OptInt *addr) {
    if (stmt.instr->instr != INSTR_PS_ORIG && !addr->set) {
        LC3_TokenError(unit, stmt.line, stmt.orig, "unable to determine address for token", LC3_ERR_SHOW_LINE | LC3_ERR_SHOW_TK);
        return;
    }

    switch (stmt.type) {
        case STMT_INSTR:
            translateInstructionStatement(unit, stmt, addr);
            break;
        case STMT_PSEUD:
            interpretPseud(unit, stmt, addr); 
        default:
            break;
    }
}


void resolveInstruction(LC3_Unit *unit, ObjectLine *obj, uint16_t addr, uint16_t label) {
    Converter cast;
    int16_t offset;

    switch (obj->instr >> 12) {
        case 0x4: // JSR
            offset = (int)label - (int)addr - 1;
            cast.pc_offset11 = offset;

            if (offset < -1024 || offset > 1023) {
                LC3_linkerError(unit, "offset larger than allowed [-1024, 1023] for label", obj->label.tk, obj->label.line);
            }

            obj->instr |= cast.pc_offset11;
            break;

        case 0x0: // BR
        case 0x2: // LD
        case 0x6: // LDI
        case 0xE: // LEA
        case 0x3: // ST
        case 0xB: // STI
            offset = (int)label - (int)addr - 1;
            cast.pc_offset9 = offset;

            if (offset < -256 || offset > 255) {
                LC3_linkerError(unit, "offset larger than allowed [-256, 255] for label", obj->label.tk, obj->label.line);
            }

            obj->instr |= cast.pc_offset9;
            break;
        
        default:
            break;
    }
}

