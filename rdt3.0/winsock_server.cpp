#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include <time.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

#define BUFFER_SIZE 1024000
#define FILE_NAME_MAX 1024
#define PORT 10234
#define IP_ADDRESS "127.0.0.1"
int state, rcvState = 0;
int timeout = 2;
SOCKET serverSocket, clientSocket;
struct sockaddr_in serverAddr, clientAddr;
int addrLen = 0;
char rcvpkt[BUFFER_SIZE] = {0};
int filEnd = 0, turnoffRcv = 0;
HANDLE rcThread;
clock_t file_start, file_end;


int get_data(char* buffer, FILE *f) {
  memset(buffer, 0, BUFFER_SIZE);
  int iResult = fread(buffer, sizeof(char), 255, f);
  if (iResult <= 0) {
    buffer[0] = 'e'; buffer[1] = 'o'; buffer[2] = 'f';
    fclose(f);
    return 1;
  }
  return 0;
}


void make_pkt(int order, char* buffer, char* pkt) {
  int checksum = 0;
  pkt[0] = order + '0';
  for(int i = 0; i <= 255; ++i) pkt[i+1] = buffer[i];
  // checksum
  for(int i = 0; i <= 255; i += 2) {
    int sum = (int)buffer[i] << 8;
    sum |= (int)buffer[i+1];
    checksum += sum;
    while(checksum > ((1<<16) - 1) ){
      int carry = checksum / (1 << 16);
      checksum = checksum % (1 << 16) + carry;
    }
    checksum &= ((1<<16) - 1);
  }
  int ans = (~checksum) & ((1 << 16)-1);
  pkt[257] = (char)(ans / (1<<8));
  pkt[258] = (char)(ans % (1<<8));
}


int notcorrupt(char* pkt){
  // rec check part
  int checksum = 0;
  for(int i = 1; i <= 258; i += 2) {
    int sum;
    if (i >= 257) {
      sum = (((int)pkt[i]) < 0 ? 256+((int)pkt[i]) : (int)pkt[i]) << 8;
      sum |=  ((int)pkt[i+1]) < 0 ? 256+((int)pkt[i+1]) : (int)pkt[i+1];
    } else {
      sum = (int)pkt[i] << 8;
      sum |= (int)pkt[i+1];
    }
    checksum += sum;
    while(checksum > ((1<<16) - 1) ){
      int carry = checksum / (1 << 16);
      checksum = checksum % (1 << 16) + carry;
    }
    checksum &= ((1<<16) - 1);
  }
  if (checksum - ((1 << 16) - 1) == 0) return 1;
  return 0;
}


int isACK(char* pkt, int order){
  if (order == 2 && pkt[1] == 'A' && pkt[2] == 'C' && pkt[3] == 'K') return 1;
  if (pkt[0] == order + '0' && pkt[1] == 'A' && pkt[2] == 'C' && pkt[3] == 'K') return 1;
  return 0;
}


DWORD WINAPI FSM(LPVOID Parameter) {
  state = 0;
  FILE *f = (FILE *) Parameter;
  char buffer[BUFFER_SIZE];
  char pkt[BUFFER_SIZE];
  clock_t timer_start, timer_end;
  while(true){
    // cout << "STATE: " << state << endl;
    if (state == 0) {
      if (rcvState == 1) {
        state = 0;
        rcvState = 0;
        continue;
      }
      filEnd = get_data(buffer, f);
      make_pkt(0, buffer, pkt);
      int iResult = sendto(serverSocket, pkt, 259, 0, (SOCKADDR *) & clientAddr, addrLen);
      if (iResult <= 0) cout << "Something wrong!" << endl;
      timer_start = clock();
      // cout << "timer_start: " << timer_start;
      state = 1;
      continue;
    }
    if (state == 1) {
      while(true){
        if (rcvState == 1 && notcorrupt(rcvpkt) && isACK(rcvpkt, 0)) {
          rcvState = 0;
          break;
        }
        timer_end = clock();
        if ((rcvState == 1 && (!notcorrupt(rcvpkt) || isACK(rcvpkt, 1))) || ((double)(timer_end - timer_start)/CLOCKS_PER_SEC * 1000 >= timeout)) {
          if ((double)(timer_end - timer_start)/CLOCKS_PER_SEC * 1000  >= timeout) cout << "timeout" << endl;
          int iResult = sendto(serverSocket, pkt, 259, 0, (SOCKADDR *) & clientAddr, addrLen);
          if (iResult <= 0) cout << "Something wrong!" << endl;
          rcvState = 0;
          timer_start = clock();
          continue;
        }
      }
      // Sleep(1);
      timer_end = clock();
      // cout << "  timer_end: " << timer_end << endl;
      // cout << "Time: " << (double) (timer_end - timer_start)/CLOCKS_PER_SEC* 1000 << endl;
      if (filEnd) {
        cout << "filEnd " << filEnd << endl;
        turnoffRcv = 1;
        TerminateThread(rcThread, 12345);
        break;
      }
      state = 2;
      continue;
    }
    if (state == 2) {
      if (rcvState == 1) {
        state = 2;
        rcvState = 0;
        continue;
      }
      filEnd = get_data(buffer, f);
      make_pkt(1, buffer, pkt);
      int iResult = sendto(serverSocket, pkt, 259, 0, (SOCKADDR *) & clientAddr, addrLen);
      if (iResult <= 0) cout << "Something wrong!" << endl;
      timer_start = clock();
      // cout << "timer_start: " << timer_start;
      state = 3;
      // Sleep(10);
      continue;
    }
    if (state == 3) {
      while(true){
        timer_end = clock();
        if ((rcvState == 1 && (!notcorrupt(rcvpkt) || isACK(rcvpkt, 0))) || ((double)(timer_end - timer_start)/CLOCKS_PER_SEC * 1000 >= timeout)) {
          if ((double)(timer_end - timer_start)/CLOCKS_PER_SEC * 1000  >= timeout) cout << "timeout" << endl;
          int iResult = sendto(serverSocket, pkt, 259, 0, (SOCKADDR *) & clientAddr, addrLen);
          if (iResult <= 0) cout << "Something wrong!" << endl;
          rcvState = 0;
          timer_start = clock();
          continue;
        }
        if (rcvState == 1 && notcorrupt(rcvpkt) && isACK(rcvpkt, 1)) {
          rcvState = 0;
          break;
        }
      }
      timer_end = clock();
      // cout << "Time: " << (double) (timer_end - timer_start)/CLOCKS_PER_SEC * 1000 << endl;
      if (filEnd) {
        TerminateThread(rcThread, 12345);
        turnoffRcv = 1;
        break;
      }
      state = 0;
      continue;
    }
  }
  return 0;
}


