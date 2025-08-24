/* 
 * author: https://github.com/beeldscherm
 * file:   cl3_tk.h
 * date:   24/08/2025
 */

/* 
 * Description: 
 * Handles token-related tasks for the LC3 assembler
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lc3_str.h"
#include "lib/optional.h"


typedef uint16_t token_t;

// Stores the different types of tokens in LC3 assembly
typedef enum TokenType {
    TOKEN_UNKNOWN = 0x01, // Incorrect or empty token
    TOKEN_PSEUD   = 0x02, // Pseudo-operator
    TOKEN_NUM     = 0x04, // Number token
    TOKEN_KEY     = 0x08, // A word, can be instruction/label/string
    TOKEN_REG     = 0x10, // Register (R0 .. R7)
    TOKEN_STR     = 0x20  // String literal
} TokenType;


// Token specifies a range inside a string
typedef struct Token {
    token_t start, sz;
} Token;


// At least it works
#include "lc3_asm.h"


// Translates token into optional number
OptInt getNumber(Token tk, String str);

// Gets next token from string, starting from index start
Token getToken(size_t start, String str);

// Checks if token is valid
bool validToken(Token tk, String str);

// Copies string segment specified by token range into new C-string
char *tokenString(Token tk, String str);

// Deduces token type
TokenType getTokenType(Token tk, String str);

// Deduces register code from string, with optional shift-left
uint16_t getRegister(const char *reg, uint8_t shl);

// Deduces condition-codes from BR(n?z?p?) instruction
uint16_t getCC(const char *instr, size_t len);

// Transforms string-literal token into actual string-literal with valid escape-codes
String generateLiteral(Token tk, String str);
