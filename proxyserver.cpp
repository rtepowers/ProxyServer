// AUTHOR: Ray Powers, Russel Asher, Nichole Minas
// DATE: February 12, 2013
// FILE: ProxyServer.cpp

// DESCRIPTION: This program will serve as a caching proxy for multiple clients.

#include<iostream>
#include<sstream>
#include<string>
#include<cstring>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<netdb.h>
#include<pthread.h>
#include<tr1/unordered_map>
#include<ctime>

using namespace std;

// Data Structures
struct sockAddr {
  unsigned short sa_family; // Address family (AF_INET)
  char sa_data[14]; // Protocol-specific addressinfo
};

struct in_address {
  unsigned long s_addr; // Internet Address (32bits)
};

struct sockAddress_in {
  unsigned short sin_family; // Address family (AF_INET)
  unsigned short sin_port; // Port (16bits)
  struct in_addr sin_addr; // Internet address structure
  char sin_zero[8]; // Not Used.
};

struct threadArgs {
  int clientSock;
};

// Globals
const int MAXPENDING = 25;
unsigned short serverPort; // up to range 12099 for my own, or 17777
pthread_mutex_t cacheLock;
int status = pthread_mutex_init(&cacheLock, NULL);
tr1::unordered_map<string, string> cacheMap;
int numCacheItems = 0;
const int MAXCACHESIZE = 100;

// Function Prototypes
void* clientThread(void* args_p);
// Function allows program to handle multiple threads.
// pre: args_p should carry a socket.
// post: thread will detach and self terminate.

void runServerRequest(int clientSock);
// Function handles the browser's requests.
// pre: none
// post: none

string getCacheControl(string httpMsg);
// Function gets cachecontrol value from httpResponse
// pre: none
// post: none

bool SendMessageStream(int hostSock, string Msgss);
// Function sends stream to socket.
// pre: none
// post: none

string GetMessageStream(int clientSock, bool isHost);
// Function reads stream from socket.
// pre: none
// post: none

string HostProcessing (string clientMsg);
// Function handles host stream send and receive to sockets
// pre: none
// post: none

string getHost(string httpMsg);
// Function extracts the Host: value from an httpMsg
// pre: none
// post: none

string getReqType(string httpMsg);
// Function extracts the Message type from an httpMsg
// pre: none
// post: none

string getUri(string httpMsg);
// Function extracts the URI from an httpMsg
// pre: none
// post: none

void addToCache (string request, string response);
// Function adds item to cache.
// pre: none
// post: none

string checkCache (string httpMsg);
// Function checks cache for item
// pre: none
// post: none

string getErrorMsg();
// Function returns the standard error message.
// pre: none
// post: none

int main(int argNum, char* argValues[]) {

  // Need to grab Command-line arguments and convert them to useful types
  // Initialize arguments with proper variables.
  if (argNum != 2){
    // Incorrect number of arguments
    cerr << "Incorrect number of arguments. Please try again." << endl;
    return -1;
  }

  // Need to store arguments
  serverPort = atoi(argValues[1]);

  // Create socket connection
  int conn_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (conn_socket < 0){
    cerr << "Error with socket." << endl;
    exit(-1);
  }

  // Set the socket Fields
  struct sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET; // Always AF_INET
  serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  serverAddress.sin_port = htons(serverPort);
  
  // Assign Port to socket
  int sock_status = bind(conn_socket,
			 (struct sockaddr *) &serverAddress,
			 sizeof(serverAddress));
  if (sock_status < 0) {
    cerr << "Error with bind." << endl;
    exit(-1);
  }

  // Set socket to listen.
  int listen_status = listen(conn_socket, MAXPENDING);
  if (listen_status < 0) {
    cerr << "Error with listening." << endl;
    exit(-1);
  }
  cout << endl << endl << "SERVER: Ready to accept connections. " << endl;
  
  // Accept connections
  while (true) {
    // Accept connections. No seriously, ACCEPT THEM!
    struct sockaddr_in clientAddress;
    socklen_t addrLen = sizeof(clientAddress);
    int clientSocket = accept(conn_socket, (struct sockaddr*) &clientAddress, &addrLen);
    if (clientSocket < 0) {
      cerr << "Error accepting connections." << endl;
      //break;
      
      continue;
    }

    // Create child thread to handle process
    struct threadArgs* args_p = new threadArgs;
    args_p -> clientSock = clientSocket;
    pthread_t tid;
    int threadStatus = pthread_create(&tid, NULL, clientThread, (void*)args_p);
    if (threadStatus != 0){
      // Failed to create child thread
      cerr << "Failed to create child process." << endl;
      close(clientSocket);
      pthread_exit(NULL);
    }
    
  }
  
  // Well done! Your computer managed to finish the infinite loop.
  return 0;
}

