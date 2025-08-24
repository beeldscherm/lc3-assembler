#include "lc3_err.h"
#include "lc3_tk.h"


static void setError(LC3_Unit *unit) {
    unit->error = true;

    if (unit->ctx != NULL) {
        unit->ctx->error = true;
    }
}


void LC3_linkerError(LC3_Unit *unit, const char *msg, Token tk, size_t line) {
    String str = unit->buf.ptr[line];
    char *tkString = tokenString(tk, str);
    setError(unit);

    LC3_BeginOutput();

    printf(tk.sz != 0 ? 
        "\x1b[1m%s: \x1b[1;31merror:\x1b[0m %s \"\x1b[1m%s\x1b[0m\"\n" :
        "\x1b[1m%s: \x1b[1;31merror:\x1b[0m %s\n",
        unit->filename, msg, (tk.sz != 0) ? tkString : ""
    );

    LC3_FinishOutput();
    free(tkString);
}


void LC3_TokenError(LC3_Unit *unit, size_t line, Token tk, const char *msg, LC3_ErrorConfig flags) {
    String str = unit->buf.ptr[line];
    char *tkString = tokenString(tk, str);
    setError(unit);

    LC3_BeginOutput();
    printf((flags & LC3_ERR_SHOW_TK) ? 
        "\n\x1b[1m%s:%ld:%hd: \x1b[1;31merror:\x1b[0m %s \"\x1b[1m%s\x1b[0m\"\n" :
        "\n\x1b[1m%s:%ld:%hd: \x1b[1;31merror:\x1b[0m %s\n",
        unit->filename, line, tk.start, msg, (flags & LC3_ERR_SHOW_TK) ? tkString : ""
    );

    if (!(flags & LC3_ERR_SHOW_LINE)) {
        LC3_FinishOutput();
        free(tkString);
        return;
    }

    uint8_t digitCount;
    size_t copy;
    for (digitCount = 1, copy = line; (copy /= 10) > 0; digitCount++);

    // Print string with incorrect token highlighted
    printf("%ld | ", line);
    for (copy = 0; copy < tk.start; copy++) {
        putchar(str.ptr[copy]);
    }
    printf("\x1b[1;31m");
    for (; copy < (tk.start + tk.sz); copy++) {
        putchar(str.ptr[copy]);
    }
    printf("\x1b[0m");
    for (; copy < str.sz; copy++) {
        putchar(str.ptr[copy]);
    }
    putchar('\n');

    // Arrow under incorrect token
    for (; digitCount > 0; digitCount--) {
        putchar(' ');
    }
    printf(" | ");
    for (copy = tk.start; copy > 0; copy--) {
        putchar(' ');
    }
    printf("\x1b[1;31m^");
    for (copy = (tk.sz) ? tk.sz - 1 : 0; copy > 0; copy--) {
        putchar('~');
    }

    printf("\x1b[0m\n");

    LC3_FinishOutput();
    free(tkString);
}
