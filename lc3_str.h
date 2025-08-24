#pragma once
#include "lib/va_template.h"

// String-related stuff
vaTypedef(char, String);
vaTypedef(String, StringArray);


// String functions
vaAllocFunctionDefine(String, newString);
vaAppendFunctionDefine(String, char, addchar);
