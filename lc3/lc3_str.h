/* 
 * author: https://github.com/beeldscherm
 * file:   lc3_str.h
 * date:   24/08/2025
 */

/* 
 * Description: 
 * String functions
 */

#pragma once
#include "lib/va_template.h"

// String-related stuff
vaTypedef(char, String);
vaTypedef(String, StringArray);

// String functions
vaAllocFunctionDefine(String, newString);
vaAppendFunctionDefine(String, char, addchar);
