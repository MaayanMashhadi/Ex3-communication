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
#include <vector>
#include <cstdio>

using namespace std;
const int PORT = 27015;
const int MAX_SOCKETS = 60;
const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;
const int HEAD = 5;
const int PUT = 6;
const int POST = 7;
const int DELETE_ = 8;
const int OPTIONS = 9;
const int TRACE = 10;
const int GET = 11;


struct SocketState
{
	time_t timer;
	SOCKET id;
	int recv;
	int send;
	int sendSubType; 
	char buffer[3000];
	int len;
};

string title(string queryString);
int getSubType(string str);
bool addSocket(SOCKET id, int what, SocketState* sockets, int& socketsCount);
void removeSocket(int index, SocketState* sockets, int& socketsCount);
void acceptConnection(int index, SocketState* sockets, int& socketsCount);
void receiveMessage(int index, SocketState* sockets, int& SocketCount);
void sendMessage(int index, SocketState* sockets);
size_t getBodyIndex(string buffer);
string whichFile(string queryString);
string traceReq(int index, SocketState* sockets);
string deleteReq(int index, SocketState* sockets);
string optionReq();
string putReq(int index, SocketState* sockets);
string postReq(int index, SocketState* sockets);
string headReq(int index, SocketState* sockets);
string getReq(int index, SocketState* sockets);
string findFile(string queryString);
string whichLanguage(string queryString);
bool checkIfTimeout(int index, SocketState* sockets);

void main()
{
	struct SocketState sockets[MAX_SOCKETS] = { 0 };
	int socketsCount = 0;

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
		cout << "Server: Error at WSAStartup()\n";
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
		cout << "Server: Error at socket(): " << WSAGetLastError() << endl;
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
	serverService.sin_port = htons(PORT);

	// Bind the socket for client's requests.

	// The bind function establishes a connection to a specified socket.
	// The function uses the socket handler, the sockaddr structure (which
	// defines properties of the desired connection) and the length of the
	// sockaddr structure (in bytes).
	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR*)&serverService, sizeof(serverService)))
	{
		cout << "Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}

	// Listen on the Socket for incoming connections.
	// This socket accepts only one connection (no pending connections 
	// from other clients). This sets the backlog parameter.
	if (SOCKET_ERROR == listen(listenSocket, 5))//line need to be significantly small
	{
		cout << "Server: Error at listen(): " << WSAGetLastError() << endl;
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
		
		
		fd_set waitRecv;
		FD_ZERO(&waitRecv);

		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (checkIfTimeout(i, sockets))
			{
				removeSocket(i, sockets, socketsCount);
			}
			else if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, &timeOut);
		if (nfd == SOCKET_ERROR)
		{
			cout << "Server: Error at select(): " << WSAGetLastError() << endl;
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
	cout << "Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}

bool checkIfTimeout(int index, SocketState* sockets)
{
	time_t time_ = time(NULL);
	if ((sockets[index].timer != 0 ) && (time_ - sockets[index].timer > 120))
	{
		return true;
	}
	else
	{
		return false;
	} 
}

bool addSocket(SOCKET id, int what, SocketState* sockets, int& socketsCount)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].timer = time(NULL);
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			socketsCount++;
			return (true);
		}
	}
	return (false);
}

void removeSocket(int index, SocketState* sockets, int& socketsCount)
{
	sockets[index].timer = 0;
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	socketsCount--;
}

void acceptConnection(int index, SocketState* sockets, int& socketsCount)
{
	sockets[index].timer = time(NULL);
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr*)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	unsigned long flag = 1;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
	{
		cout << "Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
	}

	if (addSocket(msgSocket, RECEIVE, sockets, socketsCount) == false)
	{
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

void receiveMessage(int index, SocketState* sockets, int& SocketCount)
{
	SOCKET msgSocket = sockets[index].id;
	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);

	if (SOCKET_ERROR == bytesRecv || bytesRecv == 0)
	{
		if (bytesRecv == 0)
		{
			cout << "Server: Error at recv(): " << WSAGetLastError() << endl;
		}
		closesocket(msgSocket);
		removeSocket(index, sockets, SocketCount);
		return;
	}
	else
	{
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		cout << "Server: Recieved: " << bytesRecv << " bytes of \"" << &sockets[index].buffer[len] << "\" message.\n";

		sockets[index].len += bytesRecv;

		if (sockets[index].len > 0)
		{
			string buffer = (string)sockets[index].buffer;
			string req = buffer.substr(0, buffer.find(" "));
			int len = req.length() + 2;

			sockets[index].len -= len;
			sockets[index].send = SEND;
			sockets[index].sendSubType = getSubType(req);
			memcpy(sockets[index].buffer, &sockets[index].buffer[len], sockets[index].len);
			sockets[index].buffer[sockets[index].len] = '\0';
			sockets[index].timer = time(NULL);
		}
	}
}

