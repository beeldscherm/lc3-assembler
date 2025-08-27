
lc3a: main.c lc3/lc3_asm.c lc3/lc3_cmd.c lc3/lc3_err.c lc3/lc3_tk.c lc3/lc3_instr.c lc3/lib/cmdarg.c
	gcc -std=c99 -o $@ $^ -Wall -pedantic -g

