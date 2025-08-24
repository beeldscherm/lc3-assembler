/* 
 * author: https://github.com/beeldscherm
 * file:   cl3_tk.c
 * date:   24/08/2025
 */

#include "lc3_tk.h"
#include <ctype.h>
#include <string.h>


bool isTokenChar(char c) {
    // This might be overkill but you know I've heard that branching is bad
    static const bool VALID_CHAR_MAP[256] = {
        false, true , true , true , true , true , true , true , true , false, false, true , true , true , true , true , 
        true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , 
        false, true , true , true , true , true , true , true , true , true , true , true , false, true , true , true , 
        true , true , true , true , true , true , true , true , true , true , false, false, true , true , true , true , 
        true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , 
        true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , 
        true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , 
        true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , 
        true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , 
        true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , 
        true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , 
        true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , 
        true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , 
        true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , 
        true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , 
        true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , 
    };

    return VALID_CHAR_MAP[(int)c];
}


OptInt toBase(uint8_t digit, uint8_t base) {
    // I LOVE PRECALCULATION
    static const int8_t BASE_CONVERT_MAP[256] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1, 
        -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 
        25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1, 
        -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 
        25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    };

    OptInt ret = {
        .value = BASE_CONVERT_MAP[digit],
        .set   = BASE_CONVERT_MAP[digit] < base,
    };

    return ret;
}


// Turns a string into a number if possible
OptInt getNumber(Token tk, String str) {
    uint8_t base = 10;
    size_t idx = tk.start;
    int8_t factor = 1;
    OptInt ret = {0, false};

    // Deduce base from first character
    if (str.ptr[idx] == '#') {
        idx++;
    } else if (str.ptr[idx] == 'X' || str.ptr[idx] == 'x') {
        idx++;
        base = 16;
    } else if (str.ptr[idx] == 'B' || str.ptr[idx] == 'b') {
        idx++;
        base = 2;
    } else if (!isdigit(str.ptr[idx])) {
        return ret;
    }

    // Deal with negative numbers
    if ((tk.start + tk.sz) > idx && str.ptr[idx] == '-') {
        factor = -1;
        idx++;
    }
    if ((tk.start + tk.sz) == idx) {
        return ret;
    }

    ret.set = true;

    // Calculate number value
    for (size_t i = idx; ret.set && i < (tk.start + tk.sz); i++) {
        OptInt digit = toBase(str.ptr[i], base);
        ret.set = digit.set;
        ret.value *= base;
        ret.value += digit.value;
    }

    ret.value *= factor;
    return ret;
}


// Gets the first token from character n in string
Token getToken(size_t start, String str) {
    Token ret;

    // Skip leading invalid characters
    for (ret.start = start; ret.start < str.sz && !isTokenChar(str.ptr[ret.start]); ret.start++);

    bool isString = (ret.start < str.sz && str.ptr[ret.start] == '"');

    // Token lasts until first invalid character (unless token is a string literal)
    for (ret.sz = 0; ret.start + ret.sz < str.sz; ret.sz++) {
        char current = str.ptr[ret.start + ret.sz];

        // Special logic for strings
        if (isString) {
            if (current == '"' && ret.sz > 0 && str.ptr[ret.start + ret.sz - 1] != '\\') {
                ret.sz++;
                break;
            }
        } else if (!isTokenChar(current)) {
            break;
        }
    }

    return ret;
}


bool validToken(Token tk, String str) {
    return tk.sz > 0 && (tk.start + tk.sz) <= str.sz;
}


char *tokenString(Token tk, String str) {
    if (!validToken(tk, str)) {
        return calloc(1, 1);
    }

    char *c_str = malloc(tk.sz + 1);
    memcpy(c_str, str.ptr + tk.start, tk.sz);
    c_str[tk.sz] = '\0';

    return c_str;
}


// Determine TokenType from string
TokenType getTokenType(Token tk, String str) {
    // We can learn a lot by examining the first character
    switch (str.ptr[tk.start]) {
        case '.': 
            return TOKEN_PSEUD;
        case '"':
            return TOKEN_STR;
        case 'R':
        case 'r':
            if (tk.sz == 2 && str.ptr[tk.start + 1] <= '7' && str.ptr[tk.start + 1] >= '0') {
                return TOKEN_REG;
            }
            break;
    }

    // What if it's a number though
    if (getNumber(tk, str).set) {
        return TOKEN_NUM;
    }

    return TOKEN_KEY;
}


// Turns 2-character string into register code with optional shift left
uint16_t getRegister(const char *reg, uint8_t shl) {
    return (reg[1] - '0') << shl;
}


// Gets codes from BR instruction
uint16_t getCC(const char *instr, size_t len) {
    switch (len) {
        case 3:
            return (toupper(instr[2]) == 'N') ? 0x0800 : ((toupper(instr[2]) == 'Z') ? 0x0400 : 0x0200);
        case 4:
            return (toupper(instr[2]) == 'Z') ? 0x0600 : ((toupper(instr[3]) == 'Z') ? 0x0C00 : 0x0A00);
        case 5:
            return 0x0E00;
    }
    return 0x0000;
}


// Checks for escape characters in strings
bool isEscaped(char c) {
    return c == '\\' || c == 'n' || c == 'r' || c == 't' || c == '0' || c == '"';
}


// Deal with escape codes
char getEscaped(char c) {
    switch (c) {
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case '0': return '\0';
        // There are probably more escape codes
        default: return c;
    }
}


// Turn string literal into literal string literal
String generateLiteral(Token tk, String str) {
    String ret = newString();

    // Loop while properly handling escaped characters
    for (int i = tk.start + 1; i < (tk.start + tk.sz) && str.ptr[i] != '"'; i++) {
        char c1 = str.ptr[i];
        char c2 = str.ptr[i + 1];

        if ((c1 == '\\') && (isEscaped(c2))) {
            addchar(&ret, getEscaped(c2));
            i++;
        } else {
            addchar(&ret, c1);
        }
    }
    
    return ret;
}
