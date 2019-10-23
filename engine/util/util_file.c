#include <stdio.h>
#include <stdlib.h>
#include "util.h"


void util_load_whole_file(Interface *func, const char *filename, void **data, size_t *size)
{
    FILE *fp = func->fopen(filename, "r");
    
    func->fseek(fp, 0, SEEK_END);
    *size = func->ftell(fp);
    func->fseek(fp, 0, SEEK_SET);
    
    *data = func->malloc(*size);
    
    func->fread(*data, *size, 1, fp);
    
    func->fclose(fp);
    
    return;
}
