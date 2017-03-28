#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void writeFile(char* filename, char* data)
{
    FILE* fp = fopen(filename, "w");
    fwrite(data, 1, strlen(data), fp);
    fclose(fp);
}

int fileSize(FILE* fp)
{
	
	fseek(fp, 0, SEEK_END);
	int ret = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	return ret;
}

char* readFile(char* filename)
{
	FILE* fp = fopen(filename, "r");
	int sz = fileSize(fp);
	char* buffer = malloc(sz);
	fread(buffer, sz + 1, 1, fp);
	return buffer;
}