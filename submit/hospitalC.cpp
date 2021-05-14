
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
#include <fstream>
#include <vector>
#include <algorithm>
#include "datatype.hpp"
#include "map.hpp"

using namespace std;

#define PORT 32190
#define SCHDL_UDP_PORT 33190
#define IP_ADDR "127.0.0.1"
#define ID "C"

int main(int argc, char *argv[])
{

    double **adj_matrix = read_map();

    // get cmd arguments, atoi() in <stdlib.h> to turn char* into int
    int hsptl_location = atoi(argv[1]);
    int capacity = atoi(argv[2]);
    int occupancy = atoi(argv[3]);

    //prase 1: send capacity and occupancy to scheduler using UDP
    int hsptl_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    //socket address of hospital a
    struct sockaddr_in hsptl_addr;
    memset(&hsptl_addr, 0, sizeof(hsptl_addr));      //每个字节都用0填充
    hsptl_addr.sin_family = AF_INET;                 //使用IPv4地址
    hsptl_addr.sin_addr.s_addr = inet_addr(IP_ADDR); //具体的IP地址
    hsptl_addr.sin_port = htons(PORT);               //端口

    bind(hsptl_sock, (struct sockaddr *)&hsptl_addr, sizeof(hsptl_addr));
    cout << "Hospital " << ID << " is up and running using UDP on port " << PORT << endl;

    //socket address of scheduler
    struct sockaddr_in schdl_addr;
    socklen_t schdl_addr_size = sizeof(schdl_addr);
    memset(&schdl_addr, 0, sizeof(schdl_addr));      //每个字节都用0填充
    schdl_addr.sin_family = AF_INET;                 //使用IPv4地址
    schdl_addr.sin_addr.s_addr = inet_addr(IP_ADDR); //具体的IP地址
    schdl_addr.sin_port = htons(SCHDL_UDP_PORT);     //端口

    //data to send (codes from google)
    char buffer[50];
    struct hospitalCapa co;
    co.capacity = capacity;
    co.occupancy = occupancy;
    cout << "Hospital " << ID << " has total capacity " << capacity << " and initial occupancy " << occupancy << endl;

    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, &co, sizeof(buffer));
    sendto(hsptl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&schdl_addr, schdl_addr_size);

    //phase 2: get client location from scheduler and print on screen messages
    struct sockaddr_in recv_schdl_addr;
    socklen_t recv_schdl_addr_size = sizeof(recv_schdl_addr);
    memset(&recv_schdl_addr, 0, recv_schdl_addr_size);

    int clnt_location;

    // loop for muti clients
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        recvfrom(hsptl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&recv_schdl_addr, &recv_schdl_addr_size);
        memcpy(&clnt_location, buffer, sizeof(int));
        cout << "Hospital " << ID << " has received input from client at location " << clnt_location << endl;

        bool is_loc_found = false;
        double distance;
        double availablity;
        double score;

        // for location finding
        for (int i = 0; i < node_num; i++)
        {
            if (clnt_location == reindexed[i])
            {
                is_loc_found = true;
            }
        }
        if (!is_loc_found)
        {
            distance = -1;
            cout << "Hospital " << ID << " does not have the location " << clnt_location << " in map" << endl;

            //send location not found message to scheduler
            int loc_found_msg = 0;
            memset(buffer, 0, sizeof(buffer));
            memcpy(buffer, &loc_found_msg, sizeof(buffer));
            sendto(hsptl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&schdl_addr, schdl_addr_size);
            cout << "Hospital " << ID << " has sent \"location not found\" to the Scheduler" << endl;
        }
        else
        {
            //send location found message to scheduler
            int loc_found_msg = 1;
            memset(buffer, 0, sizeof(buffer));
            memcpy(buffer, &loc_found_msg, sizeof(buffer));
            sendto(hsptl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&schdl_addr, schdl_addr_size);
            sleep(1);

            //calculate availability
            availablity = (double)(capacity - occupancy) / (double)capacity;
            cout << "Hospital " << ID << " has capacity = " << capacity << ", occupation= " << occupancy
                 << ", availability = " << availablity << endl;
            if (availablity < 0 || availablity > 1)
            {
                availablity = -1;
            }
            // find the shorest path
            if (clnt_location == hsptl_location)
            {
                distance = -1;
                cout << "Hospital " << ID << " has found the shortest path to client, distance = None" << endl;
            }
            else
            {
                distance = get_shortest_path(adj_matrix, clnt_location, hsptl_location);
                cout << "Hospital " << ID << " has found the shortest path to client, distance = " << distance << endl;
            }
            //calculate final score
            if (distance < 0 || availablity < 0)
            {
                score = -1;
                cout << "Hospital " << ID << " has the score = None" << endl;
            }
            else
            {
                score = 1 / (distance * (1.1 - availablity));
                cout << "Hospital " << ID << " has the score = " << score << endl;
            }
            //send score and distance to the scheduler
            struct hospitalScore score_msg = {score, distance};
            memset(buffer, 0, sizeof(buffer));
            memcpy(buffer, &score_msg, sizeof(buffer));
            sendto(hsptl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&schdl_addr, schdl_addr_size);

            if (score < 0)
            {
                cout << "Hospital " << ID << " has sent score = None";
            }
            else
            {
                cout << "Hospital " << ID << " has sent score = " << score;
            }
            if (distance < 0)
            {
                cout << "and distance = None to the Scheduler" << endl;
            }
            else
            {
                cout << " and distance = " << distance << " to the Scheduler" << endl;
            }

            //For receiving the result from the Scheduler
            memset(buffer, 0, sizeof(buffer));
            recvfrom(hsptl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&recv_schdl_addr, &recv_schdl_addr_size);
            string assigned_hsptl_str = buffer;
            if (assigned_hsptl_str == ID)
            {
                occupancy++;
                availablity = (double)(capacity - occupancy) / (double)capacity;
                cout << "Hospital " << ID << " has been assigned to a client, occupation is updated to " << occupancy << " , availability is updated to " << availablity << endl;
            }
        }
        cout << endl;
    }
    close(hsptl_sock);

}

