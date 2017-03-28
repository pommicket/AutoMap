#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define ON_WINDOWS 1
#else
#define ON_WINDOWS 0
#endif

#define FUNCTION_LENGTH 40

//Map modes
#define BINARY 0
#define GRADIENT 1
#define EMPIRES 2

//Settings modes
#define DEFAULT 1
#define NO_VIDEO 0
#define BASIC 2
#define ADVANCED 3

//Cell types
#define WATER	     0
#define LAND  	     1
#define BLACK        2
#define WHITE        3
#define UNOCCUPIED   1
#define FIRST_EMPIRE 4

//Error codes
#define FFMPEG_AVCONV_NOT_FOUND 1

//PPM IO
#define PPM_START 13 // Where the actual data starts for the letter PPM files.

//Colors
#define LANDR 0
#define LANDG 150
#define LANDB 0

#define WATERR 0
#define WATERG 0
#define WATERB 150

//Letters
#define LETTER_WIDTH 20
#define LETTER_HEIGHT 20
