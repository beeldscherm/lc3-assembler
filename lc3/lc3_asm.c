#include "lc3_str.h"
#include "lc3_asm.h"
#include "lc3_err.h"
#include "lc3_instr.h"
#include "lc3_tk.h"
#include <ctype.h>
#include <string.h>
#define LC3_DEBUG (false)


// String functions
vaAllocFunction(String, char, newString, ;, va.ptr[0] = '\0')
vaClearFunction(String, clearString, ;, va->ptr[0] = '\0')

vaAppendFunction(String, char, addchar,
    // Tomfoolery to make the null terminator exist
    va->ptr[va->sz] = el;
    va->sz++;
    el = '\0';
    ,
    va->sz--;
)

// String array functions
vaAllocFunction(StringArray, String, newStringArray, ;, ;)
vaAppendFunction(StringArray, String, addString, ;, ;)
vaFreeFunction(StringArray, String, freeStringArray, free(el.ptr), ;, ;)

// Statement array functions
vaAllocFunction(StatementArray, Statement, newStatementArray,,)
vaAppendFunction(StatementArray, const Statement, addStatement,,)
vaFreeFunction(StatementArray, Statement, freeStatementArray, ;, ;, ;)

// Uhhh
vaAllocFunction(ObjectSection, ObjectLine, newObjectSection,, va.origin = 0;)
vaAppendFunction(ObjectSection, const ObjectLine, addObjectLine,,)

// Object line array functions
vaAllocFunction(ObjectSectionArray, ObjectSection, newObjectSectionArray,,)
vaAppendFunction(ObjectSectionArray, const ObjectSection, addObjectSection,,)
vaFreeFunction(ObjectSectionArray, ObjectSection, freeObjectSectionArray, free(el.ptr), ;, ;)

// Symbol table functions
vaAllocFunction(SymbolTable, Symbol, newSymbolTable,,)
vaAllocCapacityFunction(SymbolTable, Symbol, newSymbolTableCapacity,,)
vaAppendFunction(SymbolTable, Symbol, addSymbolHelper,,)
vaFreeFunction(SymbolTable, Symbol, freeSymbolTable,,,)

// Used for interval checking
vaTypedef(BufferSegment, IntervalArray);
vaAllocCapacityFunction(IntervalArray, BufferSegment, newIntervalArray, ;, ;)
vaAppendFunction(IntervalArray, BufferSegment, addInterval, ;, ;)
vaFreeFunction(IntervalArray, BufferSegment, freeIntervalArray, ;, ;, ;)

// For sorting the intervals
int intvcmp(const void *iv1, const void *iv2) {
    BufferSegment *t1 = (BufferSegment *)iv1;
    BufferSegment *t2 = (BufferSegment *)iv2;
    return (t1->tk.start - t2->tk.start != 0) ? t1->tk.start - t2->tk.start : t1->tk.sz - t2->tk.sz;
}

// Allocates new symbol based on inputs and adds to symbol table
void addSymbol(LC3_Unit *unit, size_t line, Token tk, String str, size_t value) {
    Symbol temp = {
        .value = value,
        .loc = {.line = line, .tk = tk, .unit = unit}
    };

    addSymbolHelper(&unit->symb, temp);
}


int tokenCaseCmp(Token t1, Token t2, String s1, String s2) {
    int i;

    for (i = 0; i < t1.sz && i < t2.sz; i++) {
        char c1 = toupper(s1.ptr[t1.start + i]);
        char c2 = toupper(s2.ptr[t2.start + i]);

        if (c1 != c2) {
            return c1 - c2;
        }
    }

    return (t1.sz > i) ? 1 : ((t2.sz > i) ? -1 : 0);
}


