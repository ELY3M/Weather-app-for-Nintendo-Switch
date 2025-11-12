/*


Weather app for Nintendo Switch 


ELY M. 



*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "download.h"
#include "jsmn.h"


const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

const char* const months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
const char* const weekDays[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


u32 kDown;

SDL_Window * 	window;
SDL_Renderer * 	renderer;
SDL_Surface *	surface;

TTF_Font* smfont;
TTF_Font* font;
TTF_Font* lgfont;
	
SDL_Surface* mouseImage;
SDL_Texture* mouseTexture;

SDL_Surface* weatherImage;
SDL_Texture* weatherTexture;


char *weathertemp = "00°F";
char *weathericon = "romfs:/gfx/unknown.png";
char *weathertext = "unknown";
char *weatherlocation = "unknown location";

static inline SDL_Color SDL_MakeColour(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	SDL_Color colour = {r, g, b, a};
	return colour;
}

#define BLACK		SDL_MakeColour(0, 0, 0, 255)
#define CYAN		SDL_MakeColour(0, 255, 255, 255)
#define WHITE		SDL_MakeColour(255, 255, 255, 255)
#define GREEN		SDL_MakeColour(67, 170, 87, 255)
#define PURPLE		SDL_MakeColour(101, 82, 105, 255)
#define YELLOW		SDL_MakeColour(230, 227, 8, 255)
#define GREY_F		SDL_MakeColour(64, 64, 64, 128)


void renderTexture(SDL_Texture *tex, SDL_Renderer *ren, int Srcx, int Srcy, int Destx, int Desty, int w, int h)
{
	SDL_Rect srce;
	srce.x = Srcx;
	srce.y = Srcy;
	srce.w = w;
	srce.h = h;

	SDL_Rect dest;
	dest.x = Destx;
	dest.y = Desty;
	dest.w = w;
	dest.h = h;

	SDL_RenderCopy(ren, tex, &srce, &dest);
}

void SDL_DrawText(SDL_Renderer *renderer, TTF_Font *font, int x, int y, SDL_Color colour, const char *text)
{
	SDL_Surface *surface = TTF_RenderText_Blended_Wrapped(font, text, colour, 1280);
	SDL_SetSurfaceAlphaMod(surface, colour.a);
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);

	SDL_Rect position;
	position.x = x; position.y = y;
	SDL_QueryTexture(texture, NULL, NULL, &position.w, &position.h);
	SDL_RenderCopy(renderer, texture, NULL, &position);
	SDL_DestroyTexture(texture);
}

void SDL_DrawTextf(SDL_Renderer *renderer, TTF_Font *font, int x, int y, SDL_Color colour, const char* text, ...)
{
	char buffer[256];
	va_list args;
	va_start(args, text);
	vsnprintf(buffer, 256, text, args);
	SDL_DrawText(renderer, font, x, y, colour, buffer);
	va_end(args);
}

void SDL_DrawRect(SDL_Renderer *renderer, int x, int y, int w, int h, SDL_Color colour)
{
	SDL_Rect rect;
	rect.x = x; rect.y = y; rect.w = w; rect.h = h;
	SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, colour.a);
	SDL_RenderFillRect(renderer, &rect);
}



char	*popKeyboard(char *message, size_t size)
{
	SwkbdConfig	skp; // Software Keyboard Pointer
	Result		rc = swkbdCreate(&skp, 0);
	char		*tmpout = NULL;

	// +1 for the '\0'
	tmpout = (char *)calloc(sizeof(char), size + 1);
	if (tmpout == NULL)
		return (NULL);

	if (R_SUCCEEDED(rc)) {
		swkbdConfigMakePresetDefault(&skp);
		swkbdConfigSetGuideText(&skp, message);
		rc = swkbdShow(&skp, tmpout, size);
		swkbdClose(&skp);
	} else {
		free(tmpout);
		tmpout = NULL;
	}

	return (tmpout);
}



char *Clock(void) {
	
	
		//Clock
		char *clock = "time";
		const char* ampm = "AM";
		time_t unixTime = time(NULL);
		struct tm* timeStruct = localtime((const time_t *)&unixTime);
		int hours = timeStruct->tm_hour;
		int minutes = timeStruct->tm_min;
		int seconds = timeStruct->tm_sec;
		int day = timeStruct->tm_mday;
		int month = timeStruct->tm_mon;
		int year = timeStruct->tm_year +1900;
		int wday = timeStruct->tm_wday;
		
		
		if (hours <= 12 && hours >= 0) {
		//AM	
		ampm = "AM";
		}
        else if (hours >= 13 && hours <= 24)
        {
		hours = (hours - 12);
		//PM
		ampm = "PM";
        }

		snprintf(clock, 256, "%s %s %i %i %02i:%02i:%02i %s", weekDays[wday], months[month], day, year, hours, minutes, seconds, ampm);			
		SDL_DrawTextf(renderer, font, SCREEN_WIDTH / 2, 3, CYAN, clock);
		
		return (clock);
	
	
}

char * removeSpaces(char *string)
{
    // non_space_count to keep the frequency of non space characters
    int non_space_count = 0;
 
    //Traverse a string and if it is non space character then, place it at index non_space_count
    for (int i = 0; string[i] != '\0'; i++)
    {
        if (string[i] != ' ')
        {
            string[non_space_count] = string[i];
            non_space_count++;//non_space_count incremented
        }   
		
    }
    
    //Finally placing final character at the string end
    string[non_space_count] = '\0';
    return string;
}

static bool	setMyGPS(void)
{
	FILE		*fp = NULL;
	char		*tmpout = NULL;
	bool		err = false;

	tmpout = popKeyboard("set your GPS like this 39.232,-93.75", 256);

	if (tmpout != NULL) {
		if (*tmpout == 0) {
			err = true;
		} else {
			if ((fp = fopen("sdmc:/switch/weather-gps.txt", "wb")) != NULL) {
				fprintf(fp, "%s", tmpout);
				fclose(fp);
			} else {
				err = true;
			}
		}
		
		//printf("%s My GPS: %s \n%s", CONSOLE_CYAN, tmpout, CONSOLE_RESET);

		free(tmpout);
		tmpout = NULL;

	} else {
		err = true;
	}

	return (err);
}


char *readMyGPS(void)
{

	FILE		*fp = NULL;
	char		*buffer = NULL;
	size_t		nbytes = 0;
	struct stat	st;

	// if tmpfile is empty, return true and pop the keyboard.
	stat("sdmc:/switch/weather-gps.txt", &st);
	if (st.st_size == 0) {
		setMyGPS();
	}

	// if calloc fail, pop the keyboard
	buffer = (char *)calloc(sizeof(char), st.st_size);
	if (buffer == NULL) {
		setMyGPS();
	}

	
 
	fp = fopen("sdmc:/switch/weather-gps.txt", "r");
	if (fp != NULL) {
		nbytes = fread(buffer, sizeof(char), st.st_size, fp);
		if (nbytes > 0) {
			//mygps = buffer;
			//remove spaces
			//printf("readMyGPS(): trying to remove spaces\n");
			buffer = removeSpaces(buffer);
			strtok(buffer, "\n");
		}
		fclose(fp);
	}
	

	//printf("%s My GPS: %s \n%s", CONSOLE_CYAN, buffer, CONSOLE_RESET);
	// free memory
	// free(buffer);
	// buffer = NULL;


	return (buffer);
}



static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}



void *getjson(char *JsonString) {
	int i;
	int r;
	jsmn_parser p;
	jsmntok_t t[256]; /* We expect no more than 128 tokens */
	
	char *gettemp = "";
	char *getweather = ""; 
	char *getweatherimage = "";  
	char *getlocation = ""; 
	

	jsmn_init(&p);
	r = jsmn_parse(&p, JsonString, strlen(JsonString), t, sizeof(t)/sizeof(t[0]));
	if (r < 0) {
		//printf("Failed to parse JSON: %d\n", r);
		return 0;
	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		//printf("Object expected\n");
		return 0;
	}


	
	for (i = 1; i < r; i++) {
		if (jsoneq(JsonString, &t[i], "Temp") == 0) {
			//printf("Temp: %.*s\n", t[i+1].end-t[i+1].start, JsonString + t[i+1].start);
			snprintf(gettemp, 256, "%.*s", t[i+1].end-t[i+1].start, JsonString + t[i+1].start); 
			i++;
		} 
				
		else if (jsoneq(JsonString, &t[i], "Weather") == 0) {
			//printf("Weather: %.*s\n", t[i+1].end-t[i+1].start, JsonString + t[i+1].start);
			snprintf(getweather, 256, "%.*s", t[i+1].end-t[i+1].start, JsonString + t[i+1].start);
			i++;
		} else if (jsoneq(JsonString, &t[i], "Weatherimage") == 0) {
			//printf("Weatherimage: %.*s\n", t[i+1].end-t[i+1].start, JsonString + t[i+1].start);
			snprintf(getweatherimage, 256, "%.*s", t[i+1].end-t[i+1].start, JsonString + t[i+1].start);
			i++;
		} 
		
		
   		else if (jsoneq(JsonString, &t[i], "name") == 0) {
			//printf("Location: %.*s\n", t[i+1].end-t[i+1].start, JsonString + t[i+1].start);
			snprintf(getlocation, 256, "%.*s", t[i+1].end-t[i+1].start, JsonString + t[i+1].start);  
			i++;
		} 
		
		
		
		//snprintf(string, 256, "\n\n\n\nTemp: %s\nWeather: %s\nLocation: %s\n", temp, weather, location); 
	
		
		
	}
	
	
	char* myclock = Clock();
	SDL_DrawTextf(renderer, font, SCREEN_WIDTH / 2, 3, CYAN, myclock);
		
	
	snprintf(weathertemp, 256, "%s°F", gettemp);
	SDL_DrawTextf(renderer, lgfont, SCREEN_WIDTH / 2, SCREEN_HEIGHT - 30, CYAN, weathertemp);
	

	snprintf(weathericon, 256, "romfs:/gfx/%s", getweatherimage);
	weatherImage = IMG_Load(weathericon);
	weatherTexture = SDL_CreateTextureFromSurface(renderer, weatherImage);
	SDL_FreeSurface(weatherImage);
	
	snprintf(weathertext, 256, "%s", getweather);
	SDL_DrawTextf(renderer, font, SCREEN_WIDTH / 2, SCREEN_HEIGHT + 73, CYAN, weathertext);
	

	snprintf(weatherlocation, 256, "%s", getlocation);
	SDL_DrawTextf(renderer, font, SCREEN_WIDTH / 2, SCREEN_HEIGHT + 136, CYAN, weatherlocation);

		
	
	//SDL_RenderPresent(renderer);
	return JsonString;
}


