#include "file.h"

str8 file_load(arena *arena, str8 path)
{		
	str8 out;

	file_scope(cfile, path.data, "rb")
	{
		if (!cfile)
		{
			errl(could_not_open_file, str8_fmt, str8_arg(path));
		}
	
		fseek(cfile, 0, SEEK_END);
		out = str8_make_empty(arena, ftell(cfile));	
		rewind(cfile);
	
		fread(out.data, 1, out.size, cfile);		
	}	

	return out;
}

str8 *file_try_load(arena *arena, str8 path)
{		
	str8 *out;

	file_scope(cfile, path.data, "rb")
	{
		if (!cfile)
		{
			return NULL;
		}
		
		out = arena_ppush(arena, str8*);

		fseek(cfile, 0, SEEK_END);
		*out = str8_make_empty(arena, ftell(cfile));	
		rewind(cfile);
	
		fread(out->data, 1, out->size, cfile);		
	}	

	return out;
}