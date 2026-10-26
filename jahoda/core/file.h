#ifndef jahoda_file
#define jahoda_file

#include "utils.h"
#include "str.h"

#define file_scope(var, path, mode)\
	for(FILE *var = fopen(path, mode); var; fclose(var), var = NULL)

str8 file_load(arena *arena, str8 path);
str8 *file_try_load(arena *arena, str8 path);

#endif