void sendMessage(int index, SocketState* sockets)
{
	sockets[index].timer = time(NULL);
	int bytesSent = 0;
	char sendBuff[500];
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
	memset(sockets[index].buffer, 0, 500);
	sockets[index].len = 0;

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
		currentTime = time(NULL);
		if ((currentTime - sockets[i].timer > 120) && (sockets[i].timer != 0))
		{
			removeSocket(i, sockets, socketsCount);
		}
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

string traceReq(int index, SocketState* sockets)
{
	string response;
	string rn = "\r\n";
	int len = strlen(sockets[index].buffer);
	len += strlen("TRACE ");

	response = "HTTP/1.1 200 OK" + rn + "Content-type: message" + rn + "Content-length: " + to_string(len) + rn + rn + 
		"TRACE " + sockets[index].buffer;

	return response;
}

string deleteReq(int index, SocketState* sockets)
{
	string response;
	string rn = "\r\n";
	string FileName = findFile(string(sockets[index].buffer));
	string lang = whichLanguage(string(sockets[index].buffer));
	string type = whichFile(string(sockets[index].buffer));
	string resource_path = "C:\\temp\\example_files\\" + lang + "\\" + FileName;
	fstream f;
	char getLines[255];
	string lines;
	f.open(resource_path.c_str());
	while (f.getline(getLines, 255))
	{
		lines += getLines;
	}

	f.close();
	string len = to_string(lines.size());


	if (remove(resource_path.c_str()) == 0) {
		response = "HTTP/1.1 200 OK" + rn + "Content-type: " + type + rn + "Content-length: " + len + rn + rn + lines;
	}

	else
	{
		response = "HTTP/1.1 204 No Content" + rn + rn;
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
	string type = whichFile(string(sockets[index].buffer));
	string resource_path = "C:\\temp\\example_files\\" + lang + "\\" + FileName;
	fstream f;
	string response;
	f.open(resource_path.c_str());
	char getLines[255];
	string lines;

	if (!f.is_open())
	{
		f.open(resource_path.c_str(), ios_base::out);
		response = "HTTP/1.1 201 Created" + rn + "Content-type: " + type + rn + " Content-length: 0" + rn + rn;
	}
	else
	{
		response = "HTTP/1.1 200 OK ";

		while (f.getline(getLines, 255))
		{
			lines += getLines;
		}

		f.close();
		f.open(resource_path.c_str());
		f << "hi from user" << endl;

		for (const char& c : lines)
		{
			f << c << endl;
		}
		string len = to_string(lines.size());
		response += rn + "Content-type: " + type + rn + " Content-length: " + len + rn + rn + lines;


	}

	f.close();
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
	string type = whichFile(string(sockets[index].buffer));
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
		response += rn + "Content-type: " + type + rn + "Content-length: " + len + rn + rn;
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
	string type = whichFile(string(sockets[index].buffer));
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
		response += rn + "Content-type: " + type + rn + "Content-length: " + len + rn + rn + lines;
	}
	else
	{
		response = string("HTTP/1.1 404 page not found" + rn + rn);
	}

	f.close();
	return response;
}

string whichFile(string queryString)
{
	int index_kind = 0;
	for (int i = 0; i < queryString.size(); i++) {
		if (queryString[i] == '.') {
			index_kind = i + 1;
			break;
		}
	}

	if (queryString.substr(index_kind, 4)._Equal("html")) {

		return "html";
	}
	else {
		return "txt";
	}
}

string findFile(string queryString)
{
	int numOfBorder = 0;
	int index_file = 0;
	int question_mark = 0;

	for (int i = 0; i < queryString.size(); i++)
	{
		if (queryString[i] == '?' || queryString[i] == ' ') {
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
	else
	{
		return "en";
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

