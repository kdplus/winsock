#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

#define BUFFER_SIZE 1024000
#define FILE_NAME_MAX 1024
#define PORT 10234
#define IP_ADDRESS "127.0.0.1"

int main() {
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
  while (true) {
    memset(fileName, 0, FILE_NAME_MAX);
    cout << "Input the file name: " << endl;
    cin.getline(fileName, sizeof(fileName));
    cout << (int) fileName[0] << endl;
    if ((int)(fileName[0]) == 27) break;
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
        // break;
        // if (feof(f)) break;
      }
      cout << "Got file from server: " << fileName << endl;
    }

    fclose(f);
  }


  closesocket(clientSocket);
  WSACleanup();
  return 0;
}
