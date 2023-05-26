#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "Ws2_32.lib")
#include <iostream>
#include <winsock2.h>
#include <fstream>
#include <string.h>
#include <string>
#include <time.h>
#include <map>
#include <vector>
#include <cstdio>

using namespace std;
using std::map;

const int HTTP_PORT = 27015;
const int MAX_SOCKETS = 60;
constexpr int BUFFER_SIZE = 2048;
//const int EMPTY = 0;
//const int LISTEN = 1;
//const int RECEIVE = 2;
//const int IDLE = 3;
//const int SEND = 4;
const int HEAD = 5;
const int PUT = 6;
const int POST = 7;
const int DELETE_ = 8;
const int OPTIONS = 9;
const int TRACE = 10;
const int GET = 11;
enum SocketStatus { EMPTY, LISTEN, RECEIVE, IDLE, SEND };


struct SocketState
{
	SOCKET id;
	SocketStatus recv;
	SocketStatus send;
	int sendSubType;
	time_t LastActiveted;
	char buffer[BUFFER_SIZE];
	int DataLen;
};

string title(string queryString);
int getSubType(string str);
bool addSocket(SOCKET id, SocketStatus what, SocketState* sockets, int& socketsCount);
void removeSocket(int index, SocketState* sockets, int& socketsCount);
void acceptConnection(int index, SocketState* sockets, int& socketsCount);
void receiveMessage(int index, SocketState* sockets, int& socketsCount);
void sendMessage(int index, SocketState* sockets);
void updateSocketsByResponseTime(SocketState* sockets, int& socketsCount);
void updateSendSubType(int index, SocketState* sockets);
string getLangFromMessage(int index, SocketState* sockets);
size_t getBodyIndex(string buffer);
int PutRequest(int index, char* filename, SocketState* sockets);
string traceReq(int index, SocketState* sockets);
string deleteReq(int index, SocketState* sockets);
string optionReq();
string putReq(int index, SocketState* sockets);
string postReq(int index, SocketState* sockets);
string headReq(int index, SocketState* sockets);
string getReq(int index, SocketState* sockets);
string findFile(string queryString);
string whichLanguage(string queryString);