DWORD WINAPI reciveThread(LPVOID Parameter) {
  int iResult;
  while ((iResult = recvfrom(serverSocket, rcvpkt, 259, 0, (sockaddr *)&clientAddr, &addrLen)) > 0) {
    if (iResult > 0) rcvState = 1;
    while(rcvState);
    if (turnoffRcv) break;
  }
  cout << "reciveThread End" << endl;
  return 0;
}


int main() {
  WSADATA WSA;
  int iResult = 0;
  char file_name[FILE_NAME_MAX];
  char buffer[BUFFER_SIZE];
  HANDLE hThread = NULL;
  serverAddr.sin_family = AF_INET;
  // serverAddr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
  serverAddr.sin_addr.S_un.S_addr = inet_addr(IP_ADDRESS);
  serverAddr.sin_port = htons(PORT);
  memset(serverAddr.sin_zero,0x00,8);

  //init windows socket dll
  if (WSAStartup(MAKEWORD(2, 2), &WSA) != 0) {
    cout << "Error at WSAStartup()" << endl;
    return -1;
  }

  //create socket
  serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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


  //Accept
  while(true) {
    addrLen = sizeof(clientAddr);
    char recvData[BUFFER_SIZE];
    int initState = 0, ret;
    // cout << rcvpkt << '   ' << endl;
    ret = recvfrom(serverSocket, recvData, BUFFER_SIZE, 0, (sockaddr *)&clientAddr, &addrLen);
    // cout << recvData << endl;
    // if (isACK(recvData, 2)){
    //   // cout << rcvpkt << endl;
    //   ret = recvfrom(serverSocket, recvData, BUFFER_SIZE, 0, (sockaddr *)&clientAddr, &addrLen);
    //   if (ret <= 0) {
    //     cout << "some problem" << endl;
    //   }
    //   cout << "in the while " << recvData << endl;
    // }
    // cout << "Not ack " << endl;
    if (ret > 0) {
      recvData[ret] = 0x00;
      printf("A Connection: %s \r\n", inet_ntoa(clientAddr.sin_addr));
      // printf("%s", recvData);
    }
    if (ret > 0) {
        strncpy(file_name, recvData, FILE_NAME_MAX);
        cout << "Client request " << file_name << endl;
        //open file
        FILE *f = fopen(file_name, "rb");
        if (f == NULL) {
          cout << "File open failed" << endl;
          return -1;
        } else {
          file_start = clock();
          filEnd = 0;
          turnoffRcv = 0;
          rcThread = CreateThread(NULL, 0, reciveThread, (LPVOID)0, 0, NULL);
          hThread = CreateThread(NULL, 0, FSM, (LPVOID)f, 0, NULL);
          CloseHandle(hThread);
          while(!turnoffRcv);
          cout << file_name << " sent" << endl;
          fclose(f);
          file_end = clock();
          cout << "This used " << (double)(file_end - file_start)/CLOCKS_PER_SEC << " Second" << endl;
          cout << endl;
        }
    }
  }

  closesocket(serverSocket);
  closesocket(clientSocket);
  WSACleanup();
  return 0;
}
