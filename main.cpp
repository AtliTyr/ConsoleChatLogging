#if defined _WIN32 || _WIN64
#undef UNICODE

#define WIN32_LEAN_AND_MEAN


#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")

WSADATA wsaData;
int iResult;

SOCKET ListenSocket = INVALID_SOCKET;
SOCKET ClientSocket = INVALID_SOCKET;

struct addrinfo* result = NULL;
struct addrinfo hints;

int iSendResult;

#define DEFAULT_PORT "7777"

#elif __linux__
#include <sys/utsname.h>
#include<unistd.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>

#define PORT 7777 // Будем использовать этот номер порта

struct sockaddr_in serveraddress, client;
socklen_t length;
int sockert_file_descriptor, connection, bind_status, connection_status;

#endif

#define MESSAGE_LENGTH 1024
char message[MESSAGE_LENGTH];

#include <iostream>
#include "Chat.h"
#include "Functions.h"
//#include "Logger.h"

using namespace std;

Chat chat1;

bool socketInitAndBind()
{
#if defined _WIN32 || _WIN64
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // No longer need server socket
    closesocket(ListenSocket);
#elif __linux__
    sockert_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (sockert_file_descriptor == -1) {
        cout << "Socket creation failed.!" << endl;
        exit(1);
    }
    // 
    serveraddress.sin_addr.s_addr = htonl(INADDR_ANY);
    // Зададим номер порта для связи
    serveraddress.sin_port = htons(PORT);
    // Используем IPv4
    serveraddress.sin_family = AF_INET;
    // Привяжем сокет
    bind_status = bind(sockert_file_descriptor, (struct sockaddr*)&serveraddress,
        sizeof(serveraddress));
    if (bind_status == -1) {
        cout << "Socket binding failed.!" << endl;
        exit(1);
    }
    // Поставим сервер на прием данных 
    connection_status = listen(sockert_file_descriptor, 5);
    if (connection_status == -1) {
        cout << "Socket is unable to listen for new connections.!" << endl;
        exit(1);
    }
    else {
        cout << "Server is listening for new connection: " << endl;
    }
    length = sizeof(client);
    connection = accept(sockert_file_descriptor, (struct sockaddr*)&client, &length);
    if (connection == -1) {
        cout << "Server is unable to accept the data from client.!" << endl;
        exit(1);
    }

    const int enable = 1;
    setsockopt(sockert_file_descriptor, SOL_SOCKET, SO_REUSEADDR, &(enable), sizeof(int));
#endif // 
}

bool closeSocket()
{
#if defined _WIN32 || _WIN64
    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();
#elif __linux__
    close(sockert_file_descriptor);
    return 0;
#endif
}

void receiveMessage()
{
#if defined _WIN32 || _WIN64
    recv(ClientSocket, message, sizeof(message), 0);
#elif __linux__
    read(connection, message, sizeof(message));
#endif
}

void sendMessage()
{
#if defined _WIN32 || _WIN64
    iSendResult = send(ClientSocket, message, sizeof(message), 0);
#elif __linux__
    ssize_t bytes = write(connection, message, sizeof(message));
    if (bytes >= 0) {
        cout << "Data successfully sent to the client.!" << endl;
    }
#endif
}

int main()
{

    setlocale(LC_ALL, "rus");

    if (chat1.SetDataBaseConnection() == false)
    {   
        cout << "Connection to Database failed!" << endl;
        return 1;
    }

    chat1.createDatabase();
//    if (socketInitAndBind() == true)
//    {
//        return 1;
//    }
//
//    std::string msg;
//
//    while (1) {
//
//        memset(message, 0, MESSAGE_LENGTH);
//
//        receiveMessage();
//
//        bool success = true;
//        string pref;
//        string tmp = message;
//
//        if (tmp == "end") {
//            cout << "Client Exited." << endl;
//            cout << "Server is Exiting..!" << endl;
//            break;
//        }
//
//        prefixDetection(pref, tmp);
//
//#if defined _WIN32 || _WIN64
//        commandProcessing(pref, tmp, success, ClientSocket);
//#elif defined __linux__
//        commandProcessing(pref, tmp, success, connection);
//#endif
//        
//        if (pref != "/rul" && pref != "/rpm" && pref != "/rgm" && pref != "/q" && pref != "/ruol")
//        {
//            /*bzero(message, MESSAGE_LENGTH);*/
//            memset(message, 0, sizeof(message));
//
//            if (success)
//                strcpy(message, "true");
//            else
//            {
//                strcpy(message, "false");
//            }
//            sendMessage();        
//        }
//    }
//
//    if (closeSocket() == 1)
//    {
//        return 1;
//    }

    chat1.CloseDataBaseConnection();
    cout << "Connection to Database closed" << endl;

    return 0;
}