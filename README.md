# LC3TUI

LC3 assembler and linker


## Getting Started

### Dependencies

* Linux (untested on Windows/MacOS)
* C compiler


### Installing

* clone the repo
```
https://github.com/beeldscherm/lc3-assembler
```

* Either run the makefile or compile the C files using your preferred compiler
```
make
```


### Running (examples)

Assume I have the files `foo.asm` and `bar.asm`, and `foo.asm` needs symbols from `bar.asm`. I can:

Assemble and link both files into a single LC3 executable (foobar.lc3):
```
./lc3a -o foobar.lc3 1.asm 2.asm
```

First assemble the files, and then link them into an LC3 executable:
```
./lc3a -a foo.asm bar.asm
./lc3a -o foobar.lc3 foo.obj bar.obj
```

Assemble one of the files, then assemble and link them, with simple debug info:
```
./lc3a -a -g foo.asm
./lc3a -g -o foobar.lc3 foo.obj bar.asm
```


### Help

For info on possible flags, run the executable with the `--help` flag.

If you do not have time for that, here is the output of that command:
```
Usage: lc3a [options] file...
Options:
  --help                     Display this information.
  -a                         Assemble, but do not link.
  -s                         Only output symbol table.
  -g                         Embed original code (excluding indentation) in output file.
  -G                         Embed original code (including indentation) in output file.
  -o <file>                  Place the output into <file>.
```


## Problems

Please create an issue if you encounter any bugs!


## TODO

- [ ] Output to different LC3 object file formats
- [ ] More extensive testing
- [ ] Code cleanup
