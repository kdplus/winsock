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
DWORD WINAPI ClientThread(LPVOID lpParameter);

int main() {
  WSADATA WSA;
  SOCKET serverSocket, clientSocket;
  struct sockaddr_in serverAddr, clientAddr;
  int addrLen = 0;
  int iResult = 0;
  HANDLE hThread = NULL;
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
  serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket == INVALID_SOCKET) {
    cout << "Create socket failed!" << endl;
    return -1;
  }

  //bind
  iResult = bind(serverSocket,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
  if (iResult == SOCKET_ERROR) {
    cout << "Bind failed with error" << endl;
    closesocket(serverSocket);
    WSACleanup();
    return -1;
  }

  //ListenSocket
  if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
    cout << "Listen function failed with error" << endl;
    closesocket(serverSocket);
    WSACleanup();
    return -1;
  }
  cout << "Server is listening now" << endl;

  //Accept
  while(true) {
    addrLen = sizeof(clientAddr);
    clientSocket = accept(serverSocket, (struct sockaddr*) &clientAddr, &addrLen);
    if (clientSocket == INVALID_SOCKET) {
      cout << "Accept failed with error" << endl;
      closesocket(serverSocket);
      WSACleanup();
      return -1;
    }
    cout << "Client connected " << inet_ntoa(clientAddr.sin_addr) << "." << clientAddr.sin_port << endl;
    hThread = CreateThread(NULL, 0, ClientThread, (LPVOID)clientSocket, 0, NULL);
    CloseHandle(hThread);
  }

  closesocket(serverSocket);
  closesocket(clientSocket);
  WSACleanup();
  return 0;
}

DWORD WINAPI ClientThread(LPVOID ipParameter) {
  SOCKET clientSocket = (SOCKET)ipParameter;
  int iResult = 0;
  char buffer[BUFFER_SIZE];
  char file_name[FILE_NAME_MAX];
  // loop recive
  do {
    memset(buffer, 0, BUFFER_SIZE);
    memset(file_name, 0, FILE_NAME_MAX);
    iResult = recv(clientSocket, buffer, BUFFER_SIZE, 0);

    if (iResult > 0) {
        cout << "Bytes received: " << iResult << endl;
        strncpy(file_name, buffer, FILE_NAME_MAX);
        cout << "Client request " << file_name << endl;
        //open file
        FILE *f = fopen(file_name, "rb");
        if (f == NULL) {
          cout << "File open failed" << endl;
          return -1;
        } else {
          //send data
          memset(buffer, 0, BUFFER_SIZE);
          while((iResult = fread(buffer, sizeof(char), BUFFER_SIZE, f)) > 0) {
            iResult = send(clientSocket, buffer, iResult, 0);
            if (iResult == SOCKET_ERROR) {
              cout << "Send failed with error" << endl;
              closesocket(clientSocket);
              WSACleanup();
              return -1;
            }
            memset(buffer, 0, BUFFER_SIZE);
          }
          buffer[0] = 'e'; buffer[1] = 'o'; buffer[2] = 'f';
          iResult = send(clientSocket, buffer, 10, 0);
          fclose(f);
          cout << "File " << file_name << " sent!" << endl;
        }
    } else if (iResult == 0) {
      cout << "Connection closing..." << endl;
      closesocket(clientSocket);
      WSACleanup();
      return -1;
    } else {
      cout << "Recv failed" << endl;
      closesocket(clientSocket);
      WSACleanup();
      return -1;
    }
  } while (iResult > 0);
  return 0;
}
