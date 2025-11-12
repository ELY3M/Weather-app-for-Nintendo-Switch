/* Includes */
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <switch.h>
#include <stdlib.h> // used for alloc/malloc
#include <string.h> // used for mini tools
#include <unistd.h> // used for chdir()
#include <stdbool.h>// bool = 1 == true; 0 == false;
#include <curl/curl.h>
#include <fcntl.h>

#include "download.h"

#define Megabytes_in_Bytes	1048576
#define Kibibyte_in_Bytes	1024

int dlnow_Mb = 0;
int dltotal_Mb = 0;

// measure download speed
bool open_room = false;
bool once = false;
int dlspeed = 0;
int dl_curr = 0;
int	curr_sec = 0; // current second from system
int ticket = 0; // current (second + 1) from system

char global_f_tmp[512]; /* we need this global FILE variable for passing args */

/* Functions */
int older_progress(__attribute__((unused)) void *p, double dltotal, double dlnow, __attribute__((unused)) double ultotal, __attribute__((unused)) double ulnow) {
	return xferinfo((curl_off_t)dltotal, (curl_off_t)dlnow);
}

/*

    fun getNWSStringFromURL(url: String): String =
        getNWSStringFromURLBase(url, "application/vnd.noaa.dwml+xml;version=1")

    fun getNWSStringFromURLJSON(url: String): String =
        getNWSStringFromURLBase(url, "application/geo+json;version=1")

*/



bool	downloadFile(const char *url, const char *filename)
{
	FILE				*dest = NULL;
	CURL				*curl = NULL;
	CURLcode			res = -1;
	struct myprogress	prog;

	consoleClear();

	curl = curl_easy_init();
	struct curl_slist *headers = NULL;
	
	if (curl) {
		prog.lastruntime = 0;
		prog.curl = curl;

		dest = fopen(filename, "wb");
		if (dest == NULL) {
			perror("fopen");
		} else {
			curl_easy_setopt(curl, CURLOPT_URL, url);						// getting URL from char *url
			
			
			
			curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/png,image/svg+xml,*/*;q=0.8");
			curl_slist_append(headers, "Content-Type: application/json");
			curl_slist_append(headers, "charsets: utf-8");
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); 
			
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "Weather Text for Nintendo Switch by ELY M. contact money@mboca.com");
			
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);					// useful for debugging
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); 			// skipping cert. verification, if needed
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); 			// skipping hostname verification, if needed
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, dest);				// writes pointer into FILE *destination
			curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, older_progress);
			curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &prog);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
			
			if (strlen(global_f_tmp) != 0) curl_easy_setopt(curl, CURLOPT_USERPWD, global_f_tmp);
			
			res = curl_easy_perform(curl);									// perform tasks curl_easy_setopt asked before
			
			fclose(dest);
		}
	}

	curl_easy_cleanup(curl);												// Cleanup chunk data, resets functions.

	if (res != CURLE_OK) {
		printf("\n# Failed: %s%s%s\n", CONSOLE_RED, curl_easy_strerror(res), CONSOLE_RESET);
		remove(filename);
		return false;
	}
	
	return true;
}

size_t dnld_header_parse(void *hdr, size_t size, size_t nmemb)
{
	const size_t	cb = size * nmemb;
	const char		*hdr_str = hdr;
	const char		*compareContent = "Content-disposition:";

	/* Example:
	* ...
	* Content-Type: text/html
	* Content-Disposition: filename=name1367; charset=funny; option=strange
	*/
	if (strstr(hdr_str, compareContent)) {
		printf ("has c-d: %s\n", hdr_str);
	}

	return cb;
}

int xferinfo(curl_off_t dltotal, curl_off_t dlnow) {

	dlnow_Mb = dlnow / Megabytes_in_Bytes;
	dltotal_Mb = dltotal / Megabytes_in_Bytes;
	
	// we need to create a separated room inside this "while" loop
	// so we can process this room once every so often
	curr_sec = time(0);
	
	if (open_room == false) {
		ticket = time(0) + 1;
		dl_curr = dlnow;
		open_room = true; // closing room
	}
	
	if (curr_sec >= ticket) {
		dlspeed = (dlnow - dl_curr) / Kibibyte_in_Bytes;
		open_room = false; // opening room
	}
	
	if (dltotal_Mb == 1) {
		printf("# DOWNLOAD: %" CURL_FORMAT_CURL_OFF_T " Bytes of %" CURL_FORMAT_CURL_OFF_T " Bytes | %3d Kb/s\r", dlnow, dltotal, dlspeed);
	} else if (dltotal_Mb > 1) {
		printf("# DOWNLOAD: %d Mb of %d Mb | %3d Kb/s\r", dlnow_Mb, dltotal_Mb, dlspeed);
	}
	
	if (dlnow == dltotal && dltotal > 0 && once == false) {
		printf("\n"); // lol, is required
		once = true;
	}
	
	consoleUpdate(NULL);
	return 0;
}

void curlInit(void) {
	socketInitializeDefault();
}

void curlExit(void) {
	appletEndBlockingHomeButton();
	socketExit();
}




bool FILE_TRANSFER_HTTP(char *lat, char *lon) {
	consoleClear();
	
	
	
	//https://forecast.weather.gov/MapClick.php?lat=40.6&lon=-111.64&FcstType=json
	char	*url = "https://forecast.weather.gov/MapClick.php?";
	char *finalurl = "";  
	
	snprintf(finalurl, 256, "%slat=%s&lon=%s&FcstType=json", url, lat, lon);
	
	
	
	//char	*filename = NULL;

	
	if (finalurl == (void *) -1) {
		return (false);
	} else if (finalurl != NULL) {


		downloadFile(finalurl, "weather.txt");
		
		


		// release memory
		//free(url); //crash//  
		
		
	}

	/*printf ("\nRemote name: %s\n", dnld_params.dnld_remote_fname);*/

	return 0; ///(functionExit());
}

