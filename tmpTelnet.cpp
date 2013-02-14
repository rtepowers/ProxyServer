// Author: Ray Powers
// Date: Jan 31, 2013
// Filename: tmpTelnet.cpp

// DESCRIPTION: This program will provide a simple telnet-like clone


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

// Globals
const int c_UserNameIndex = 1;
unsigned short serverPort; // up to range 12099 for my own, or 17777
char* hostName;
int sizeOfHostName = 0;

void runServerRequest (int clientSock);
// Function handles server interactions and displays messages from server.
// pre: socket must exist and be open.
// post: none

int main(int argNum, char* argValues[]) {

  // Local Variables
  char* localArg;
  int sizeOfArg;

  // Need to grab Command-line arguments and convert them to useful types
  // Initialize arguments with proper variables.
  if (argNum < 3 || argNum > 3){
    // Incorrect number of arguments
    cerr << "Incorrect number of arguments. Please try again." << endl;
    return -1;
  }
  localArg = argValues[c_UserNameIndex];
  sizeOfArg = strlen(localArg);

  // Need to store arguments
  hostName = argValues[1];
  serverPort = atoi(argValues[2]);

  // Create a socket and start server communications.
  int conn_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (conn_socket > 0){
    // Socket was successfully opened.
    // Run the game portion.
    runServerRequest(conn_socket);
    close(conn_socket);
  } else {
    // Socket was unsuccessful.
    cerr << "Socket was unable to be opened." << endl;
    exit(-1);
  }

  // Well done!
  return 0;
}

void runServerRequest (int clientSock) {
  
  // Local variables.
  struct hostent* host;
  int status;
  int bytesRecv;

  // Get host IP and Set proper fields
  host = gethostbyname(hostName);
  if (!host) {
    cerr << "Unable to resolve hostname's ip address. Exiting..." << endl;
    return;
  }
  char* tmpIP = inet_ntoa( *(struct in_addr *)host->h_addr_list[0]);
  unsigned long serverIP;
  status = inet_pton(AF_INET, tmpIP,(void*) &serverIP);
  if (status <= 0) exit(-1);

  struct sockAddress_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = serverIP ;
  serverAddress.sin_port = htons(serverPort);

  // Now that we have the proper information, we can open a connection.
  status = connect(clientSock, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
  if (status < 0) {
    cerr << "Error with the connection." << endl;
    exit(-1);
  }
  
  // Output message to user 
  cout << "Trying " << inet_ntoa(*(in_addr *)host->h_addr) << "..." << endl;
  cout << "Connected to " << inet_ntoa(*(in_addr *)host->h_addr) << "." << endl;
  cout << "Ctrl-c to escape." << endl << endl;

  // Begin handling communication with Server.
  bool finished = false;
  int numReturns = 0;
  string msgToSend = "GET /index.html HTTP/1.0";
  while (!finished) {
    getline(cin, msgToSend, '\n');
    msgToSend.append("\n");
    int msgLength = msgToSend.length();
    char msgBuff[msgLength];
    strcpy(msgBuff, msgToSend.c_str());

    if (msgToSend.length() == 1) {
      numReturns++;
      if (numReturns >= 1) {
	// we're done here
	finished = true;
	msgToSend = "\n";
	msgLength = msgToSend.length();
	strcpy(msgBuff, msgToSend.c_str());
      }
    }
    // Send Data
    int msgSent = send(clientSock, msgBuff, msgLength, 0);
    if (msgSent != msgLength){
      // Failed to send
      cerr << "Unable to send data. Closing clientSocket: " << clientSock << "." << endl;
      close(clientSock);
      exit(-1);
    }
  }

  // Read Data
  int bufferSize = 1024;
  int bytesLeft = bufferSize;
  char buffer[bufferSize];
  char* buffPTR = buffer;
  int realSize = 0;
  while ((bytesRecv = recv(clientSock, buffPTR, bufferSize, 0)) > 0){
    //buffPTR = buffPTR + bytesRecv;
    //bytesLeft = bytesLeft - bytesRecv;
    //realSize += bytesRecv;
    write(1,buffer, bytesRecv);
  }
  cout << endl;
  
}
