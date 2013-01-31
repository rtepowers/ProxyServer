// Author: Ray Powers
// Date: Jan 31, 2013
// Filename: proxy.cpp

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
const unsigned short serverPort = 12050; // up to range 12099 for my own, or 17777
char* userName;
int sizeOfUserName = 0;
char* hostName;
int sizeOfHostName = 0;

void runServerRequest (int clientSock);
// Function handles server interactions and displays messages from server.
// pre: socket must exist and be open.
// post: none

int main(int argNum, char* argValues[]) {

  // Need to grab Command-line arguments and convert them to useful types
  // Initialize arguments with proper variables.
  if (argNum != 2){
    // Incorrect number of arguments
    cerr << "Incorrect number of arguments. Please try again." << endl;
    return -1;
  }
  char* localArg = argValues[c_UserNameIndex];
  int sizeOfArg = strlen(localArg);

  // Now we need to split our input into a username and hostname
  bool foundUserName = false;

  
  
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


  // Housekeeping
  delete [] userName;
  delete [] hostName;

  // Well done!
  return 0;
}

void runServerRequest (int clientSock) {
  
  // Local variables.
  struct hostent* host;
  int status;
  string s_userName;
  stringstream ss;
  ss << userName;
  ss >> s_userName;
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

  // Begin handling communication with Server.

  
  // Close it down!
  close(clientSock);
  exit(1);
}
