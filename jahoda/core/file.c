#include "file.h"

str file_load(arena arena, strv filename)
{
	str out = {0};
	FILE *cfile = NULL;

	scratch scratch = scratch_begin(arena);

	str filename_nt = str_from_view_nt(arena, filename);		
	cfile = fopen(filename_nt.data, "rb");	

	scratch_end(scratch);

	if (!cfile)
	{
		errl(could_not_open_file, "%.*s", strv_fmt(&filename));
	}

	fseek(cfile, 0, SEEK_END);
	da_resize(arena, &out, (uz)ftell(cfile));
	rewind(cfile);

	fread(out.data, 1, out.occupied, cfile);
	
	fclose(cfile);

	return out;
}