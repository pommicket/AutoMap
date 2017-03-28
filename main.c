#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>
#include "PPM.h" //.ppm file IO

double rand01()
{
    return (double)rand() / RAND_MAX;
}

color* randColor()
{
	color* c = malloc(sizeof(color));
	c->r = rand01();
	c->g = rand01();
	c->b = rand01();
	return c;
}

color* mkColor(double r, double g, double b)
{
	color* c = malloc(sizeof(color));
	c->r = r;
	c->g = g;
	c->b = b;
	return c;
}

int randrange(int start, int end)
{
	return (int) (rand01() * (end - start) + start);
}



double changeScale(double val, double oldMin, double oldMax, double newMin, double newMax)
{
	double oldScale = oldMax - oldMin;
	double newScale = newMax - newMin;
	double normalized = (val - oldMin) / oldScale;
	return normalized * newScale + newMin;
}

long long int year(map_state* ms)
{
	int n = ms->numFrames;
	int t = ms->frameNumber;
	if (ms->showMapGen)
	{
		if (t < n)
		{
			return (long long int) changeScale(t, 0, n, -1000000000, -1000);
		}
		else
		{
			return (long long int) changeScale(t, n, 2*n, -2000, 1500);
		}
	}
	else
	{
		return (long long int) changeScale(t, 0, n, -1000, 1000);
	}

}

char* showYear(map_state* ms)
{
	char* val = malloc(32);
	long long int y = year(ms);
	if (y < 0)
		sprintf(val, "year %lld bce", -y);
	else if (y > 0)
		sprintf(val, "year %lld ce", y);
	else
		return "year 1 ce"; //0 CE wasn't a year.
	return val;
}

double clamp(double value, double mn, double mx)
{
	if (value < mn)
		return mn;
	if (value > mx)
		return mx;
	return value;
}

double** blankMap(int width, int height)
{
    int i, j;

    double** map = malloc(height*sizeof(double*));
    for (i = 0; i < height; i++)
    {
        map[i] = malloc(width*sizeof(double));
        for (j = 0; j < width; j++)
            map[i][j] = 0;
    }

    return map;
}


void iteration(map_state* ms)
{
    double** map = ms->map;
	int y, x, selection; //isLand NTBCW island
	int i, j, importance, currentMax;
    double landNeighbours, neighbourWeights, r;
    double landScore, waterScore;

    for (y = 0; y < ms->height; y++)
    {
        for (x = 0; x < ms->width; x++)
        {
			switch (ms->mode)
			{
			case BINARY:
				landNeighbours = 0;
				if (y > 0)
				{
					landNeighbours += map[y-1][x-1] + map[y-1][x] + map[y-1][x+1];
				}
				if (x > 0)
					landNeighbours += map[y][x-1];
				if (x < ms->width-1)
					landNeighbours += map[y][x+1];
				if (y < ms->height-1)
				{
					landNeighbours += map[y+1][x-1] + map[y+1][x] + map[y+1][x+1];
				}
				landScore  = (double)landNeighbours/8;
				waterScore = 1 - landScore;
				waterScore = waterScore < 0.01 ? 0.1 : waterScore;
				if (map[y][x])
				{
					if (landNeighbours == 8)
					{
						ms->newMap[y][x] = rand01() > ms->lake;
					}
					else
					{
						ms->newMap[y][x] = rand01() > waterScore * ms->retractCoast;
					}
				}
				else
				{
					if (landNeighbours == 0)
					{
						ms->newMap[y][x] = rand01() < ms->island;
					}
					else
					{
						ms->newMap[y][x] = rand01() < landScore * ms->expandCoast;
					}
				}

				break;
			case GRADIENT:
				neighbourWeights = 3;
				landNeighbours = 3*map[y][x];
				if (y > 0)
				{
					landNeighbours += map[y-1][x-1] + 2*map[y-1][x] + map[y-1][x+1];
					neighbourWeights += 4;
				}
				if (x > 0)
				{
					landNeighbours += 2*map[y][x-1];
					neighbourWeights += 2;
				}
				if (x < ms->width-1)
				{
					landNeighbours += 2*map[y][x+1];
					neighbourWeights += 2;
				}
				if (y < ms->height-1)
				{
					landNeighbours += map[y+1][x-1] + 2*map[y+1][x] + map[y+1][x+1];
					neighbourWeights += 4;
				}
				r = rand01();
				ms->newMap[y][x] = clamp(landNeighbours / neighbourWeights + r * (rand01() < ms->base + ms->factor / ms->frameNumber ? 1 : -1) * ms->change, 0, 1);
				break;
			default: //EMPIRES

				if (ms->map[y][x] == WATER)
				{
					ms->newMap[y][x] = WATER;
					break;
				}

				currentMax = 15;
				selection = randrange(0, currentMax);
				for (i = -1; i <= 1; i++)
				{
					for (j = -1; j <= 1; j++)
					{
						if (y+i < 0 || x+j < 0 || y+i >= ms->height || x+j >= ms->width)
							continue;
						importance = 3 - (abs(i) + abs(j));
						currentMax -= importance;
						if ((selection -= importance) < 0)
						{

							if (map[y+i][x+j] <= UNOCCUPIED)
							{
								selection = randrange(0, currentMax);
							}
							else
							{
								ms->newMap[y][x] = ms->map[y+i][x+j];
								goto done; // Sorry!
							}
						}
					}
				}
				ms->newMap[y][x] = UNOCCUPIED;

				done:
				break;
			}
		}
	}

    if (ms->mode == BINARY)
	{
		// As the frame_number approaches infinity, expandCoast -> retractCoast
		if (ms->frameNumber > 0 && ms->frameNumber % ms->expandCoastDecay == 0)
			ms->expandCoast = (ms->retractCoast + ms->expandCoast) / 2;
	}
    double** tmp = ms->newMap;
    ms->newMap = ms->map;
    ms->map = tmp;
}


