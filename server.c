/*
	Author: Cameron Hillman
	Purpose: to run a server at localhost to return the Guild Wars 2
	Trading Post price of an item.

	Code based on stub by Richard Buckland.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <curl/curl.h>
#include <regex.h>

#define REQUEST_BUFFER_SIZE 1000
#define MAX_ID_SIZE 8
#define DEFAULT_PORT 7191
#define NUMBER_OF_PAGES_TO_SERVE 1000

#define INDEX_FILE	"index.html"
#define ICO_FILE	"faviconx.ico.gz"
#define PRICE_FILE	"price.html"

#define INDEX	0
#define ICO		1
#define PRICE	2

#define MAX_RESPONSE_SIZE 10000
#define GEN_BUFF 100

typedef struct _gw2price {
	int gold;
	int silver;
	int copper;
} gw2price;

typedef struct _HTMLDat {
	char *memory;
	size_t size;
} HTMLDat;

int waitForConnection(int serverSocket);
int makeServerSocket(int portno);
void serveIndex(int socket);
void serveICO(int socket);
void servePrice(int socket, int itemID);
void sendFile(int socket, int type);
gw2price getPrice(int itemID);
size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
gw2price intToPrice(int cost);
void updateHTML(int itemID);
void getItemDetails(int itemID, char *name);

int main(int argc, char* argv[]) {

	printf("STARTING SERVER\n");
	printf("Access this server at http://localhost:%d\n", DEFAULT_PORT);
	printf("*******************************************\n");

	int serverSocket = makeServerSocket(DEFAULT_PORT);
	char request[REQUEST_BUFFER_SIZE];
	int numberServed = 0;
	while (numberServed < NUMBER_OF_PAGES_TO_SERVE) {
		printf("\n*** So far served %d pages ***\n", numberServed);

		int connectionSocket = waitForConnection(serverSocket);

		int bytesRead = recv(connectionSocket, request, sizeof(request) - 1, 0);
		assert(bytesRead >= 0);

		printf("*** Received http request ***\n%s\n", request);

		printf("*** Sending http response ***\n\n");

		char filepath[MAX_ID_SIZE];
		sscanf(request, "GET %s", filepath);

		if (strcmp(filepath, "/") == 0) {
			serveIndex(connectionSocket);
		} else if (strcmp(filepath, "/favicon.ico") == 0) {
			serveICO(connectionSocket);
		} else if (strncmp(filepath, "/?itemid=", strlen("/?itemid=")) == 0) {
			int itemID;
			sscanf(filepath, "/?itemid=%d", &itemID);

			servePrice(connectionSocket, itemID);
		}

		close(connectionSocket);
		++numberServed;
	}

	printf("*** Shutting server down ***\n");
	close(serverSocket);

	return EXIT_SUCCESS;
}

int waitForConnection(int serverSocket) {

	const int serverMaxBacklog = 10;
	listen(serverSocket, serverMaxBacklog);

	struct sockaddr_in clientAddress;
	socklen_t clientLen = sizeof(clientAddress);
	int connectionSocket = accept(serverSocket, (struct sockaddr *) &clientAddress, &clientLen);
	assert(connectionSocket >= 0);

	return connectionSocket;
}

int makeServerSocket(int portNumber) {

	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	assert(serverSocket >= 0);

	struct sockaddr_in serverAddress;
	serverAddress.sin_family		= AF_INET;
	serverAddress.sin_addr.s_addr	= INADDR_ANY;
	serverAddress.sin_port			= htons(portNumber);

	const int optionValue = 1;
	setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optionValue, sizeof (int));

	int bindSuccess = bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof serverAddress);

	assert(bindSuccess >= 0);

	return serverSocket;
}

void serveIndex(int socket) {
	const char *header =
		"HTTP/1.1 200 OK\n"
		"Content-Type: text/html\n"
		"\n";
		
	send(socket, header, strlen(header), 0);

	sendFile(socket, INDEX);
}

void serveICO(int socket) {
	const char *header =
		"HTTP/1.1 200 OK\n"
		"Content-Length: 1754\n"
		"Content-Type: image/x-icon\n"
		"Content-Encoding: gzip\n"
		"\n";
		
	send(socket, header, strlen(header), 0);

	sendFile(socket, ICO);
}

void servePrice(int socket, int itemID) {
	const char *header =
		"HTTP/1.1 200 OK\n"
		"Content-Type: text/html\n"
		"\n";

	send(socket, header, strlen(header), 0);

	updateHTML(itemID);

	sendFile(socket, PRICE);
}

void sendFile(int socket, int type) {
	FILE *index;
	char l[GEN_BUFF];
	
	if		(type == INDEX)	{index = fopen(INDEX_FILE, "r");}
	else if (type == ICO)	{index = fopen(ICO_FILE, "r");}
	else					{index = fopen(PRICE_FILE, "r");}

	while (fgets(l, GEN_BUFF, index) != NULL) {
		send(socket, l, strlen(l), 0);
	}

	fclose(index);
}

gw2price getPrice(int itemID) {
	char url[GEN_BUFF];
	char path[] = "https://api.guildwars2.com/v2/commerce/prices/";
	sprintf(url, "%s%d", path, itemID);

	CURL *curl;
	HTMLDat response;

	response.memory = malloc(1);
	response.size = 0;

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &response);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

		curl_easy_perform(curl);

		curl_easy_cleanup(curl);
	}

	gw2price cost;

	if (strcmp(response.memory, "{\"text\":\"no such id\"}") == 0) {
		cost.copper = -1;
	} else {
		int result;
		sscanf(response.memory, "{\"id\":%*d,\"buys\":{\"quantity\":%*d,\"unit_price\":%*d},\"sells\":{\"quantity\":%*d,\"unit_price\":%d}}", &result);
		cost = intToPrice(result);
	}

	printf("The response was:\n%s\n", response.memory);

	return cost;
}

size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;
	HTMLDat *mem = (HTMLDat *) userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
		/* out of memory! */ 
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

