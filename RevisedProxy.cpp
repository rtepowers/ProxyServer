// AUTHOR: Ray Powers
// DATE: February 12, 2013
// FILE: ProxyServer.cpp

// DESCRIPTION: This program will serve as a caching proxy for multiple clients.

#include<iostream>
#include<sstream>
#include<string>
#include<cstring>
#include<iomanip>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<netdb.h>
#include<pthread.h>
#include<tr1/unordered_map>

using namespace std;

// Data Structures
struct sockAddr {
  unsigned short sa_family;   // Address family (AF_INET)
  char sa_data[14];           // Protocol-specific addressinfo
};

struct in_address {
  unsigned long s_addr;       // Internet Address (32bits)
};

struct sockAddress_in {
  unsigned short sin_family;  // Address family (AF_INET)
  unsigned short sin_port;    // Port (16bits)
  struct in_addr sin_addr;    // Internet address structure
  char sin_zero[8];           // Not Used.
};

struct threadArgs {
  int clientSock;
};

// Globals
const int MAXPENDING = 40;
const int c_UserNameIndex = 1;
unsigned short serverPort; // up to range 12099 for my own, or 17777
pthread_mutex_t cacheLock;
int status = pthread_mutex_init(&cacheLock, NULL);


// Function Prototypes
void* clientThread(void* args_p);
// Function allows program to handle multiple threads. 
// pre: args_p should carry a socket.
// post: thread will detach and self terminate.

void runServerRequest (int clientSock);
// Function handles server interactions and displays messages from server.
// pre: socket must exist and be open.
// post: none

string talkToHost(string hostName, string httpMsg, int &replyLength);
// Function initiates communication with a particular host. Returns host response.
// pre: hostName should be resolvable.
// post: returns response string

string getHostName(string httpMsg);
// Function searches message for host informaion.
// pre: none
// post: none

bool SendMessage(int clientSock, string msgToSend, int msgLength);
// Function sends http message to socket.
// pre: none
// post: none

string GetMessage(int clientSock);
// Function gets a message of unknown length.
// pre: none
// post: none

string GetMessage(int clientSock, int &messageLength);
// Function gets a message of specified length
// pre: none
// post: none

string makeHeadRequest(string httpMsg);
// Function makes a HEAD request from a GET request
// pre: none
// post: none

int getMessageLength(string httpMsg);
// Function parses http message for a content-length header.
// pre: none
// post: none