int getInt(char* msg, int default_value, int ask)
{
	//Get int from user. If it is invalid, or ask = 0, return the default_value.
	if (!ask)
		return default_value;
    char* i;
	char* ptr;
	int val;
	i = malloc(16);
    printf(msg);
    fgets(i, 16, stdin);
	val = (int)strtol(i, &ptr, 10);
	if (strlen(i) <= 1 || strlen(ptr) > 1)
        return default_value;
    else
        return val;
}


double getDouble(char* msg, double default_value, int ask)
{
	//Get double from user. If it is invalid, or ask = 0, return the default_value.
	if (!ask)
		return default_value;
    char* i;
	char* ptr;
    double val;
    i = malloc(16);
    printf(msg);
    fgets(i, 16, stdin);
	val = strtod(i, &ptr);
    if (strlen(i) <= 1 || strlen(ptr) > 1)
        return default_value;
    else
        return val;
}

int askYesNo(char* msg, int default_value, int ask)
{
	if (!ask)
		return default_value;
	char* yn;
	yn = malloc(20);
	printf(msg);
	fgets(yn, 20, stdin);
	switch (yn[0])
	{
	case 'y':
		return 1;
	case 'n':
		return 0;
	default:
		return default_value;
	}
}


void printMap(double** map, int width, int height)
{
    int y, x;
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
            printf("%02d", (int)map[y][x]);
        printf("\n");
    }
}


