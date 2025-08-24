#pragma once
#include "lc3_asm.h"


typedef enum LC3_ErrorConfig {
    LC3_ERR_SHOW_TK   = 0x01,
    LC3_ERR_SHOW_LINE = 0x02,
} LC3_ErrorConfig;


#define LC3_SimpleError(unit, ...) \
    if (unit != NULL) {((LC3_Unit *)unit)->error = true; if (((LC3_Unit *)unit)->ctx != NULL) {((LC3_Unit *)unit)->ctx->error = true;}}\
    printf("\x1b[1;31merror:\x1b[0m ");\
    printf(__VA_ARGS__);\


void LC3_linkerError(LC3_Unit *unit, const char *msg, Token tk, size_t line);
void LC3_TokenError(LC3_Unit *unit, size_t line, Token tk, const char *msg, LC3_ErrorConfig flags);
