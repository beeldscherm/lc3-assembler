#include <stdio.h>
#include "lc3_cmd.h"
#include "lc3_asm.h"
#include "lib/cmdarg.h"


enum LC3_CommandLineFlag {
    LC3_CMD_FLAG_HELP    = 0x01,
    LC3_CMD_FLAG_OBJ     = 0x02,
    LC3_CMD_FLAG_SYMB    = 0x04,
    LC3_CMD_FLAG_DEBUG   = 0x08,
    LC3_CMD_FLAG_INDENT  = 0x08,
};


static void showHelpMessage() {
    printf("Usage: lc3a [options] file...\n");
    printf("Options:\n");
    printf("  --help                     Display this information.\n");
    printf("  -a                         Assemble, but do not link.\n");
    printf("  -s                         Only output symbol table.\n");
    printf("  -g                         Embed original code (excluding indentation) in output file.\n");
    printf("  -G                         Embed original code (including indentation) in output file.\n");
    printf("  -o <file>                  Place the output into <file>.\n");
}


static char objFilename[64];


const char *getObjectFilename(const char *filename) {
    int i;

    for (i = 0; i < 60 && filename[i] != '\0' && filename[i] != '.'; i++) {
        objFilename[i] = filename[i];
    }

    memcpy(&objFilename[i], ".obj\0", 5);
    return objFilename;
}


int LC3_AssemblyCommand(int argc, char **argv) {
    ca_config *argConfig = ca_alloc_config();

    ca_bind_flag(argConfig, "--help", LC3_CMD_FLAG_HELP);
    ca_bind_flag(argConfig, "-a", LC3_CMD_FLAG_OBJ);
    ca_bind_flag(argConfig, "-s", LC3_CMD_FLAG_SYMB);
    ca_bind_flag(argConfig, "-g", LC3_CMD_FLAG_DEBUG);
    ca_bind_flag(argConfig, "-G", LC3_CMD_FLAG_DEBUG | LC3_CMD_FLAG_INDENT);

    ca_set_hasv(argConfig, "-o");

    ca_info *argInfo = ca_parse(argConfig, argc - 1, argv + 1);
    uint64_t flags = ca_flags(argInfo);
    ca_free_config(argConfig);

    if (flags & LC3_CMD_FLAG_HELP) {
        showHelpMessage();
        ca_free_info(argInfo);
        return 0;
    }

    size_t inputCount = 0;
    const char **inputs = ca_literals(argInfo, &inputCount);

    // Pre-checks
    if (inputCount == 0) {
        printf("\x1b[1;31mfatal error:\x1b[0m no input files\nassembly terminated.\n");
        ca_free_info(argInfo);
        return 1;
    }

    LC3_Context ctx = {
        .output      = ca_flag_value(argInfo, "-o"),
        .storeDebug  = (flags & LC3_CMD_FLAG_DEBUG),
        .storeIndent = (flags & LC3_CMD_FLAG_INDENT),
        .error       = false,
    };

    if (inputCount > 1 && ctx.output != NULL && (flags & LC3_CMD_FLAG_OBJ)) {
        printf("\x1b[1;31mfatal error:\x1b[0m cannot specify '-o' with '-a' with multiple files\nassembly terminated.\n");
        ca_free_info(argInfo);
        return 1;
    }

    // Assembly process
    LC3_Unit *units = malloc(inputCount * sizeof(LC3_Unit));

    for (size_t i = 0; i < inputCount; i++) {
        units[i] = LC3_CreateUnit(&ctx, inputs[i]);
    }

    LC3_AssembleUnits(inputCount, units);

    if (!ctx.error && !(flags & LC3_CMD_FLAG_OBJ)) {
        LC3_LinkUnits(inputCount, units);
    }

    // Writing output
    if (!ctx.error && (flags & LC3_CMD_FLAG_OBJ)) {
        for (size_t i = 0; i < inputCount; i++) {
            FILE *fp = fopen((inputCount > 1 || ctx.output == NULL) ? getObjectFilename(units[i].filename) : ctx.output, "wb");
            LC3_WriteObject(&units[i], fp, true);
            fclose(fp);
        }
    } else if (!ctx.error && (flags & LC3_CMD_FLAG_SYMB)) {
        FILE *fp = fopen((ctx.output == NULL) ? "out.symb" : ctx.output, "wb");

        for (size_t i = 0; i < inputCount; i++) {
            LC3_WriteSymbolTable(&units[i], fp, (i == 0));
        }
        
        fclose(fp);
    } else if (!ctx.error) {
        LC3_WriteExecutable(inputCount, units, (ctx.output == NULL) ? "out.lc3" : ctx.output);
    }

    // Cleanup
    for (size_t i = 0; i < inputCount; i++) {
        LC3_DestroyUnit(units[i]);
    }

    free(units);
    ca_free_info(argInfo);
    return 0;
}
