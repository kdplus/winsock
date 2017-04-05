#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

#define BUFFER_SIZE 1024000
#define FILE_NAME_MAX 1024
// #define PORT 10234

int main() {
  //get ip
  char IP_ADDRESS[100];
  memset(IP_ADDRESS, 0, 100);
  cout << "Connect to where (IP_ADDRESS): " << endl;
  cin.getline(IP_ADDRESS, sizeof(IP_ADDRESS));
  unsigned short int PORT;
  cout << "PORT: " << endl;
  cin >> PORT;
  cin.get();

  WSADATA WSA;
  SOCKET  clientSocket;
  struct  sockaddr_in serverAddr;
  int addrLen = 0;
  HANDLE hThread = NULL;
  int iResult = 0;
  char fileName[FILE_NAME_MAX];
  char buffer[BUFFER_SIZE];
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
  serverAddr.sin_port = htons(PORT);
  memset(serverAddr.sin_zero,0x00,8);

  //init windows socket dll
  if (WSAStartup(MAKEWORD(2, 2), &WSA) != 0) {
    cout << "Error at WSAStartup()" << endl;
    return -1;
  }

  //create socket
  clientSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (clientSocket == INVALID_SOCKET) {
    cout << "Create socket failed!" << endl;
    return -1;
  }

  //connect
  iResult = connect(clientSocket, (struct sockaddr*) &serverAddr, sizeof(serverAddr));
  if (iResult == SOCKET_ERROR) {
    cout << "Connect failed" << endl;
    closesocket(clientSocket);
    return -1;
  }
  cout << "Connected!" << endl;

  //send&recv
  while (true) {
    memset(fileName, 0, FILE_NAME_MAX);
    cout << "Input the file name: " << endl;
    cin.getline(fileName, sizeof(fileName));
    if ((fileName[0]) == 'q') {
      cout << "quiting.." << endl;
      break;
    }
    iResult = send(clientSocket, fileName, (int) strlen(fileName), 0);
    if (iResult == SOCKET_ERROR) {
      cout << "Send failed with error" << endl;
      closesocket(clientSocket);
      WSACleanup();
      return -1;
    }
    cout << "Bytes Sent: " << iResult << endl;
    FILE *f = fopen(fileName, "wb");
    if (f == NULL) {
      cout << "can't open the a file to write" << endl;
      return -1;
    } else {
      while ((iResult = recv(clientSocket, buffer, BUFFER_SIZE, 0)) > 0) {
        // cout << buffer << endl;
        // cout << iResult<<endl;
        if (iResult > 0 && buffer[0] == 'e' && buffer[1] == 'o' && buffer[2] == 'f')
            break;
        if (fwrite(buffer, sizeof(char), iResult, f) < iResult) {
          cout << "Write failed" << endl;
          break;
        }
      }
      cout << "Got file from server: " << fileName << endl;
    }

    fclose(f);
  }


  closesocket(clientSocket);
  WSACleanup();
  return 0;
}