int main()
{
	srand(time(NULL));
    int i, frame_number, x, y, createVideo, frameRate, deleteFrames, counter, showCurrentYear, settings;
	char* yr;
	map_state* ms;


	//AutoMap logo
	printf("####     #####   ##### ##############   ############\n### ###     ###     ###         ##        ##       ##\n###    ###    ###     ###        ##         ##       ##\n###########    ###     ###        ##         ##       ##\n###       ###   ###     ###        ##         ##       ##\n#####      #####   #########       ######       ###########\n\n    ###          ###         ####       ############\n     #####      #####        ### ###      ##        ##\n       #######  #######      ###    ###     ###########\n        ###   #####  ###     ###########     ###\n         ###    ###   ###    ###       ###    ###\n           ###    ##    ###  #####      #####  #####\n");
	printf("For more information, see https://pommicket.itch.io/automap\n");
    printf("WARNING: AutoMap may temporarily use a lot of your disk space. The default settings should use about 300MB.\n");

	printf("Please select a settings mode\n(%d) Default settings\n(%d) Default settings, but don't produce a video\n(%d) Basic settings\n(%d) Advanced settings\nMode (default=%d)? ", DEFAULT, NO_VIDEO, BASIC, ADVANCED, DEFAULT);
	settings = getInt("", DEFAULT, 1);


	ms = malloc(sizeof(map_state));
	ms->showMapGen = 0;
    ms->width = getInt("Width (default=1280)? ", 1280, settings > DEFAULT);
    ms->height = getInt("Height (default=720)? ", 720, settings > DEFAULT);
	if (settings == NO_VIDEO)
	{
		createVideo = 0;
	}
	else
	{
		createVideo = askYesNo("Would you like to make a video (ffmpeg/avconv is required. If you are on Windows, it must be on your PATH. default=yes)? ",
							   1, settings > DEFAULT);
		if (createVideo)
		{
			frameRate = getInt("Frame rate (default=20)? ", 20, settings > BASIC);
			deleteFrames = askYesNo("Would you like to delete the frames after the video is created (default=yes, yes is recommended)? ", 1, settings > DEFAULT);
		}
	}

	ms->mode = getInt("Select a mode.\n(0) Binary map (only land and water)\n(1) Gradient map (with elevation)\n(2) Map with empires\nMode (default=2)? ", 2, settings > DEFAULT);


	if (ms->mode == BINARY || ms->mode == EMPIRES)
	{
		ms->expandCoast      = getDouble("Expand coast probability (default=0.8)? ", 0.8, settings > BASIC);
		ms->expandCoastDecay = getInt("Expand coast decay, the lower the number the higher the decay (default=100)? ", 100, settings > BASIC);
		ms->retractCoast     = getDouble("Retract coast probability (default=0.6)? ", 0.6, settings > BASIC);
		ms->island           = getDouble("Island probability (default=0.0000025)? ", 0.0000025, settings > BASIC);
		ms->lake             = getDouble("Lake probability (default=0.0005)? ", 0.0005, settings > BASIC);
	}

	switch(ms->mode)
	{
	case GRADIENT:
		ms->base   = getDouble("Base positive random number probability (default=0.5)? ", 0.5, settings > BASIC);
		ms->factor = getDouble("Positive random number factor (default=0.4)? ", 0.4, settings > BASIC);
		ms->change = getDouble("Change multiplier (default=0.1)? ", 0.1, settings > BASIC);
		break;
	case EMPIRES:
		ms->numEmpires   = getInt("Number of empires (Some will be confined to small islands, default=20)? ", 20, settings > DEFAULT);
		ms->showMapGen   = askYesNo("Show map generation (default=yes)? ", 1, settings > DEFAULT);
		showCurrentYear  = askYesNo("Show year (default=yes)? ", 1, settings > BASIC);
		ms->empireColors = malloc((ms->numEmpires+FIRST_EMPIRE)*sizeof(color));
		ms->empireColors[WATER]      = *mkColor((double)WATERR/255, (double)WATERG/255, (double)WATERB/255);
		ms->empireColors[UNOCCUPIED] = *mkColor((double)LANDR/255, (double)LANDG/255, (double)LANDB/255);
		ms->empireColors[BLACK]      = *mkColor(0, 0, 0);
		ms->empireColors[WHITE]      = *mkColor(1, 1, 1);
		for (i = 0; i < ms->numEmpires; i++)
			ms->empireColors[i+FIRST_EMPIRE] = *randColor();
		break;
	}

	if (createVideo)
	{
		//Ask for length (s)
		ms->numFrames = (int)(frameRate * getDouble("Length of video in seconds (default=100)? ", 100, settings > DEFAULT));
	}
	else
	{
		//Ask for # of frames (total)
		ms->numFrames = getInt(ms->showMapGen ? "Number of frames (default=2000)? " : "Number of frames (default=1000)? ", ms->showMapGen ? 2000 : 1000, settings > DEFAULT);
	}

	if (ms->showMapGen)
		ms->numFrames /= 2;

    double** map = blankMap(ms->width, ms->height);
    ms->map = map;

	ms->newMap = blankMap(ms->width, ms->height);
    ms->contents = malloc(ms->width * ms->height * 3 + ms->height + 90);
    char* start = malloc(90);
    sprintf(start, "P6\n# AutoMap (pommicket.itch.io/auto-map)\n%d %d\n255\n", ms->width, ms->height);



    char* folder = malloc(32);
    sprintf(folder, "map%d", rand());

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__) // Helps if you're trying to compile for Windows.
	mkdir(folder);
