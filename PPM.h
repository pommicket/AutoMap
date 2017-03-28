#include <string.h>
#include "FileIO.h"
#include "Constants.h"

typedef struct
{
	double r; // 0 - 1
	double g; // 0 - 1
	double b; // 0 - 1
} color;

typedef struct
{
	//General settings
    double** map;
    double** newMap;
    char* contents;
	int width;
    int height;
	int numFrames;
	int frameNumber;
	int mode;
	//BINARY settings
    double expandCoast;
    double retractCoast;
    double island;
    double lake;
	int expandCoastDecay;
	//GRADIENT settings
	double base; //Base positive random number probability; default: 0.5
	double factor; //Positive random number factor; default: 0.4
	double change; //Change multiplier; default: 0.1
	//EMPIRES settings
	color* empireColors;
    int numEmpires;
	int showMapGen;
} map_state;

void overlay(double** dest, double** src, int src_x, int src_y, int src_width, int src_height)
{
	int x, y;
	for (y = src_y; y < src_y + src_height; y++)
		for (x = src_x; x < src_x + src_width; x++)
			dest[y][x] = src[y-src_y][x-src_x];
}

double** getLetter(char letter)
{
	char* filePath;
	if (letter == ' ')
	{
		filePath = "images/space.ppm";
	}
	else
	{
		filePath = malloc(16);
		sprintf(filePath, "images/%c.ppm", letter);
	}
	char* data = readFile(filePath);
	double** img = malloc(LETTER_HEIGHT * sizeof(double*));
	int i;
	for (i = 0; i < LETTER_HEIGHT; i++)
		img[i] = malloc(LETTER_WIDTH * sizeof(double));
	for (i = 0; i < LETTER_HEIGHT * LETTER_WIDTH; i++)
		img[i / LETTER_HEIGHT][i % LETTER_HEIGHT] = (double)((unsigned char) data[i * 3 + PPM_START] / 255 + BLACK);

	return img;
}


void writeChar(double** dest, char letter, int x, int y)
{
	double** l = getLetter(letter);
	overlay(dest, l, x, y, LETTER_WIDTH, LETTER_HEIGHT);
}

void writeString(double** dest, char* s, int x, int y)
{
	int i;
	for (i = 0; i < strlen(s); i++)
	{
		writeChar(dest, s[i], x+LETTER_WIDTH*i, y);
	}
}

// Convert a map to a .ppm file
char* writePPM(map_state* ms, char* start, int binaryModeOverride)
{
	if (ms->mode < 2 && !binaryModeOverride)
	{
		int x, y;
		char r, g, b;
		int string_pos = strlen(start);
		double** map = ms->map;

		memcpy(ms->contents, start, strlen(start));

		for (y = 0; y < ms->height; y++)
		{
			for (x = 0; x < ms->width; x++)
			{
				if (map[y][x] == -2)
				{
					ms->contents[string_pos++] = 1;
					ms->contents[string_pos++] = 1;
					ms->contents[string_pos++] = 1;
				}
				else if (map[y][x] == -1)
				{
					ms->contents[string_pos++] = 255;
					ms->contents[string_pos++] = 255;
					ms->contents[string_pos++] = 255;
				}
				else
				{
					r = (char)(LANDR * map[y][x] + WATERR * (1-map[y][x]));
					g = (char)((ms->mode == GRADIENT ? 255 : LANDG) * map[y][x] + WATERG * (1-map[y][x])); // For gradient mode, LANDG should be 255
					b = (char)(LANDB * map[y][x] + WATERB * (1-map[y][x]));
					ms->contents[string_pos++] = r == 0 ? 1: r;
					ms->contents[string_pos++] = g == 0 ? 1 : g;
					ms->contents[string_pos++] = b == 0 ? 1 : b;
				}
			}
		}
		ms->contents[string_pos] = 0;

		return ms->contents;
	}
	else
	{
		int x, y;
		color col;
		char r, g, b;
		int string_pos = strlen(start);

		memcpy(ms->contents, start, strlen(start));
		for (y = 0; y < ms->height; y++)
		{
			for (x = 0; x < ms->width; x++)
			{
				col = ms->empireColors[(int)ms->newMap[y][x]];

				r = (char)(col.r * 255);
				g = (char)(col.g * 255);
				b = (char)(col.b * 255);
				ms->contents[string_pos++] = r == 0 ? 1 : r;
				ms->contents[string_pos++] = g == 0 ? 1 : g;
				ms->contents[string_pos++] = b == 0 ? 1 : b;
			}
		}
		ms->contents[string_pos] = 0;

		return ms->contents;
	}
}

