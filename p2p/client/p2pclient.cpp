#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

#define BUFFER_SIZE 1024000
#define FILE_NAME_MAX 1024
#define MY_PORT 10235
#define MY_ADDRESS "127.0.0.1"
DWORD WINAPI ClientThread(LPVOID lpParameter);
DWORD WINAPI MyThread(LPVOID ipParameter);

int main() {
  //get ip to connect
  char IP_ADDRESS[100];
  memset(IP_ADDRESS, 0, 100);

  WSADATA WSA;
  SOCKET serverSocket, clientSocket, myclientSocket;
  struct sockaddr_in serverAddr, clientAddr, connectAddr;
  int addrLen = 0;
  int iResult = 0;
  HANDLE hThread = NULL;

  //for bind my own serverSocket
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = inet_addr(MY_ADDRESS);
  serverAddr.sin_port = htons(MY_PORT);
  memset(serverAddr.sin_zero,0x00,8);

  //init windows socket dll
  if (WSAStartup(MAKEWORD(2, 2), &WSA) != 0) {
    cout << "[Server]: Error at WSAStartup()" << endl;
    return -1;
  }

  //create socket
  serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket == INVALID_SOCKET) {
    cout << "[Server]: Create socket failed!" << endl;
    return -1;
  }

  //bind
  iResult = bind(serverSocket,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
  if (iResult == SOCKET_ERROR) {
    cout << "[Server]: Bind failed with error" << endl;
    closesocket(serverSocket);
    WSACleanup();
    return -1;
  }

  //ListenSocket
  if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
    cout << "[Server]: Listen function failed with error" << endl;
    closesocket(serverSocket);
    WSACleanup();
    return -1;
  }
  cout << "[Server]: Server is listening now" << endl;


  //to connect other peer
  unsigned short int PORT;
  cout << "[Client]: Connect to where (IP_ADDRESS): " << endl;
  cin.getline(IP_ADDRESS, sizeof(IP_ADDRESS));
  cout << "[Client]: PORT: " << endl;
  cin >> PORT;
  cin.get();
  connectAddr.sin_family = AF_INET;
  connectAddr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
  connectAddr.sin_port = htons(PORT);
  memset(serverAddr.sin_zero,0x00,8);

  //create myclient socket
  myclientSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (myclientSocket == INVALID_SOCKET) {
    cout << "[Client]: Create myclientSocket failed!" << endl;
    return -1;
  }

  //connect
  iResult = connect(myclientSocket, (struct sockaddr*) &connectAddr, sizeof(connectAddr));
  if (iResult == SOCKET_ERROR) {
    cout << "[Client]: Connect failed" << endl;
    closesocket(myclientSocket);
    return -1;
  }
  cout << "[Client]: Connected!" << endl;

  HANDLE tThread = CreateThread(NULL, 0, MyThread, (LPVOID)myclientSocket, 0, NULL);
  CloseHandle(tThread);

  //Accept
  while(true) {
    addrLen = sizeof(clientAddr);
    clientSocket = accept(serverSocket, (struct sockaddr*) &clientAddr, &addrLen);
    if (clientSocket == INVALID_SOCKET) {
      cout << "[Server]:Accept failed with error" << endl;
      closesocket(serverSocket);
      WSACleanup();
      return -1;
    }
    cout << "[Server]: Client " << inet_ntoa(clientAddr.sin_addr) << "." << clientAddr.sin_port << " connected you!" << endl;
    hThread = CreateThread(NULL, 0, ClientThread, (LPVOID)clientSocket, 0, NULL);
    CloseHandle(hThread);
  }

  closesocket(serverSocket);
  closesocket(clientSocket);
  closesocket(myclientSocket);
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
        cout << "[Server]: Bytes received: " << iResult << endl;
        strncpy(file_name, buffer, FILE_NAME_MAX);
        cout << "[Server]: Client request " << file_name << endl;
        //open file
        FILE *f = fopen(file_name, "rb");
        if (f == NULL) {
          cout << "[Server]: File open failed" << endl;
          return -1;
        } else {
          //send data
          memset(buffer, 0, BUFFER_SIZE);
          while((iResult = fread(buffer, sizeof(char), BUFFER_SIZE, f)) > 0) {
            iResult = send(clientSocket, buffer, iResult, 0);
            if (iResult == SOCKET_ERROR) {
              cout << "[Server]: Send failed with error" << endl;
              closesocket(clientSocket);
              WSACleanup();
              return -1;
            }
            memset(buffer, 0, BUFFER_SIZE);
          }
          buffer[0] = 'e'; buffer[1] = 'o'; buffer[2] = 'f';
          iResult = send(clientSocket, buffer, 10, 0);
          fclose(f);
          cout << "[Server]: File " << file_name << " sent!" << endl;
        }
    } else if (iResult == 0) {
      cout << "[Server]: Connection closing..." << endl;
      closesocket(clientSocket);
      WSACleanup();
      return -1;
    } else {
      cout << "[Server]: Recv failed" << endl;
      closesocket(clientSocket);
      WSACleanup();
      return -1;
    }
  } while (iResult > 0);
  return 0;
}

DWORD WINAPI MyThread(LPVOID ipParameter) {
  SOCKET myclientSocket = (SOCKET)ipParameter;
  char fileName[FILE_NAME_MAX];
  char buffer[BUFFER_SIZE];
  int iResult = 0;
  while (true) {
    memset(fileName, 0, FILE_NAME_MAX);
    cout << "[Client]: Input the file name: " << endl;
    cin.getline(fileName, sizeof(fileName));
    if ((fileName[0]) == 'q') {
      cout << "[Client]: quiting.." << endl;
      exit(0);
    }
    iResult = send(myclientSocket, fileName, (int) strlen(fileName), 0);
    if (iResult == SOCKET_ERROR) {
      cout << "[Client]: Send failed with error" << endl;
      closesocket(myclientSocket);
      WSACleanup();
      return -1;
    }
    cout << "[Client]: Bytes Sent: " << iResult << endl;
    FILE *f = fopen(fileName, "wb");
    if (f == NULL) {
      cout << "[Client]: can't open the a file to write" << endl;
      return -1;
    } else {
      while ((iResult = recv(myclientSocket, buffer, BUFFER_SIZE, 0)) > 0) {
        // cout << buffer << endl;
        // cout << iResult<<endl;
        if (iResult > 0 && buffer[0] == 'e' && buffer[1] == 'o' && buffer[2] == 'f')
            break;
        if (fwrite(buffer, sizeof(char), iResult, f) < iResult) {
          cout << "[Client]: Write failed" << endl;
          break;
        }
      }
      cout << "[Client]: Got file from server: " << fileName << endl;
    }
    fclose(f);
  }
}