// Finds label in sorted symbol table. Returns NO_LABEL if not found
OptInt findSymbol(LC3_Unit *unit, SymbolTable symbols, Token tk, String str) {
    int low = 0, high = symbols.sz - 1, mid, res;

    // Perform a binary search
    while (low <= high) {
        mid = (low + high) / 2;

        res = tokenCaseCmp(
            tk, symbols.ptr[mid].loc.tk, 
            str, symbols.ptr[mid].loc.unit->buf.ptr[symbols.ptr[mid].loc.line]
        );

        if (res == 0) {
            break;
        } else if (res > 0) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    OptInt ret = {
        .value = symbols.ptr[mid].value,
        .set = !res,
    };

    return ret;
}



static pthread_mutex_t outputMutex = PTHREAD_MUTEX_INITIALIZER;

void LC3_BeginOutput() {
    pthread_mutex_lock(&outputMutex);
}

void LC3_FinishOutput() {
    pthread_mutex_unlock(&outputMutex);
}


// Compare function for sortSymbolTable
int symcmp(const void *sym1, const void* sym2)
{
    const Symbol *s1 = (Symbol *)sym1;
    const Symbol *s2 = (Symbol *)sym2;

    int tmp = tokenCaseCmp(
        s1->loc.tk, s2->loc.tk,
        s1->loc.unit->buf.ptr[s1->loc.line],
        s2->loc.unit->buf.ptr[s2->loc.line]
    );
    
    return tmp ? tmp : (s1->loc.line - s2->loc.line);
}


// Sorts all values in strarr
void sortSymbolTable(SymbolTable *strarr) {
    qsort(strarr->ptr, strarr->sz, sizeof(Symbol), symcmp);
}


LC3_Unit LC3_CreateUnit(LC3_Context *ctx, const char *filename) {
    LC3_Unit ret = {
        .filename = filename,
        .buf   = newStringArray(),
        .obj   = newObjectSectionArray(),
        .symb  = newSymbolTable(),
        .upper = newString(),
        .ctx   = ctx,
        .error = false,
    };

    return ret;
}


void LC3_DestroyUnit(LC3_Unit unit) {
    freeStringArray(unit.buf);
    freeObjectSectionArray(unit.obj);
    freeSymbolTable(unit.symb);
    free(unit.upper.ptr);
}


// Add string with some extra checks
void addFileLine(LC3_Unit *unit, String str) {
    addString(&unit->buf, str);

    if (str.sz >= TOKEN_MAX) {
        size_t line = unit->buf.sz - 1;
        Token  tk   = {0, 132};
        memcpy(unit->buf.ptr[line].ptr + 128, " ...\0", 5);
        unit->buf.ptr[line].sz = 132;
        LC3_TokenError(unit, line, tk, "line longer than maximum allowed length", LC3_ERR_SHOW_LINE);
    }
}


// Reads file into unit buffer
void readFile(LC3_Unit *unit) {
    int c;
    bool inComment = false;
    FILE *fp = fopen(unit->filename, "r");

    if (fp == NULL || ferror(fp)) {
        LC3_SimpleError(unit, "failed to open file %s\n", unit->filename);
        return;
    }

    String temp = newString();

    while ((c = getc(fp)) != EOF) {
        // Push string onto buf without '\n'
        if (c == '\n') {
            // Remove trailing whitespace
            for (inComment = false; temp.sz > 0 && temp.ptr[temp.sz - 1] == ' '; temp.sz--);
            addFileLine(unit, temp);
            temp = newString();
            continue;
        }
        // Mark string as comment
        else if (c == ';') {
            inComment = true;
        }
        // Don't read comment
        if (!inComment) {
            addchar(&temp, c);
        }
    }

    // Read last string if necessary
    if (temp.sz) {
        addFileLine(unit, temp);
    } else {
        free(temp.ptr);
    }

    fclose(fp);

#if (LC3_DEBUG)
    LC3_BeginOutput();

    printf("--------------------------------------------------------\n");
    printf("File buffer: (file \"%s\" : thread %ld)\n", unit->filename, pthread_self());
    printf("--------------------------------------------------------\n");
    for (size_t i = 0; i < unit->buf.sz; i++) {
        printf("%3ld|%s\n", i, unit->buf.ptr[i].ptr);
    }
    printf("--------------------------------------------------------\n");

    LC3_FinishOutput();
#endif
}


void objectify(LC3_Unit *unit) {
    Statement stmt;
    OptInt addr = {0, false};

    // Tokenize and create statements
    for (size_t i = 0; !unit->error && i < unit->buf.sz; i++) {
        String current = unit->buf.ptr[i];
        Token tkn = getToken(0, current);

        // No tokens in line
        if (!validToken(tkn, current)) {
            continue;
        }

        memset(&stmt, 0, sizeof(Statement));
        stmt.line = i;
        stmt.instr = getInstructionIndex(tkn, current);

        if (stmt.instr == NULL) {
            // Statement starts with a label
            stmt.label = tkn;
            addSymbol(unit, i, tkn, current, addr.value);

            // Token needs to be key
            switch (getTokenType(tkn, current)) {
                case TOKEN_PSEUD:
                    LC3_TokenError(unit, i, tkn, "invalid assembler directive", LC3_ERR_SHOW_LINE | LC3_ERR_SHOW_TK);
                    break;
                case TOKEN_NUM:
                    LC3_TokenError(unit, i, tkn, "label can't be number", LC3_ERR_SHOW_LINE | LC3_ERR_SHOW_TK);
                    break;
                case TOKEN_REG:
                    LC3_TokenError(unit, i, tkn, "label can't be register", LC3_ERR_SHOW_LINE | LC3_ERR_SHOW_TK);
                    break;
                default:
                    break;
            }

            // Check if there are other tokens
            tkn = getToken(tkn.start + tkn.sz, current);

            // Label-statement
            if (!validToken(tkn, current)) {
                stmt.type = STMT_LABEL;
                continue;
            }

            // This should be the actual instruction
            stmt.instr = getInstructionIndex(tkn, current);

            if (stmt.instr == NULL) {
                // Invalid instruction!
                LC3_TokenError(unit , i, tkn, "invalid instruction", LC3_ERR_SHOW_LINE | LC3_ERR_SHOW_TK);
                continue;
            }
        }

        stmt.type = (stmt.instr->instr < INSTR_AS_ADD) ? STMT_PSEUD : STMT_INSTR;
        stmt.orig = tkn;

        // Check if the amount of tokens is right
        for (uint8_t j = 0; j < stmt.instr->argc; j++) {
            tkn = getToken(tkn.start + tkn.sz, current);

            if (!validToken(tkn, current)) {
                LC3_TokenError(unit , i, tkn, "unexpected end of line", LC3_ERR_SHOW_LINE);
            }
            if ((getTokenType(tkn, current) & stmt.instr->argl[j]) == 0) {
                LC3_TokenError(unit , i, tkn, "unexpected token", LC3_ERR_SHOW_LINE | LC3_ERR_SHOW_TK);
            }
            stmt.args[j] = tkn;
        }

        // Check for too many tokens
        tkn = getToken(tkn.start + tkn.sz, current);

        if (validToken(tkn, current)) {
            LC3_TokenError(unit , i, tkn, "unexpected extra argument", LC3_ERR_SHOW_LINE | LC3_ERR_SHOW_TK);
        }

        // Finally, we can add the (validated) statement to the list
        interpretStatement(unit, stmt, &addr);
    }

    // Check for duplicate symbols
    sortSymbolTable(&unit->symb);

    for (size_t i = 1; i < unit->symb.sz; i++) {
        Symbol current = unit->symb.ptr[i];

        if (tokenCaseCmp(
                current.loc.tk,
                unit->symb.ptr[i - 1].loc.tk,
                unit->buf.ptr[current.loc.line],
                unit->buf.ptr[unit->symb.ptr[i - 1].loc.line]
            ) == 0) {
            LC3_TokenError(unit, current.loc.line, current.loc.tk, "redefinition of label", LC3_ERR_SHOW_LINE | LC3_ERR_SHOW_TK);
        }
    }

#if (LC3_DEBUG == true)
    LC3_BeginOutput();

    printf("--------------------------------------------------------\n");
    printf("Symbol table: (file \"%s\" : thread %ld)\n", unit->filename, pthread_self());
    printf("--------------------------------------------------------\n");


    for (size_t i = 0; i < unit->symb.sz; i++) {
        char *tks = tokenString(unit->symb.ptr[i].loc.tk, unit->buf.ptr[unit->symb.ptr[i].loc.line]);
        printf("%4X | %s\n", (uint16_t)unit->symb.ptr[i].value, tks);
        free(tks);
    }

    printf("--------------------------------------------------------\n");
    printf("Object table: (file \"%s\" : thread %ld)\n", unit->filename, pthread_self());
    printf("--------------------------------------------------------\n");

    for (int i = 0; !unit->error && i < unit->obj.ptr[0].sz; i++) {
        ObjectLine obj = unit->obj.ptr[0].ptr[i];

        char *tk = tokenString(obj.label.tk, unit->buf.ptr[obj.label.line]);
        char *db = tokenString(obj.debug.tk, unit->buf.ptr[obj.debug.line]);

        printf("%3d | 0x%04X [%s] \"%s\"\n", i, obj.instr, tk, db);

        free(tk);
        free(db);
    }

    LC3_FinishOutput();
#endif
}


bool isObjectFile(const char *filename) {
    char *ext = strchr(filename, '.');

    if (ext && strlen(ext) == 4 && strcmp(ext, ".obj") == 0) {
        FILE *fp = fopen(filename, "rb");
        char mgc[4];
        fread(&mgc, 1, 4, fp);
        fclose(fp);
        return (memcmp(mgc, MAGIC_NUM, 4) == 0);
    }

    return 0;
}


// First step of the assembly
void LC3_AssembleUnit(LC3_Unit *unit) {
    if (isObjectFile(unit->filename)) {
        LC3_ReadFromFile(unit);
    } else {
        // Read file contents into unit
        readFile(unit);
    
        // Convert contents into a series of statements (and validate those) - Also constructs symbol table
        if (!unit->error && (unit->ctx == NULL ? true : !unit->ctx->error)) {
            objectify(unit);
        }
    }
}


void *LC3_AssembleUnit_Threaded(void *unit) {
    LC3_AssembleUnit((LC3_Unit *)unit);
    return NULL;
}


void LC3_AssembleUnits(size_t unitCount, LC3_Unit *units) {
    pthread_t *threads = malloc(unitCount * sizeof(pthread_t));

    for (size_t i = 0; i < unitCount; i++) {
        pthread_create(&threads[i], NULL, LC3_AssembleUnit_Threaded, &units[i]);
    }

    for (size_t i = 0; i < unitCount; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
}


// Resolve symbols in a parsed unit
void resolveSymbols(LC3_Unit *unit, const SymbolTable *symbols, IntervalArray *sections) {
    for (size_t section = 0; section < unit->obj.sz; section++) {
        BufferSegment addr = {
            .line = 0,
            .tk   = {unit->obj.ptr[section].origin, unit->obj.ptr[section].origin},
            .unit = unit,
        };

        for (size_t line = 0; !unit->error && line < unit->obj.ptr[section].sz; line++, addr.tk.sz++) {
            ObjectLine *current = &unit->obj.ptr[section].ptr[line];

            if (current->label.tk.sz != 0) {
                OptInt label = findSymbol(unit, *symbols, current->label.tk, unit->buf.ptr[current->label.line]);

                if (!label.set) {
                    label.value = 0;
                    LC3_linkerError(unit, "unable to determine address for label", current->label.tk, current->label.line);
                    continue;
                }

                resolveInstruction(unit, current, addr.tk.sz, label.value);
                current->label.tk.sz = 0;
            }
        }

        if (addr.tk.start != addr.tk.sz) {
            addInterval(sections, addr);
        }
    }
}


// Second step - performs linking too
void LC3_LinkUnits(size_t unitCount, LC3_Unit *units) {
    // Construct the large symbol table
    size_t totalCount = 0;
    size_t totalSegments = 0;

    for (size_t i = 0; i < unitCount; i++) {
        totalCount += units[i].symb.sz;
        totalSegments += units[i].obj.sz;
    }

    SymbolTable combined = newSymbolTableCapacity(totalCount);

    for (size_t i = 0; i < unitCount; i++) {
        memcpy(combined.ptr + combined.sz, units[i].symb.ptr, units[i].symb.sz * sizeof(Symbol));
        combined.sz += units[i].symb.sz;
    }

    // Another redefinition check
    sortSymbolTable(&combined);

    for (size_t i = 1; i < combined.sz; i++) {
        Symbol current = combined.ptr[i];

        if (tokenCaseCmp(
                current.loc.tk,
                combined.ptr[i - 1].loc.tk,
                current.loc.unit->buf.ptr[current.loc.line],
                combined.ptr[i - 1].loc.unit->buf.ptr[combined.ptr[i - 1].loc.line]
            ) == 0) {
            LC3_linkerError(current.loc.unit, "redefinition of label", current.loc.tk, current.loc.line);
            LC3_linkerError(combined.ptr[i - 1].loc.unit, "first defined here", combined.ptr[i - 1].loc.tk, combined.ptr[i - 1].loc.line);
        }
    }


#if (LC3_DEBUG)
    LC3_BeginOutput();

    printf("--------------------------------------------------------\n");
    printf("Symbol table: (combined)\n");
    printf("--------------------------------------------------------\n");

    for (size_t i = 0; i < combined.sz; i++) {
        char *tks = tokenString(combined.ptr[i].loc.tk, combined.ptr[i].loc.unit->buf.ptr[combined.ptr[i].loc.line]);
        printf("%4X | %s (%d)\n", (uint16_t)combined.ptr[i].value, tks, combined.ptr[i].loc.tk.sz);
        free(tks);
    }

    LC3_FinishOutput();
#endif
    IntervalArray sections = newIntervalArray(totalSegments);

    for (size_t i = 0; i < unitCount; i++) {
        resolveSymbols(&units[i], &combined, &sections);
    }

    // Check if any sections overlap
    qsort(sections.ptr, sections.sz, sizeof(BufferSegment), intvcmp);

#if (LC3_DEBUG)
    for (size_t i = 0; i < sections.sz; i++) {
        printf("Interval [%04X %04X]\n", sections.ptr[i].tk.start, sections.ptr[i].tk.sz);
    }
#endif

    for (size_t i = 1; i < sections.sz; i++) {
        if (sections.ptr[i].tk.start <= sections.ptr[i - 1].tk.sz) {
            LC3_SimpleError(
                sections.ptr[i].unit, "\x1b[1m%s:\x1b[0m code overlap with \"%s\" at address x%04X\n", 
                sections.ptr[i].unit->filename, sections.ptr[i - 1].unit->filename, sections.ptr[i].tk.start);
        }
    }

    freeIntervalArray(sections);
    free(combined.ptr);
}


enum FileFlag {
    LC3_FILE_OBJ = 0x0001,
    LC3_FILE_EXC = 0x0002,
    LC3_FILE_DBG = 0x0004,
    
    // Meta
    LC3_FILE_HDR = 0x10000,
    LC3_FILE_SYM = 0x20000,
};


enum FileIndicator {
    LC3_INDICATOR_SYM = 'S',
    LC3_INDICATOR_ASM = 'A',
};


void writeObjectLine(LC3_Unit *unit, FILE *fp, ObjectLine obj, uint32_t flags) {
    fwrite(&obj.instr, 2, 1, fp);
    char terminator = '\0';

    if (flags & LC3_FILE_OBJ) {
        fwrite(unit->buf.ptr[obj.label.line].ptr + obj.label.tk.start, 1, obj.label.tk.sz, fp);
        fwrite(&terminator, 1, 1, fp);
    }

    if (flags & LC3_FILE_DBG) {
        fwrite(unit->buf.ptr[obj.debug.line].ptr + obj.debug.tk.start, 1, obj.debug.tk.sz, fp);
        fwrite(&terminator, 1, 1, fp);
    }
}


void writeToFile(LC3_Unit *unit, FILE *fp, uint32_t flags) {
    uint8_t indicator;

    if (unit->ctx && unit->ctx->storeDebug) {
        flags |= LC3_FILE_DBG;
    }

    if (flags & LC3_FILE_HDR) {
        fwrite(MAGIC_NUM, 1, 4, fp);
        fwrite((uint16_t *)&flags, 2, 1, fp);
    }

    if ((flags & LC3_FILE_SYM) && unit->symb.sz > 0) {
        indicator = LC3_INDICATOR_SYM;

        fwrite(&indicator, 1, 1, fp);
        fwrite((int *)&unit->symb.sz, 4, 1, fp);

        for (int i = 0; i < unit->symb.sz; i++) {
            Symbol current = unit->symb.ptr[i];
            int terminator = '\0';

            fwrite(&current.value, 2, 1, fp);
            fwrite(current.loc.unit->buf.ptr[current.loc.line].ptr + current.loc.tk.start, 1, current.loc.tk.sz, fp);
            fwrite(&terminator, 1, 1, fp);
        }
    }

    if (!(flags & (LC3_FILE_OBJ | LC3_FILE_EXC))) {
        return;
    }

    indicator = LC3_INDICATOR_ASM;

    for (int section = 0; section < unit->obj.sz; section++) {
        ObjectSection current = unit->obj.ptr[section];

        // Write the section header
        fwrite(&indicator, 1, 1, fp);
        fwrite(&current.origin, 2, 1, fp);
        fwrite(&current.sz, 2, 1, fp);

        for (int i = 0; i < current.sz; i++) {
            writeObjectLine(unit, fp, current.ptr[i], flags);
        }
    }
}


void LC3_WriteSymbolTable(LC3_Unit *unit, FILE *fp, bool header) {
    writeToFile(unit, fp, LC3_FILE_SYM | (LC3_FILE_HDR * (!!header)));
}


void LC3_WriteObject(LC3_Unit *unit, FILE *fp, bool header) {
    writeToFile(unit, fp, LC3_FILE_OBJ | (LC3_FILE_HDR * (!!header)) | LC3_FILE_SYM);
}


void LC3_WriteExecutable(size_t unitCount, LC3_Unit *units, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    uint32_t flags = LC3_FILE_HDR | LC3_FILE_EXC;

    for (int i = 0; i < unitCount; i++) {
        writeToFile(&units[i], fp,  flags);
        flags &= ~LC3_FILE_HDR;
    }

    fclose(fp);
}


// Check for reading
#define FREAD_C(ptr, sz, n, fp, ret) if ((fread(ptr, sz, n, fp)) != n) { printf("Failed to read %i elements\n", n); unit->error = true; fclose(fp); return ret; }


int readString(LC3_Unit *unit, String *str, FILE *fp) {
    char c;
    FREAD_C(&c, 1, 1, fp, 1);

    while (c != '\0') {
        addchar(str, c);
        FREAD_C(&c, 1, 1, fp, 1);
    }

    return 0;
}


int readObjectLine(LC3_Unit *unit, ObjectSection *section, FILE *fp, String *str, uint32_t flags) {
    ObjectLine current = {0};
    FREAD_C(&current.instr, 2, 1, fp, 1);

    if (flags & LC3_FILE_OBJ) {
        if (readString(unit, str, fp) != 0) {
            return 1;
        }

        current.label.unit  = unit;
        current.label.line  = unit->buf.sz;
        current.label.tk.sz = str->sz;

        if (current.label.tk.sz > 0) {
            addString(&unit->buf, *str);
            (*str) = newString();
        } else {
            clearString(str);
        }
    }

    if (flags & LC3_FILE_DBG) {
        if (readString(unit, str, fp) != 0) {
            return 1;
        }

        current.debug.unit  = unit;
        current.debug.line  = unit->buf.sz;
        current.debug.tk.sz = str->sz;

        if (current.debug.tk.sz > 0) {
            addString(&unit->buf, *str);
            (*str) = newString();
        } else {
            clearString(str);
        }
    }

    addObjectLine(section, current);
    return 0;
}


void LC3_ReadFromFile(LC3_Unit *unit) {
    FILE *fp = fopen(unit->filename, "rb");

    char mgc[4];
    FREAD_C(&mgc, 1, 4, fp,);

    uint16_t flags = 0;
    FREAD_C(&flags, 2, 1, fp,);
    uint8_t indicator = 0;

    while (fread(&indicator, 1, 1, fp) == 1) {
        if (indicator == LC3_INDICATOR_SYM) {
            uint32_t size = 0;
            FREAD_C(&size, 4, 1, fp,);

            for (int i = 0; i < size; i++) {
                String line = newString();
                Symbol symb = {0};
                FREAD_C(&symb.value, 2, 1, fp,);

                if (readString(unit, &line, fp) != 0) {
                    return;
                }

                symb.loc.unit  = unit;
                symb.loc.line  = unit->buf.sz;
                symb.loc.tk.sz = line.sz;

                addSymbolHelper(&unit->symb, symb);
                addString(&unit->buf, line);
            }

        } else if (indicator == LC3_INDICATOR_ASM) {
            addObjectSection(&unit->obj, newObjectSection());
            ObjectSection *section = &unit->obj.ptr[unit->obj.sz - 1];
            uint16_t size = 0;

            FREAD_C(&section->origin, 2, 1, fp,);
            FREAD_C(&size, 2, 1, fp,);

            String label = newString();

            for (int i = 0; i < size; i++) {
                if (readObjectLine(unit, section, fp, &label, flags) != 0) {
                    free(label.ptr);
                    return;
                }
            }

            free(label.ptr);

        } else {
            unit->error = true;
            fclose(fp);
            return;
        }
    }

    fclose(fp);

#if (LC3_DEBUG)
    LC3_BeginOutput();

    printf("--------------------------------------------------------\n");
    printf("Symbol table: (file \"%s\"\n", unit->filename);
    printf("--------------------------------------------------------\n");


    for (size_t i = 0; i < unit->symb.sz; i++) {
        char *tks = tokenString(unit->symb.ptr[i].loc.tk, unit->buf.ptr[unit->symb.ptr[i].loc.line]);
        printf("%4X | %s\n", (uint16_t)unit->symb.ptr[i].value, tks);
        free(tks);
    }

    printf("--------------------------------------------------------\n");
    printf("Object table: (file \"%s\"\n", unit->filename);
    printf("--------------------------------------------------------\n");

    for (int i = 0; !unit->error && i < unit->obj.ptr[0].sz; i++) {
        ObjectLine obj = unit->obj.ptr[0].ptr[i];

        char *tk = tokenString(obj.label.tk, unit->buf.ptr[obj.label.line]);
        char *db = tokenString(obj.debug.tk, unit->buf.ptr[obj.label.line + 1]);

        printf("%3d | 0x%04X [%s] \"%s\"\n", i, obj.instr, tk, db);

        free(tk);
        free(db);
    }

    LC3_FinishOutput();
#endif

    return;
}
