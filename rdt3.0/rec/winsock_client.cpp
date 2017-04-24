#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

#define BUFFER_SIZE 1024000
#define FILE_NAME_MAX 1024
// #define PORT 10234
char ACK[BUFFER_SIZE] = {0};
int rcvState = 0;
char rcvpkt[BUFFER_SIZE] = {0};
char fileName[FILE_NAME_MAX];
SOCKET  clientSocket;
struct  sockaddr_in serverAddr;
int len, state = 0, iResult, filEnd = 0;

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


int has_seq(char* s, int order) {
  if (order + '0' == s[0]) return 1;
  return 0;
}


int notcorrupt(char* s){
  // rec check part
  int checksum = 0;
  for(int i = 1; i <= 258; i += 2) {
    int sum;
    if (i >= 257) {
      sum = (((int)s[i]) < 0 ? 256+((int)s[i]) : (int)s[i]) << 8;
      sum |=  ((int)s[i+1]) < 0 ? 256+((int)s[i+1]) : (int)s[i+1];
    } else {
      sum = (int)s[i] << 8;
      sum |= (int)s[i+1];
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


// DWORD WINAPI FSM(LPVOID Parameter) {
//   state = 0;
//   FILE *f = (FILE *) Parameter;
//   char buffer[BUFFER_SIZE];
//   char pkt[BUFFER_SIZE];
//   while(true){
//     // cout << "STATE: " << state << endl;
//     if (state == 0) {
//       while(!rcvState);
//       if(!rcvState || !notcorrupt(rcvpkt) || has_seq(rcvpkt, 1)) {
//         if (rcvState){
//           make_pkt(1, ACK, pkt);
//           sendto(clientSocket, pkt, 259, 0, (sockaddr *)&serverAddr, len);
//           rcvState = 0;
//         }
//         continue;
//       }
//       for(int i = 1; i <= 256; ++i) rcvpkt[i-1] = rcvpkt[i];
//       make_pkt(0, ACK, pkt);
//       sendto(clientSocket, pkt, 259, 0, (sockaddr *)&serverAddr, len);
//       if (iResult > 0 && rcvpkt[0] == 'e' && rcvpkt[1] == 'o' && rcvpkt[2] == 'f') break;
//       iResult = 255;
//       if (fwrite(rcvpkt, sizeof(char), iResult, f) < iResult) {
//         cout << "Write failed" << endl;
//         break;
//       }
//       rcvState = 0;
//       state = 1;
//       continue;
//     }
//     if (state == 1) {
//       while(!rcvState);
//       if(!rcvState || !notcorrupt(rcvpkt) || has_seq(rcvpkt, 0)) {
//         if (rcvState == 1){
//           make_pkt(0, ACK, pkt);
//           sendto(clientSocket, pkt, 259, 0, (sockaddr *)&serverAddr, len);
//           rcvState = 0;
//         }
//         continue;
//       }
//       for(int i = 1; i <= 256; ++i) rcvpkt[i-1] = rcvpkt[i];
//       make_pkt(1, ACK, pkt);
//       sendto(clientSocket, pkt, 259, 0, (sockaddr *)&serverAddr, len);
//       if (iResult > 0 && rcvpkt[0] == 'e' && rcvpkt[1] == 'o' && rcvpkt[2] == 'f') break;
//       iResult = 255;
//       if (fwrite(rcvpkt, sizeof(char), iResult, f) < iResult) {
//         cout << "Write failed" << endl;
//         break;
//       }
//       rcvState = 0;
//       state = 0;
//       continue;
//     }
//   }
//   cout << "Got file from server: " << fileName << endl;
//   rcvState = 0;
//   filEnd = 1;
//   fclose(f);
//   return 0;
// }


int main() {
  //get ip
  ACK[0] = 'A';
  ACK[1] = 'C';
  ACK[2] = 'K';
  char pkt[BUFFER_SIZE] = {0};
  char IP_ADDRESS[100];
  memset(IP_ADDRESS, 0, 100);
  cout << "Connect to where (IP_ADDRESS): " << endl;
  cin.getline(IP_ADDRESS, sizeof(IP_ADDRESS));
  unsigned short int PORT;
  cout << "PORT: " << endl;
  cin >> PORT;
  cin.get();

  WSADATA WSA;
  int addrLen = 0;
  HANDLE hThread = NULL;
  int iResult = 0;
  char buffer[BUFFER_SIZE];
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
  serverAddr.sin_port = htons(PORT);
  memset(serverAddr.sin_zero,0x00,8);
  len = sizeof(serverAddr);
  //init windows socket dll
  if (WSAStartup(MAKEWORD(2, 2), &WSA) != 0) {
    cout << "Error at WSAStartup()" << endl;
    return -1;
  }

  //create socket
  clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (clientSocket == INVALID_SOCKET) {
    cout << "Create socket failed!" << endl;
    return -1;
  }

  while(true) {
    memset(fileName, 0, FILE_NAME_MAX);
    cout << "Input the file name: " << endl;
    cin >> fileName;
    sendto(clientSocket, fileName, strlen(fileName), 0, (sockaddr *)&serverAddr, len);
    cout << "fileName " << fileName << endl;


    FILE *f = fopen(fileName, "wb");
    if (f == NULL) {
      cout << "can't open the a file to write" << endl;
      return -1;
    } else {
      // filEnd = 0;
      // hThread = CreateThread(NULL, 0, FSM, (LPVOID)f, 0, NULL);
      // CloseHandle(hThread);
      // HANDLE rcThread = CreateThread(NULL, 0, reciveThread, (LPVOID)0, 0, NULL);
      // CloseHandle(rcThread);
      state = 0;
        while ((iResult = recvfrom(clientSocket, rcvpkt, 259, 0, (sockaddr *)&serverAddr, &len)) > 0){
          if(!notcorrupt(rcvpkt) || has_seq(rcvpkt, 1-state)) {
            make_pkt(1-state, ACK, pkt);
            sendto(clientSocket, pkt, 259, 0, (sockaddr *)&serverAddr, len);
            rcvState = 0;
            continue;
          }
          for(int i = 1; i <= 256; ++i) rcvpkt[i-1] = rcvpkt[i];
          make_pkt(state, ACK, pkt);
          sendto(clientSocket, pkt, 259, 0, (sockaddr *)&serverAddr, len);
          if (iResult > 0 && rcvpkt[0] == 'e' && rcvpkt[1] == 'o' && rcvpkt[2] == 'f') break;
          iResult = 255;
          if (fwrite(rcvpkt, sizeof(char), iResult, f) < iResult) {
            cout << "Write failed" << endl;
            break;
          }
          rcvState = 0;
          state = 1-state;
        }
        fclose(f);
      // while (!filEnd &&(iResult = recvfrom(clientSocket, rcvpkt, 259, 0, (sockaddr *)&serverAddr, &len)) > 0) {
      //   if (iResult > 0) {
      //       rcvState = 1;
      //       cout << rcvpkt << endl;
      //     }
      //   while (rcvState);
      //   // cout << rcvpkt << endl;
      //   if(filEnd) break;
      // }
    }
  }
  closesocket(clientSocket);
  WSACleanup();
  return 0;
}
