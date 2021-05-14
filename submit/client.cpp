#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <iostream>
#include "datatype.hpp"

using namespace std;

#define SCHDL_TCP_PORT 34190

int main(int argc, char *argv[])
{

    int location = atoi(argv[1]);
    cout << "The client is up and running" << endl;

    //client socket(TCP)
    int clnt_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //client address, dynamic port
    struct sockaddr_in clnt_addr;
    memset(&clnt_addr, 0, sizeof(clnt_addr));           //每个字节都用0填充
    clnt_addr.sin_family = AF_INET;                     //使用IPv4地址
    clnt_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //具体的IP地址
    //server address, assigned port
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));           //每个字节都用0填充
    serv_addr.sin_family = AF_INET;                     //使用IPv4地址
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //具体的IP地址
    serv_addr.sin_port = htons(SCHDL_TCP_PORT);         //端口

    connect(clnt_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    //send client location to scheduler
    char buffer[50];
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, &location, sizeof(buffer));
    write(clnt_sock, buffer, sizeof(buffer));
    cout << "The client has sent query to Scheduler using TCP: client location " << location << endl;

    //location not found error
    int loc_found;
    memset(buffer, 0, sizeof(buffer));
    read(clnt_sock, buffer, sizeof(buffer));
    memcpy(&loc_found, buffer, sizeof(loc_found));
    if(loc_found == 0)
    {
        cout << "Location " << location << " not found" << endl;
    }
    else
    {
        //receive result from scheduler
        //const char* assigned_hsptl_charstar;
        memset(buffer, 0, sizeof(buffer));
        read(clnt_sock, buffer, sizeof(buffer));
        //memcpy(&assigned_hsptl_charstar, buffer, sizeof(int));
        string assigned_hsptl_str = buffer;
        cout << "The client has received results from the Scheduler: assigned to Hospital " << assigned_hsptl_str << endl;

        //not assignment error
        if (assigned_hsptl_str == "None")
        {
            cout << "Score = None, No assignment" << endl;
        }
    }

    close(clnt_sock);

    return 0;
}