char *readWeather(void)
{

	FILE		*fp = NULL;
	char		*buffer = NULL;
	size_t		nbytes = 0;
	struct stat	st;
	

	// if tmpfile is empty, return true and pop the keyboard.
	stat("sdmc:/switch/weather.txt", &st);
	if (st.st_size == 0) {
		return (buffer);
	}

	// if calloc fail, pop the keyboard
	buffer = (char *)calloc(sizeof(char), st.st_size);
	if (buffer == NULL) {
		return (buffer);
	}

	
 
	fp = fopen("sdmc:/switch/weather.txt", "r");
	if (fp != NULL) {
		nbytes = fread(buffer, sizeof(char), st.st_size, fp);
		if (nbytes > 0) {
			//mygps = buffer;
		}
		fclose(fp);
	}
	
	//parse the json file//  
	getjson(buffer);
	
	
	//printf("%s Weather: %s\n%s", CONSOLE_CYAN, buffer, CONSOLE_RESET);
	// free memory
	// free memory
	// free(buffer);
	// buffer = NULL;


	return (buffer);
}


int main()
{


	//char *myclock = NULL;
	char *mygps = NULL;
	char *lon = NULL;
   	char *lat = NULL;
	
	
	SDL_Init(SDL_INIT_EVERYTHING);
	IMG_Init(IMG_INIT_PNG);
	TTF_Init();
	romfsInit();
	curlInit();

	padConfigureInput(1, HidNpadStyleSet_NpadStandard);
	PadState pad;
	padInitializeDefault(&pad);

	// Create an SDL window & renderer
	window = SDL_CreateWindow("Main-Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);

	// Font loading
	smfont = TTF_OpenFont("romfs:/fonts/FSEX300.ttf", 30);
	font = TTF_OpenFont("romfs:/fonts/FSEX300.ttf", 43);
    lgfont = TTF_OpenFont("romfs:/fonts/FSEX300.ttf", 103);

	
	weatherImage = IMG_Load(weathericon);
	weatherTexture = SDL_CreateTextureFromSurface(renderer, weatherImage);
	SDL_FreeSurface(weatherImage);

	while (appletMainLoop())
	{
	
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		//Clear
		SDL_RenderClear(renderer);
	

	
		//Clock
		const char* ampm = "AM";
		time_t unixTime = time(NULL);
		//struct tm* timeStruct = gmtime((const time_t *)&unixTime);
		struct tm* timeStruct = localtime((const time_t *)&unixTime);
		int hours = timeStruct->tm_hour;
		int minutes = timeStruct->tm_min;
		int seconds = timeStruct->tm_sec;
		int day = timeStruct->tm_mday;
		int month = timeStruct->tm_mon;
		int year = timeStruct->tm_year +1900;
		int wday = timeStruct->tm_wday;
		
		
		if (hours <= 12 && hours >= 0) {
		//AM	
		ampm = "AM";
		}
        else if (hours >= 13 && hours <= 24)
        {
		hours = (hours - 12);
		//PM
		ampm = "PM";
        }

		//char *clockstring[256]; 
		//snprintf(clockstring, 256, "%s %s %i %i %02i:%02i:%02i %s", weekDays[wday], months[month], day, year, hours, minutes, seconds, ampm);
		//SDL_DrawTextf(renderer, font, SCREEN_WIDTH / 2, 3, CYAN, clockstring);
		
		SDL_DrawTextf(renderer, font, SCREEN_WIDTH / 2, 3, CYAN, "%s %s %i %i %02i:%02i:%02i %s", weekDays[wday], months[month], day, year, hours, minutes, seconds, ampm);

		//myclock = Clock();
		//SDL_DrawTextf(renderer, font, SCREEN_WIDTH / 2, 3, CYAN, myclock);
		
		
		
		
		
		mygps = readMyGPS();
	    lat = strtok(mygps, ",");
   	    lon = strtok(NULL, ",");	
		char gpsstring[256]; 
		snprintf(gpsstring, 256, "My GPS: %s,%s", lat,lon);
		SDL_DrawTextf(renderer, font, SCREEN_WIDTH / 2, 43, CYAN, gpsstring);
		


		renderTexture(weatherTexture, renderer, 0, 0, SCREEN_WIDTH / 2, 200, 390, 300);
		
		
		SDL_DrawTextf(renderer, lgfont, SCREEN_WIDTH / 2, SCREEN_HEIGHT - 30, CYAN, weathertemp);
		
		SDL_DrawTextf(renderer, font, SCREEN_WIDTH / 2, SCREEN_HEIGHT + 73, CYAN, weathertext);
	
		SDL_DrawTextf(renderer, font, SCREEN_WIDTH / 2, SCREEN_HEIGHT + 136, CYAN, weatherlocation);
		
		
		SDL_DrawTextf(renderer, smfont, 3, SCREEN_HEIGHT + 300, CYAN, "Press - for GPS Setting | Press A or X for Weather Info | Press + to exit");
		
	

		if (kDown & HidNpadButton_Minus)  { 
		setMyGPS();	
		}


		if (kDown & HidNpadButton_A)  { 
		FILE_TRANSFER_HTTP(lat, lon); 
		sleep(1); 
		readWeather(); 

		}
	
		
		if (kDown & HidNpadButton_X)  { 
		FILE_TRANSFER_HTTP(lat, lon); 
		sleep(1);
		readWeather(); 
		

		}	
		
		if (kDown & HidNpadButton_Plus) { break; } 
				
		
		SDL_RenderPresent(renderer);
		
		
		
	}



	SDL_DestroyTexture(mouseTexture);
	
	curlExit();
	romfsExit();
	TTF_Quit();
	IMG_Quit();
	SDL_DestroyWindow(window);
	SDL_Quit();

	
	return EXIT_SUCCESS;
}
