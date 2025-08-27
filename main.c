/* 
 * author: https://github.com/beeldscherm
 * file:   main.c
 * date:   24/08/2025
 */

/* 
 * Description: 
 * Delegates command-line arguments to the LC3 command interpreter
 */

#include "lc3/lc3_cmd.h"


int main(int argc, char **argv) {
    return LC3_AssemblyCommand(argc, argv);;
}