// cost: 12809798 -> 1280 97 98 -> 1280g 97s 98c
gw2price intToPrice(int cost) {
	gw2price result;
	
	result.copper = cost % 100;
	cost -= result.copper;
	cost /= 100;

	result.silver = cost % 100;
	cost -= result.silver;
	cost /= 100;

	result.gold = cost;

	return result;
}

void updateHTML(int itemID) {
	FILE *index = fopen(PRICE_FILE, "w");

	gw2price cost = getPrice(itemID);

	if (cost.copper != -1) {
		char name[GEN_BUFF];

		getItemDetails(itemID, name);

		fprintf(index,	"<!DOCTYPE html>"
						"<head><title>Trading Post Lookup</title></head>"
						"<body><p>%s (Item ID %d) costs:<br>"
						"%d <img src=\"https://wiki.guildwars2.com/images/d/d1/Gold_coin.png\" alt=\"gold\" height=\"15\" width=\"15\">  "
						"%d <img src=\"https://wiki.guildwars2.com/images/3/3c/Silver_coin.png\" alt=\"silver\" height=\"15\" width=\"15\">  "
						"%d <img src=\"https://wiki.guildwars2.com/images/e/eb/Copper_coin.png\" alt=\"copper\" height=\"15\" width=\"15\">  <br><br>"
						"<a href=\"http://localhost:7191\"><button type=\"button\">Back</button></a>"
						"</p></body></html>", name, itemID, cost.gold, cost.silver, cost.copper);
	} else {
		fprintf(index, "<!DOCTYPE html>"
						"<head><title>Trading Post Lookup</title></head>"
						"<body><p>Error: itemID not valid.<br>"
						"This is caused either because the ID does not belong to an item<br>"
						"(eg. it belonged to a skill) or because that item is not available<br>"
						"on the trading post (eg. it is account/soul bound).<br><br>"
						"<a href=\"http://localhost:7191\"><button type=\"button\">Back</button></a>"
						"</p></body>"
						"</html>");
	}

	fclose(index);
}

void getItemDetails(int itemID, char *name) {
	char url[GEN_BUFF];
	char path[] = "https://api.guildwars2.com/v2/items/";
	sprintf(url, "%s%d", path, itemID);

	CURL *curl;
	HTMLDat response;

	response.memory = malloc(1);
	response.size = 0;

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &response);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

		curl_easy_perform(curl);

		curl_easy_cleanup(curl);
	}

	sscanf(response.memory, "{\"name\":\"%[A-Za-z]s\"", name);
	printf("Name: %s\n", name);

	printf("The response was:\n%s\n", response.memory);
}