int main(int argNum, char* argValues[]) {

  // Local Variables
  tr1::unordered_map<string, string> cacheMap;
  

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
  serverAddress.sin_family = AF_INET;    // Always AF_INET
  serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  serverAddress.sin_port = htons(serverPort);
  
  // Assign Port to socket
  int sock_status = bind(conn_socket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
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
  cout << endl << "SERVER: Ready to accept connections. " << endl;
  
    // Accept connections
  while (true) {
    // Accept connections. No seriously, ACCEPT THEM!
    struct sockaddr_in clientAddress;
    socklen_t addrLen = sizeof(clientAddress);
    int clientSocket = accept(conn_socket, (struct sockaddr*) &clientAddress, &addrLen);
    if (clientSocket < 0) {
      cerr << "Error accepting connections." << endl;
      exit(-1);
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

void runServerRequest (int clientSock) {
  
  // Local variables.
  struct hostent* host;
  string hostName = "";
  string clientMsg;
  int status;
  int bytesRecv = 0;
  
  // Begin handling communication with Server.
  // Receive HTTP Message from client.
  clientMsg = GetMessage(clientSock);

  // Forward HTTP Message to host.
  string responseMsg = talkToHost(getHostName(clientMsg), clientMsg, bytesRecv);

  // Return HTTP Message to client.
  if (!SendMessage(clientSock, responseMsg, bytesRecv)){
    // An error occured.
    return;
  }

}

string talkToHost(string hostName, string httpMsg, int &replyLength){

  // Local Variables
  struct hostent* host;
  struct sockAddress_in serverAddress;
  char* tmpIP;
  string responseMsg = "";
  unsigned long hostIP;
  int status = 0;
  int hostSock;
  unsigned short hostPort = 80;

  // Get Host IP Address
  host = gethostbyname(hostName.c_str());
  if (!host) {
    cerr << "Unable to resolve hostname's IP Address. Exiting..." << endl;
    return responseMsg;
  }
  tmpIP = inet_ntoa(*(struct in_addr *)host ->h_addr_list[0]);
  status = inet_pton(AF_INET, tmpIP, (void*) &hostIP);
  if (status <= 0) exit(-1);
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
    return responseMsg;
  }

  // Forward Message
  SendMessage(hostSock, makeHeadRequest(httpMsg), makeHeadRequest(httpMsg).length());
  string headTmp = GetMessage(hostSock);
  int respLen = getMessageLength(headTmp);
  cout << "Message Length: " << respLen << endl;
  SendMessage(hostSock, httpMsg, httpMsg.length());

  // Receive Response
  responseMsg = GetMessage(hostSock);
  cout << responseMsg << endl;

  replyLength = responseMsg.length();
  // Great Success!
  return responseMsg;
}


// HTTP HELPER FUNCTIONS

string getHostName(string httpMsg){

  // Local Variables
  string hostName = "";

  // Store the whole host name
  if (httpMsg.find("Host: ") != 0) {
  hostName.append(httpMsg, 
		  httpMsg.find("Host: ")+6, 
		  httpMsg.find("\n",httpMsg.find("Host: ")) - httpMsg.find("Host: ") - 6);
  } else {
    return hostName;
  }

  // Remove spaces
  for(int i=0; i < hostName.length(); i++) {
    if ( hostName[i] == ' ') {
      hostName.replace(i,1, "");
      i--;
    } else if (hostName[i] == '\r') {
      hostName.replace(i,1, "");
      i--;
    } else if (hostName[i] == '\n') {
      hostName.replace(i,1, "");
      i--;
    }
  }

  // Return Host Name
  return hostName;
}

bool SendMessage(int clientSock, string msgToSend, int msgLength){

  // Local Variables
  char msgBuff[msgLength];
  strcpy(msgBuff, msgToSend.c_str());

  // Since they now know how many bytes to receive, we'll send the userName
  int msgSent = send(clientSock, msgBuff, msgLength, 0);
  if (msgSent != msgLength){
    // Failed to send
    cerr << "Unable to send data. Closing clientSocket: " << clientSock << "." << endl;
    return false;
  }

  return true;
}

char* GetMessage(int clientSock, int &messageLength){
  
  // Retrieve msg
  int bytesLeft = messageLength;
  char* buffer = new char[bytesLeft];
  char* buffPTR = buffer;
  while (bytesLeft > 0){
    int bytesRecv = recv(clientSock, buffPTR, messageLength, 0);
    if (bytesRecv < 0) {
      // Failed to Read for some reason.
      cerr << "Could not recv bytes. Closing clientSocket: " << clientSock << "." << endl;
      return "";
    }
    if (bytesRecv == 0) {
      // done!
      return buffer;
    }
    bytesLeft = bytesLeft - bytesRecv;
    buffPTR = buffPTR + bytesRecv;
  }

  
  return buffer;
}

string GetMessage(int clientSock) {

  // Local Variables
  string responseMsg = "";
  int bytesRecv;
  int contentSize = 0;
  char* restOfMessage;

  
  // Begin handling communication with Server.
  int bufferSize = 1024;
  int bytesLeft = bufferSize;
  int bytesReceived = 0;
  char buffer[bufferSize];
  char* buffPTR = buffer;
  while (true){
    if (responseMsg.length() > 4){
      if (responseMsg[responseMsg.length()-4] == '\r'
	  && responseMsg[responseMsg.length()-3] == '\n'
	  && responseMsg[responseMsg.length()-2] == '\r'
	  && responseMsg[responseMsg.length()-1] == '\n') {
	break;
      }
    }
    bytesRecv = recv(clientSock, buffPTR, bufferSize, 0);
    if (bytesRecv == 0) {
      break;
    } else if (bytesRecv < 0){
      cerr << "Problem occured receiving data" << endl;
      break;
    }
    responseMsg.append(buffPTR, bytesRecv);
    bytesReceived += bytesRecv;
  }

  return responseMsg;
}

string makeHeadRequest(string httpMsg) {

  // Local Variables
  return httpMsg.replace(httpMsg.find("GET "), 4, "HEAD ");
}

int getMessageLength(string httpMsg) {

  // Local Variables
  int msgLength = 0;
  string textLength = "";

  // parse httpMsg
  if (httpMsg.find("Content-Length: ") != 0) {
    textLength.append(httpMsg,
		      httpMsg.find("Content-Length: ")+ 16,
		      httpMsg.find("\n", httpMsg.find("Content-Length: "))-3);
  
    
    stringstream ss(textLength);
    ss >> msgLength;
  }
  return msgLength;
}