void* clientThread(void* args_p){
  
  // Local Variables
  threadArgs* tmp = (threadArgs*) args_p;
  int clientSock = tmp -> clientSock;
  delete tmp;

  // Detach Thread to ensure that resources are deallocated on return.
  pthread_detach(pthread_self());

  // Communicate with Client
  runServerRequest(clientSock);
 
  // Close Client socket
  close(clientSock);

  // Quit thread
  pthread_exit(NULL);
}

void runServerRequest(int clientSock) {

  // Local Variables
  string requestMsg;
  string responseMsg;

  // Get Browser message
  requestMsg = GetMessageStream(clientSock, false);
  
  // Process Browser Message
  // Check Cache
  //responseMsg = HostProcessing(requestMsg);
  responseMsg = checkCache(requestMsg);
  if (responseMsg == "") {
    // Didn't find in cache, try Host
    responseMsg = HostProcessing(requestMsg);
  }

  // Send back to Browser
  // Must use exception handling because browser can manually close connection.
  try {
  SendMessageStream(clientSock, responseMsg);
  } catch (...) {
    close(clientSock);
    pthread_exit(NULL);
  }
}

string HostProcessing (string clientMsg) {

  // Local Variables
  string responseMsg;
  struct hostent* host;
  struct sockAddress_in serverAddress;
  char* tmpIP;
  unsigned long hostIP;
  int status = 0;
  int hostSock;
  unsigned short hostPort = 80;
  string tmpMsg = clientMsg;
  string hostName = "";

  if (tmpMsg.find("Host: ") != string::npos) {
    hostName.append(tmpMsg,
		    tmpMsg.find("Host: ") + 6,
		    tmpMsg.find("\n", tmpMsg.find("Host: ")) -tmpMsg.find("Host: ")-7);
  }
  cout << "HOSTNAME: " << hostName << endl;
  // Get Host IP Address
  host = gethostbyname(hostName.c_str());
  if (!host) {
    cerr << "Unable to resolve hostname's IP Address. Exiting..." << endl;
    return getErrorMsg();
  }
  tmpIP = inet_ntoa(*(struct in_addr *)host ->h_addr_list[0]);
  cout << "IP Address: " << tmpIP << endl;
  status = inet_pton(AF_INET, tmpIP, (void*) &hostIP);
  if (status <= 0) pthread_exit(NULL);
  status = 0;

  // Establish socket and address to talk to Host
  hostSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = hostIP;
  serverAddress.sin_port = htons(hostPort);

  // Now we have the right information, let's open a connection to the host.
  status = connect(hostSock, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
  if (status < 0) {
    cerr << "Error opening host connection." << endl;
    pthread_exit(NULL);
  }

  // Forward Message
  SendMessageStream(hostSock, clientMsg);
  
  // Receive Response
  responseMsg =  GetMessageStream(hostSock, true);

  // Great Success!
  return responseMsg;
}

string GetMessageStream(int clientSock, bool isHost) {

  // Local Variables
  stringstream ss;
  int bufferSize = 1000; 
  int totalSize = 0;
  int bytesRecv;
  char buffer[bufferSize];
  char* buffPTR = buffer;
  time_t timer;
  time_t check;
  time(&timer);
  memset(buffPTR, '\0', bufferSize);

  // Handle communications
  while (true) {
    bytesRecv = recv(clientSock, (void*) buffPTR, bufferSize, 0);
    if (bytesRecv < 0) {
      cerr << "Error occured while trying to receive data." << endl;
      pthread_exit(NULL);
    } else if (bytesRecv == 0) {
      break;
    } else {
      totalSize += bytesRecv;
      for (int i = 0; i < bytesRecv; i++) {
	ss << buffPTR[i];
      }
      if (totalSize > 4 && !isHost){
	string tmpMsg = ss.str();
	if (tmpMsg[tmpMsg.length()-4] == '\r'
	    && tmpMsg[tmpMsg.length()-3] == '\n'
	    && tmpMsg[tmpMsg.length()-2] == '\r'
	    && tmpMsg[tmpMsg.length()-1] == '\n') {
	  break;
	}
      }
      time(&check);
      if (difftime(check,timer) > 4) {
	cout << "Time Failed: " << difftime(check, timer) << endl;
	close(clientSock);
	pthread_exit(NULL);
      }

    }
  }
  string tmpStr = ss.str();
  if (tmpStr.find("Connection: keep-alive") != string::npos)
    tmpStr.replace(tmpStr.find("Connection: keep-alive"), 24, "");
  // Return HttpRequestObj
  return tmpStr;
}

bool SendMessageStream(int hostSock, string Msgss) {

  //Local
  string messageToSend = Msgss;
  int msgLength = messageToSend.length();
  int msgSent = 0;
  char msgBuff[msgLength];

  // Transfer message.
  memcpy(msgBuff, messageToSend.c_str(), msgLength);

  // Send message
  while (msgSend != msgLength) {
  msgSent = send(hostSock,(void*) msgBuff, msgLength, 0);
  }
  
  return true;
}

string getHost(string httpMsg) {

  // Local Variables
  string hostName = "";

  // Store the whole host name
  if (httpMsg.find("Host: ") != string::npos) {
    hostName.append(httpMsg,
		    httpMsg.find("Host: ")+6,
		    httpMsg.find("\n",httpMsg.find("Host: ")) - httpMsg.find("Host: ") - 7);

  }
  // Return Host Name
  return hostName;
}


string getReqType(string httpMsg) {

  // Local
  string reqType = "";
  if (httpMsg.find(" ") != string::npos) {
    reqType.append(httpMsg, 0, httpMsg.find(" "));
  }
  return reqType;
}

string getUri(string httpMsg) {
  
  // Local Variables
  string url = "";

  // Process HTTP message
  if (httpMsg.find("https://") != string::npos){
    url.append(httpMsg,
	       httpMsg.find("GET https://") + 12,
	       httpMsg.find(" HTTP/1.0") - (httpMsg.find("GET https://") + 12));
  } else if (httpMsg.find("http://") != string::npos) {
    url.append(httpMsg,
	       httpMsg.find("GET http://") + 11,
	       httpMsg.find(" HTTP/1.0") - (httpMsg.find("GET http://") + 11));
  } else {
    url.append(httpMsg,
	       httpMsg.find("GET ") + 4,
	       httpMsg.find(" HTTP/1.0") - (httpMsg.find("GET ") + 4));
  }

  return url;
}

void addToCache (string request, string response) {
  
  string cacheKey = "";
  cacheKey.append (getReqType(request));
  cacheKey.append (getUri(request));
  cacheKey.append (getHost(request));

  // Critical Section
  pthread_mutex_lock(&cacheLock);
  try {
  if (cacheMap.size() < MAXCACHESIZE) {
    cacheMap.insert (make_pair<string, string>(cacheKey, response));
  } else {
    cacheMap.erase (cacheMap.begin());
    cacheMap.insert (make_pair<string, string>(cacheKey, response));
  }
  } catch (...) {
  }
  pthread_mutex_unlock(&cacheLock);
}

string checkCache (string httpMsg) {
  
  string cacheItem;
  string cacheKey = "";
  cacheKey.append (getReqType(httpMsg));
  cacheKey.append (getUri(httpMsg));
  cacheKey.append (getHost(httpMsg));

  // Critical Section
  pthread_mutex_lock(&cacheLock);
  if (cacheMap.size() != 0) {
    try {
      tr1::unordered_map<string,string>::const_iterator got = cacheMap.find(cacheKey);
      if (got == cacheMap.end()) {
	cacheItem = "";
      } else {
	cacheItem = got->second;
      }
    } catch (...) {
      cacheItem = "";
    }
  } else {
    cacheItem = "";
  }
  pthread_mutex_unlock(&cacheLock);

  return cacheItem;
}

string getErrorMsg() {

  stringstream ss;
  time_t rawtime;
  ss << "HTTP/1.0 408 Request Timeout\r\n";
  ss << "Content-Type: text/html; charset=UTF-8\r\n";
  ss << "X-Content-Type-Options: nosniff\r\n";
  ss << "Date: " << ctime(&rawtime) << "\r\n";
  ss << "Server: CSS2\r\n";
  ss << "Content-Length: 230\r\n";
  ss << "X-XSS-Protection: 1; mode=block\r\n";
  ss << "\r\n";
  ss << "<!DOCTYPE html>";
  ss << "<html lang=en>  \n<meta charset=utf-8>";
  ss << "<meta name=viewport content=\"initial-scale=1, minimum-scale=1, width=device-width\">\n";
  ss << "<title>Error 404 (Not Found)!!</title>\n";
  ss << "<\\html>";

  return ss.str();
}
