
lc3a: main.c lc3_asm.c lc3_cmd.c lc3_err.c lc3_tk.c lc3_instr.c lib/cmdarg.c
	gcc -std=c99 -o $@ $^ -Wall -pedantic -g