void main()
{
	struct SocketState sockets[MAX_SOCKETS] = { 0 };
	int socketsCount = 0;

	//set timeout for 'select' function
	struct timeval timeOut;
	timeOut.tv_sec = 120;
	timeOut.tv_usec = 0;

	// Initialize Winsock (Windows Sockets).

	// Create a WSADATA object called wsaData.
	// The WSADATA structure contains information about the Windows 
	// Sockets implementation.
	WSAData wsaData;

	// Call WSAStartup and return its value as an integer and check for errors.
	// The WSAStartup function initiates the use of WS2_32.DLL by a process.
	// First parameter is the version number 2.2.
	// The WSACleanup function destructs the use of WS2_32.DLL by a process.
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "HTTP Server: Error at WSAStartup()\n";
		return;
	}

	// Server side:
	// Create and bind a socket to an internet address.
	// Listen through the socket for incoming connections.

	// After initialization, a SOCKET object is ready to be instantiated.

	// Create a SOCKET object called listenSocket. 
	// For this application:	use the Internet address family (AF_INET), 
	//							streaming sockets (SOCK_STREAM), 
	//							and the TCP/IP protocol (IPPROTO_TCP).
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for errors to ensure that the socket is a valid socket.
	// Error detection is a key part of successful networking code. 
	// If the socket call fails, it returns INVALID_SOCKET. 
	// The if statement in the previous code is used to catch any errors that
	// may have occurred while creating the socket. WSAGetLastError returns an 
	// error number associated with the last error that occurred.
	if (INVALID_SOCKET == listenSocket)
	{
		cout << "HTTP Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	// For a server to communicate on a network, it must bind the socket to 
	// a network address.

	// Need to assemble the required data for connection in sockaddr structure.

	// Create a sockaddr_in object called serverService. 
	sockaddr_in serverService;
	// Address family (must be AF_INET - Internet address family).
	serverService.sin_family = AF_INET;
	// IP address. The sin_addr is a union (s_addr is a unsigned long 
	// (4 bytes) data type).
	// inet_addr (Iternet address) is used to convert a string (char *) 
	// into unsigned long.
	// The IP address is INADDR_ANY to accept connections on all interfaces.
	serverService.sin_addr.s_addr = INADDR_ANY;
	// IP Port. The htons (host to network - short) function converts an
	// unsigned short from host to TCP/IP network byte order 
	// (which is big-endian).
	serverService.sin_port = htons(HTTP_PORT);

	// Bind the socket for client's requests.

	// The bind function establishes a connection to a specified socket.
	// The function uses the socket handler, the sockaddr structure (which
	// defines properties of the desired connection) and the length of the
	// sockaddr structure (in bytes).
	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR*)&serverService, sizeof(serverService)))
	{
		cout << "HTTP Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}

	// Listen on the Socket for incoming connections.
	// This socket accepts only one connection (no pending connections 
	// from other clients). This sets the backlog parameter.
	if (SOCKET_ERROR == listen(listenSocket, 5))//line need to be significantly small
	{
		cout << "HTTP Server: Error at listen(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}
	addSocket(listenSocket, LISTEN, sockets, socketsCount);

	// Accept connections and handles them one by one.
	cout << "Waiting for client to connect to the server." << endl;
	while (true)
	{
		// The select function determines the status of one or more sockets,
		// waiting if necessary, to perform asynchronous I/O. Use fd_sets for
		// sets of handles for reading, writing and exceptions. select gets "timeout" for waiting
		// and still performing other operations (Use NULL for blocking). Finally,
		// select returns the number of descriptors which are ready for use (use FD_ISSET
		// macro to check which descriptor in each set is ready to be used).
		updateSocketsByResponseTime(sockets, socketsCount);
		fd_set waitRecv;
		FD_ZERO(&waitRecv);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		//
		// Wait for interesting event.
		// Note: First argument is ignored. The fourth is for exceptions.
		// And as written above the last is a timeout, hence we are blocked if nothing happens.
		//
		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, &timeOut);
		if (nfd == SOCKET_ERROR)
		{
			cout << "HTTP Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i, sockets, socketsCount);
					break;

				case RECEIVE:
					receiveMessage(i, sockets, socketsCount);
					break;
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				sendMessage(i, sockets);
			}
		}
	}

	// Closing connections and Winsock.
	cout << "HTTP Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}

bool addSocket(SOCKET id, SocketStatus what, SocketState* sockets, int& socketsCount)
{
	// Set the socket to be in non-blocking mode.
	unsigned long flag = 1;
	if (ioctlsocket(id, FIONBIO, &flag) != 0)
	{
		cout << "HTTP Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
	}
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].DataLen = 0;
			sockets[i].LastActiveted = time(0);//reset responding time
			socketsCount++;
			return (true);
		}
	}
	return (false);
}

void removeSocket(int index, SocketState* sockets, int& socketsCount)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	sockets[index].LastActiveted = 0;
	socketsCount--;
	cout << "The socket in position: " << index << " removed." << endl;
}

void acceptConnection(int index, SocketState* sockets, int& socketsCount)
{
	SOCKET id = sockets[index].id;
	sockets[index].LastActiveted = time(0);//reset responding time
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr*)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "HTTP Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "HTTP Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	if (addSocket(msgSocket, RECEIVE, sockets, socketsCount) == false)
	{
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

void receiveMessage(int index, SocketState* sockets, int& socketsCount)
{
	SOCKET msgSocket = sockets[index].id;
	int len = sockets[index].DataLen;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);

	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "HTTP Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(index, sockets, socketsCount);
		return;
	}
	if (bytesRecv == 0)
	{
		closesocket(msgSocket);
		removeSocket(index, sockets, socketsCount);
		return;
	}
	else
	{
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		cout << "HTTP Server: Recieved: " << bytesRecv << " bytes of \"" << &sockets[index].buffer[len] << "\" message.\n";

		sockets[index].DataLen += bytesRecv;

		if (sockets[index].DataLen > 0)
		{
			updateSendSubType(index, sockets);
		}
	}

}