#else
    mkdir(folder, 0777);
#endif
	
	char* command;
    char* filename;
	char* png_filename;
	frame_number = 0;
	if (ms->mode == EMPIRES)
	{
		ms->mode = BINARY;
		for (frame_number = 0; frame_number < ms->numFrames; frame_number++)
		{
			printf("Preparing map %d/%d (%.2f%%)...\n", frame_number+1, ms->numFrames, 100*(double)(frame_number+1)/ms->numFrames);
			if (ms->showMapGen)
			{
				ms->frameNumber = frame_number + 1;
				filename = malloc(64);
				png_filename = malloc(64);
				command = malloc(200);
				sprintf(filename, "%s/frame%09d.ppm", folder, frame_number);
				sprintf(png_filename, "%s/frame%09d.png", folder, frame_number);
				if (ON_WINDOWS)
					sprintf(command, "convert %s %s && del %s", filename, png_filename, filename);
				else
					sprintf(command, "convert %s %s && rm %s", filename, png_filename, filename);
				if (showCurrentYear)
				{
					yr = showYear(ms);
					writeString(ms->newMap, yr, 0, 0);
				}
				writeFile(filename, writePPM(ms, start, 1));
				system(command);
			}

			iteration(ms);
		}
		ms->mode = EMPIRES;
		if (!ms->showMapGen)
			frame_number = 0;
		//Place empires
		for (i = 0; i < ms->numEmpires; i++)
		{
			while (ms->map[(y=randrange(0, ms->height))][(x=randrange(0, ms->width))] == WATER);
			ms->map[y][x] = i+FIRST_EMPIRE;
		}
	}

    for (; frame_number < ms->numFrames + ms->showMapGen*ms->numFrames; frame_number++)
    {
		counter = frame_number+1;
		i = counter - ms->showMapGen * ms->numFrames;
        printf(ms->mode == EMPIRES ? "Expanding empires %d/%d (%.2f%%)...\n" : "Creating map %d/%d (%.2f%%)...\n", i, ms->numFrames, 100*(double)i/ms->numFrames);
        if (ms->mode == EMPIRES && showCurrentYear)
		{
			yr = showYear(ms);
			writeString(ms->newMap, yr, 0, 0);
		}
		filename = malloc(64);
		png_filename = malloc(64);
		command = malloc(200);
		sprintf(filename, "%s/frame%09d.ppm", folder, frame_number);
		sprintf(png_filename, "%s/frame%09d.png", folder, frame_number);
		if (ON_WINDOWS)
			sprintf(command, "convert %s %s && del %s", filename, png_filename, filename);
		else
			sprintf(command, "convert %s %s && rm %s", filename, png_filename, filename);
        writeFile(filename, writePPM(ms, start, 0));
		system(command);
		ms->frameNumber = counter;
        iteration(ms);
    }

	if (!createVideo)
	{
		printf("Done!\n");
		return 0;
	}

	printf("Creating video...\n");

	command = malloc(128);
	sprintf(command, "ffmpeg -framerate %d -i '%s/frame%%09d.png' -c:v libx264 %s/video.mp4", frameRate, folder, folder);
	printf("%s\n", command);
	if (system(command))
	{
		// ffmpeg failed. Trying avconv.
		command = malloc(128);
		sprintf(command, "avconv -framerate %d -i '%s/frame%%09d.png' -c:v libx264 %s/video.mp4", frameRate, folder, folder);
		printf("%s\n", command);
		if (system(command))
		{
			printf("Error - You must have avconv/ffmpeg installed. If you are on Windows, one of them must be on your PATH.\n");
			return FFMPEG_AVCONV_NOT_FOUND;
		}
	}
	printf("%s/video.mp4 created.\n", folder);
	if (!deleteFrames)
	{
		printf("Done!\n");
		return 0;
	}
	command = malloc(128);


	if (ON_WINDOWS)
		sprintf(command, "del %s/frame*.png", folder);
	else
		sprintf(command, "rm %s/frame*.png", folder);
	printf("%s\n", command);
	system(command);
	printf("Frames deleted.\nDone.\n");


    return 0;
}
