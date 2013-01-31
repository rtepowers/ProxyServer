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

bool SendInteger(int clientSock, int hostInt);
// Function sends a network long variable over the network.
// pre: socket must exist
// post: none

long GetInteger(int clientSock);
// Function listens to socket for a network Long variable. 
// pre: socket must exist.
// post: none

bool SendMessage(int clientSock, string msgToSend);
// Function sends a message over network.
// pre: Socket must exist and should have paired this function with SendInt(MSG.length()+1)
// post: none.

string GetMessage(int clientSock, long messageLength);
// Function listens to socket for a variable length message.
// pre: socket must exist.
// post: none


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
  for (int i = 0; i < sizeOfArg; i++) {
    if (localArg[i] == '@' && !foundUserName) {
      // We found the delimiter '@' so let's split the argument up.
      sizeOfUserName = (i + 1);
      userName = new char[sizeOfUserName];
      for (int j = 0; j < i; j++) {
	// Copy over char by char the UserName
	userName[j] = localArg[j];
      }
      foundUserName = true;

      hostName = new char[sizeOfArg - ( i+ 1)];
      sizeOfHostName = (sizeOfArg - (i+ 1));
      continue;
    }
    if (foundUserName) {
      // Copy over rest of argument into hostName.
      hostName[i - sizeOfUserName] = localArg[i];
    }
  }
  
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


  delete [] userName;
  delete [] hostName;
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
  SendInteger(clientSock, (long)s_userName.length());
  SendMessage(clientSock, s_userName);

  // Begin handling communication with Server.
  SendInteger(clientSock, (long)s_userName.length());
  SendMessage(clientSock, s_userName);

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
  
  close(clientSock);
  exit(1);
}

long GetInteger(int clientSock){

  // Retreive length of msg
  int bytesLeft = sizeof(long);
  long networkInt;
  char* bp = (char *) &networkInt;
  
  while (bytesLeft) {
    int bytesRecv = recv(clientSock, bp, bytesLeft, 0);
    if (bytesRecv <= 0) exit(-1);
    bytesLeft = bytesLeft - bytesRecv;
    bp = bp + bytesRecv;
  }
  return ntohl(networkInt);
}

bool SendInteger(int clientSock, int hostInt){

  // Local Variables
  long networkInt = htonl(hostInt);

  // Send Integer (as a long)
  int didSend = send(clientSock, &networkInt, sizeof(long), 0);
  if (didSend != sizeof(long)){
    // Failed to Send
    cerr << "Unable to send data. Aborting program." << endl;
    exit(-1);
  }

  return true;
}

string GetMessage(int clientSock, long messageLength){
  
  // Retrieve msg
  int bytesLeft = messageLength;
  char buffer[messageLength];
  char* buffPTR = buffer;
  while (bytesLeft > 0){
    int bytesRecv = recv(clientSock, buffPTR, messageLength, 0);
    if (bytesRecv <= 0) exit(-1);
    bytesLeft = bytesLeft - bytesRecv;
    buffPTR = buffPTR + bytesRecv;
  }

  return buffer;
}

bool SendMessage(int clientSock, string msgToSend){

  // Local Variables
  int msgLength = msgToSend.length()+1;
  char msgBuff[msgLength];
  strcpy(msgBuff, msgToSend.c_str());
  msgBuff[msgLength-1] = '\0';

  // Since they now know how many bytes to receive, we'll send the userName
  int msgSent = send(clientSock, msgBuff, msgLength, 0);
  if (msgSent != msgLength){
    // Failed to send
    cerr << "Unable to send username. Aborting program." << endl;
    exit(-1);
  }

  return true;
}