void sendMessage(int index, SocketState* sockets)
{
	int bytesSent = 0;
	char sendBuff[255];
	string response;

	SOCKET msgSocket = sockets[index].id;

	switch (sockets[index].sendSubType)
	{
	case GET:
		response = getReq(index, sockets);
		break;
	case OPTIONS:
		response = optionReq();
		break;
	case HEAD:
		response = headReq(index, sockets);
		break;
	case POST:
		response = postReq(index, sockets);
		break;
	case PUT:
		response = putReq(index, sockets);
		break;
	case DELETE_:
		response = deleteReq(index, sockets);
		break;
	case TRACE:
		response = traceReq(index, sockets);

	default:
		break;
	}

	strcpy(sendBuff, response.c_str());
	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	memset(sockets[index].buffer, 0, 255);
	sockets[index].DataLen = 0;

	if (SOCKET_ERROR == bytesSent)
	{
		cout << "Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}

	cout << "Server: Sent: " << bytesSent << "\\" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" message.\n";

	sockets[index].send = IDLE;
}

// A utility to check for the inactive sockets (if the response time is greater then 2 minutes) and remove them
void updateSocketsByResponseTime(SocketState* sockets, int& socketsCount)
{
	time_t currentTime;
	for (int i = 1; i < MAX_SOCKETS; i++)
	{
		currentTime = time(0);
		if ((currentTime - sockets[i].LastActiveted > 120) && (sockets[i].LastActiveted != 0))
		{
			removeSocket(i, sockets, socketsCount);
		}
	}
}

void updateSendSubType(int index, SocketState* sockets)
{
	int toSub;
	string buffer, firstWord;
	//static map<string, HTTPRequest> request = { {"TRACE",HTTPRequest::TRACE},{"DELETE",HTTPRequest::DELETE_REQ},
	//											{"PUT",HTTPRequest::PUT},{"POST",HTTPRequest::POST},
	//											{"HEAD",HTTPRequest::HEAD},{"GET",HTTPRequest::GET},{"OPTIONS",HTTPRequest::OPTIONS} };

	buffer = (string)sockets[index].buffer;

	firstWord = buffer.substr(0, buffer.find(" "));

	sockets[index].send = SEND;
	sockets[index].sendSubType = getSubType(firstWord);

	toSub = firstWord.length() + 2;//1 for space and 1 for '/'
	sockets[index].DataLen -= toSub;
	memcpy(sockets[index].buffer, &sockets[index].buffer[toSub], sockets[index].DataLen);
	sockets[index].buffer[sockets[index].DataLen] = '\0';
}

string getLangFromMessage(int index, SocketState* sockets)
{
	static map<string, string> request = { {"en","English"},{"en-AU","English"},{"en-BZ","English"},{"en-CA","English"},{"en-CB","English"},{"en-GB","English"},{"en-IE","English"},
										{"en-JM","English"},{"en-NZ","English"},{"en-PH","English"},{"en-TT","English"},{"en-US","English"},{"en-ZA","English"},{"en-ZW","English"},
										{"fr","French"},{"fr-BE","French"},{"fr-CA","French"},{"fr-CH","French"},{"fr-FR","French"},{"fr-LU","French"},{"fr-MC","French"},
										{"he","Hebrew"},{"he-IL","Hebrew"} };
	string buffer, lookWord, langWord, bufferAtLangWord;
	size_t found;
	//import buffer to string
	buffer = (string)sockets[index].buffer;
	lookWord = "?lang=";
	found = buffer.find(lookWord);
	if (found == std::string::npos)//lang Query String doesn't exist
		return "English";//defult language
	//else get the Language word in buffer
	bufferAtLangWord = &buffer[found + lookWord.length()];
	langWord = bufferAtLangWord.substr(0, bufferAtLangWord.find(" "));
	auto search = request.find(langWord);
	if (search != request.end()) {
		return request[langWord];
	}
	else {
		return "Error";//Error::No Language matched: there is no supporting language
	}
}
int getSubType(string str)
{
	int res;

	if (strcmp(str.c_str(), "GET") == 0)
	{
		return GET;
	}
	else if (strcmp(str.c_str(), "OPTIONS") == 0)
	{
		return OPTIONS;
	}
	else if (strcmp(str.c_str(), "HEAD") == 0)
	{
		return HEAD;
	}
	else if (strcmp(str.c_str(), "POST") == 0)
	{
		return POST;
	}
	else if (strcmp(str.c_str(), "PUT") == 0)
	{
		return PUT;
	}
	else if (strcmp(str.c_str(), "DELETE") == 0)
	{
		return DELETE_;
	}
	else if (strcmp(str.c_str(), "TRACE") == 0)
	{
		return TRACE;
	}
}

int PutRequest(int index, char* filename, SocketState* sockets)
{
	string content, buffer = (string)sockets[index].buffer, address = "C:\\Temp\\HTML_FILES\\";
	int buffLen = 0;
	int retCode = 200; // 'OK' code
	size_t found;
	filename = strtok(sockets[index].buffer, " ");
	address += filename;

	fstream outPutFile;
	outPutFile.open(address);

	if (!outPutFile.good())
	{
		outPutFile.open(address, ios::out);
		retCode = 201; // New file created
	}

	if (!outPutFile.good())
	{
		cout << "HTTP Server: Error writing file to local storage: " << WSAGetLastError() << endl;
		return 0; // Error opening file
	}
	found = buffer.find("\r\n\r\n");
	content = &buffer[found + 4];
	if (content.length() == 0)
		retCode = 204; // No content
	else
		outPutFile << content;

	outPutFile.close();
	return retCode;
}

string traceReq(int index, SocketState* sockets)
{
	string response;
	string rn = "\r\n";
	response = "HTTP/1.1 200 OK" + rn + "Content - type: message/http" + rn + "Content-length: ";

	int len = strlen(sockets[index].buffer);
	len += strlen("TRACE ");
	response += to_string(len) + rn + rn + sockets[index].buffer;

	return response;
}

string deleteReq(int index, SocketState* sockets)
{
	string response;
	string rn = "\r\n";
	string FileName = findFile(string(sockets[index].buffer));
	string lang = whichLanguage(string(sockets[index].buffer));
	string resource_path = "C:\\temp\\example_files\\" + lang + "\\" + FileName;

	if (remove(resource_path.c_str()) == 0) {
		response = "HTTP/1.1 200 OK" + rn + "Resource deleted successfully" + rn + rn;
	}

	else
	{
		response = "HTTP/1.1 204 Failed to delete the file" + rn + rn;
	}

	return response;
}

string optionReq()
{
	string response;
	string rn = "\r\n";

	response = "HTTP/1.1 204 No Content" + rn + "Methods: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE" + rn
		+ "Content-length: 0" + rn + rn;

	return response;
}

string putReq(int index, SocketState* sockets)
{
	string rn = "\r\n";
	string FileName = findFile(string(sockets[index].buffer));
	string lang = whichLanguage(string(sockets[index].buffer));
	string resource_path = "C:\\temp\\example_files\\" + lang + "\\" + FileName;
	string tit = title(string(sockets[index].buffer));
	FILE* f = fopen(resource_path.c_str(), "a");
	string response = "";

	if (f != nullptr)
	{
		response = "HTTP/1.1 200 OK" + rn + "Resource updated successfully" + rn + rn;

		vector<char> lines;
		char c;
		do {
			c = fgetc(f);
			if (c != EOF)
			{
				lines.push_back(c);
			}
		} while (c != EOF);

		fclose(f);
		f = fopen(resource_path.c_str(), "w");

		fprintf(f, "%s\n", tit.c_str());

		for (const char& line : lines)
		{
			fprintf(f, "%s", line);
		}

		fclose(f);
	}

	else
	{
	response = string("HTTP/1.1 404 page not found" + rn + rn);
	}

	return response;
}

string postReq(int index, SocketState* sockets)
{
	string response;
	string rn = "\r\n";
	size_t bodyInd = getBodyIndex((string)sockets[index].buffer);
	cout << "Post - " << &sockets[index].buffer[bodyInd] << endl;
	response = "HTTP/1.1 200 OK" + rn + "Content-length: 0" + rn + rn;

	return response;
}

size_t getBodyIndex(string buffer)
{
	size_t index = 4;
	index += buffer.find("\r\n\r\n");
	return index;
}

string headReq(int index, SocketState* sockets)
{
	string rn = "\r\n";
	string FileName = findFile(string(sockets[index].buffer));
	string lang = whichLanguage(string(sockets[index].buffer));
	string resource_path = "C:\\temp\\example_files\\" + lang + "\\" + FileName;
	ifstream f;
	char getLines[255];
	string lines;
	f.open(resource_path.c_str());
	string response;
	char c;

	if (f)
	{
		response = "HTTP/1.1 200 OK ";// +rn + " ";
		while (f.getline(getLines, 255))
		{
			lines += getLines;
		}
		string len = to_string(lines.length());
		response += rn + "Content-type: html" + rn + "Content-length: " + len + rn + rn;
	}
	else
	{
		response = string("HTTP/1.1 404 page not found" + rn + rn);
	}

	f.close();
	return response;
}

string getReq(int index, SocketState* sockets)
{
	string rn = "\r\n";
	string FileName = findFile(string(sockets[index].buffer));
	string lang = whichLanguage(string(sockets[index].buffer));
	string resource_path = "C:\\temp\\example_files\\" + lang + "\\" + FileName;
	ifstream f;
	char getLines[255];
	string lines;
	f.open(resource_path.c_str());
	string response;
	char c;

	if (f)
	{
		response = "HTTP/1.1 200 OK ";// +rn + " ";
		while (f.getline(getLines, 255))
		{
			lines += getLines;
		}
		string len = to_string(lines.length());
		response += rn + "Content-type: html" + rn + "Content-length: " + len + rn + rn + lines;
	}
	else
	{
		response = string("HTTP/1.1 404 page not found" + rn + rn);
	}

	f.close();
	return response;
}

string findFile(string queryString)
{
	int numOfBorder = 0;
	int index_file = 0;
	int question_mark = 0;

	/*for (int i = 0; i < queryString.size(); i++) {
		if (queryString[i] == '') {
			numOfBorder++;
		}

		if (numOfBorder == 1) {
			index_file = i + 1;
			break;
		}
	}*/

	for (int i = 0; i < queryString.size(); i++)
	{
		if (queryString[i] == '?') {
			question_mark = i ;
			break;
		}
	}
	return queryString.substr(index_file, question_mark);

}

string whichLanguage(string queryString)
{
	bool found = false;
	int index_lang = 0;
	for (int i = 0; i < queryString.size() && !found; i++)
	{

		if (queryString[i] == '=') {
			found = true;
			index_lang = i + 1;
		}

	}

	if (found)
	{
		return queryString.substr(index_lang, 2);
	}
}

string title(string queryString)
{
	bool found = false;
	int index_title = 0;
	for (int i = 0; i < queryString.size(); i++)
	{
		if (found && queryString[i] == '=')
		{
			index_title = i + 1;
		}

		if (queryString[i] == '&') {
			found = true;
		}

	}

	if (found)
	{
		return queryString.substr(index_title, queryString.size() - 1);
	}
}

