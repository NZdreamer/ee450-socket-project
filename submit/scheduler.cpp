
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
#include <cmath>
#include <algorithm>
#include <vector>
#include "datatype.hpp"

using namespace std;

#define PORT_UDP 33190 // the port users will be connecting to
#define PORT_TCP 34190
#define PORT_HA 30190
#define PORT_HB 31190
#define PORT_HC 32190
#define IP_ADDR "127.0.0.1"

#define BACKLOG 10 // how many pending connections queue will hold

string from_which_hsptl(struct sockaddr_in hsptl_addr);
string decide_assigned_hospital(vector<string> three_hospital, vector<double> three_score, vector<double> three_distance);

int main()
{

    //server socket for udp connection to hospitals
    int udp_schdl_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    //server socket for tcp connection to client
    int tcp_serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //hospital A addresss
    struct sockaddr_in hsptl_a_addr;
    socklen_t hsptl_a_addr_size = sizeof(hsptl_a_addr);
    memset(&hsptl_a_addr, 0, sizeof(hsptl_a_addr));
    hsptl_a_addr.sin_family = AF_INET;
    hsptl_a_addr.sin_addr.s_addr = inet_addr(IP_ADDR);
    hsptl_a_addr.sin_port = htons(PORT_HA);
    //hospital B addresss
    struct sockaddr_in hsptl_b_addr;
    socklen_t hsptl_b_addr_size = sizeof(hsptl_b_addr);
    memset(&hsptl_b_addr, 0, sizeof(hsptl_b_addr));
    hsptl_b_addr.sin_family = AF_INET;
    hsptl_b_addr.sin_addr.s_addr = inet_addr(IP_ADDR);
    hsptl_b_addr.sin_port = htons(PORT_HB);
    //hospital C addresss
    struct sockaddr_in hsptl_c_addr;
    socklen_t hsptl_c_addr_size = sizeof(hsptl_c_addr);
    memset(&hsptl_c_addr, 0, sizeof(hsptl_c_addr));
    hsptl_c_addr.sin_family = AF_INET;
    hsptl_c_addr.sin_addr.s_addr = inet_addr(IP_ADDR);
    hsptl_c_addr.sin_port = htons(PORT_HC);
    //scheduler(server) socket address
    struct sockaddr_in schdl_addr;
    memset(&schdl_addr, 0, sizeof(schdl_addr));      //每个字节都用0填充
    schdl_addr.sin_family = AF_INET;                 //使用IPv4地址
    schdl_addr.sin_addr.s_addr = inet_addr(IP_ADDR); //具体的IP地址
    schdl_addr.sin_port = htons(PORT_UDP);           //端口
    //scheduler(server) socket address
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));       //每个字节都用0填充
    serv_addr.sin_family = AF_INET;                 //使用IPv4地址
    serv_addr.sin_addr.s_addr = inet_addr(IP_ADDR); //具体的IP地址
    serv_addr.sin_port = htons(PORT_TCP);           //端口

    //hospital capacity and occupancy of A, B, C
    struct hospitalCapa co_a;
    struct hospitalCapa co_b;
    struct hospitalCapa co_c;
    //client location
    int clnt_location;

    //bind udp
    bind(udp_schdl_sock, (struct sockaddr *)&schdl_addr, sizeof(schdl_addr));
    cout << "The Scheduler is up and running." << endl;

    //prase 1 for hospitals: get hospital capacity and occupancy from three hospital using UDP
    //client hosipital address
    struct sockaddr_in hsptl_addr;
    socklen_t hsptl_addr_size = sizeof(hsptl_addr);

    //receive struct hospitalCapa from hospital
    char buffer[50];
    struct hospitalCapa co1;

    //改参数：i < 3
    for (int i = 0; i < 3; i++)
    {
        memset(buffer, 0, sizeof(buffer));
        recvfrom(udp_schdl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&hsptl_addr, &hsptl_addr_size);
        memcpy(&co1, buffer, sizeof(hospitalCapa));

        //client hosipital a
        if (hsptl_addr.sin_port == htons(PORT_HA))
        {
            co_a = {co1.capacity, co1.occupancy};
            cout << "The Scheduler has received information from Hospital A: total capacity is " << co1.capacity << " and initial occupancy is " << co1.occupancy << endl;
        }
        //client hosipital b
        if (hsptl_addr.sin_port == htons(PORT_HB))
        {
            co_b = {co1.capacity, co1.occupancy};
            cout << "The Scheduler has received information from Hospital B: total capacity is " << co1.capacity << " and initial occupancy is " << co1.occupancy << endl;
        }
        //client hosipital c
        if (hsptl_addr.sin_port == htons(PORT_HC))
        {
            co_c = {co1.capacity, co1.occupancy};
            cout << "The Scheduler has received information from Hospital C: total capacity is " << co1.capacity << " and initial occupancy is " << co1.occupancy << endl;
        }
    }

    //prase 1 for client: get client location from client using TCP

    //bind tcp
    bind(tcp_serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    //listen
    listen(tcp_serv_sock, 10);
    //client socket address
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);

    //loop for muti clients
    while (1)
    {
        //receive data(client location) from client
        int tcp_clnt_sock = accept(tcp_serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        memset(buffer, 0, sizeof(buffer));
        read(tcp_clnt_sock, buffer, sizeof(buffer));
        memcpy(&clnt_location, buffer, sizeof(int));
        cout << "The Scheduler has received client at location " << clnt_location << " from the client using TCP over port " << PORT_TCP << endl;

        //prase 2: send clnt_location to three hospitals using UDP

        //send clnt_location to hospitalA
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, &clnt_location, sizeof(buffer));

        //hospital is fully occupied or not
        int send_num = 0;
        vector<bool> has_seat(3, false);
        if (co_a.occupancy < co_a.capacity)
        {
            sendto(udp_schdl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&hsptl_a_addr, hsptl_a_addr_size);
            send_num++;
            has_seat[0] = true;
            cout << "The Scheduler has sent client location to Hospital A using UDP over port " << PORT_UDP << endl;
        }
        if (co_b.occupancy < co_b.capacity)
        {
            sendto(udp_schdl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&hsptl_b_addr, hsptl_b_addr_size);
            send_num++;
            has_seat[1] = true;
            cout << "The Scheduler has sent client location to Hospital B using UDP over port " << PORT_UDP << endl;
        }
        if (co_c.occupancy < co_c.capacity)
        {
            sendto(udp_schdl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&hsptl_c_addr, hsptl_c_addr_size);
            send_num++;
            has_seat[2] = true;
            cout << "The Scheduler has sent client location to Hospital C using UDP over port " << PORT_UDP << endl;
        }

        //receive from hospital : location found or not
        memset(buffer, 0, sizeof(buffer));
        int loc_found;
        for(int i = 0; i < send_num; i++)
        {
            recvfrom(udp_schdl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&hsptl_addr, &hsptl_addr_size);
        }
        memcpy(&loc_found, buffer, sizeof(loc_found));

        //send error message to clint if location not found
        if (loc_found == 0)
        {
            write(tcp_clnt_sock, buffer, sizeof(buffer));
        }
        else
        {
            //send no error message to client
            write(tcp_clnt_sock, buffer, sizeof(buffer));
            //receive score and distance from three hospitals
            vector<double> three_score(3);
            vector<double> three_distance(3);
            vector<string> three_hospital;
            three_hospital.push_back("A");
            three_hospital.push_back("B");
            three_hospital.push_back("C");
            three_hospital.push_back("None");
            for (int i = 0; i < send_num; i++)
            {
                memset(buffer, 0, sizeof(buffer));
                struct hospitalScore score_msg;
                if (recvfrom(udp_schdl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&hsptl_addr, &hsptl_addr_size) == -1) {
                    perror("receive hospital score msg");
                }
                memcpy(&score_msg, buffer, sizeof(hospitalScore));
                double score = score_msg.score;
                double distance = score_msg.distance;

                if (hsptl_addr.sin_port == htons(PORT_HA))
                {
                    cout << "The Scheduler has received map information from Hospital A";
                    three_score[0] = score;
                    three_distance[0] = distance;
                }
                if (hsptl_addr.sin_port == htons(PORT_HB))
                {
                    cout << "The Scheduler has received map information from Hospital B";
                    three_score[1] = score;
                    three_distance[1] = distance;
                }
                if (hsptl_addr.sin_port == htons(PORT_HC))
                {
                    cout << "The Scheduler has received map information from Hospital C";
                    three_score[2] = score;
                    three_distance[2] = distance;
                }

                if (score < 0)
                {
                    cout << ", the score = None";
                }
                else
                {
                    cout << ", the score = " << score;
                }
                if (distance < 0)
                {
                    cout << " and the distance = None" << endl;
                }
                else
                {
                    cout << " and the distance = " << distance << endl;
                }
            }
            //decide the assignment
            string assigned_hsptl_str = decide_assigned_hospital(three_hospital, three_score, three_distance);
            cout << "The Scheduler has assigned Hospital " << assigned_hsptl_str << " to the client" << endl;

            //send result to client
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, assigned_hsptl_str.c_str());
            if (write(tcp_clnt_sock, buffer, sizeof(buffer)) != -1)
            {
                cout << "The Scheduler has sent the result to client using TCP over port " << PORT_TCP << endl;
            }

            //send result to assigned hospital
            // memset(buffer, 0, sizeof(buffer));
            // memcpy(buffer, &assigned_hsptl_charstar, sizeof(buffer));
            if (assigned_hsptl_str == "A")
            {
                co_a.occupancy++;
                if (has_seat[0])
                {
                    sendto(udp_schdl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&hsptl_a_addr, hsptl_a_addr_size);
                }
                if (has_seat[1])
                {
                    sendto(udp_schdl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&hsptl_b_addr, hsptl_b_addr_size);
                } 
                if (has_seat[2])
                {
                    sendto(udp_schdl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&hsptl_c_addr, hsptl_c_addr_size);
                }
                cout << "The Scheduler has sent the result to Hospital A using UDP over port " << PORT_UDP << endl;
            }
            else if (assigned_hsptl_str == "B")
            {
                co_b.occupancy++;
                if (has_seat[0])
                {
                    sendto(udp_schdl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&hsptl_a_addr, hsptl_a_addr_size);
                }
                if (has_seat[1])
                {
                    sendto(udp_schdl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&hsptl_b_addr, hsptl_b_addr_size);
                } 
                if (has_seat[2])
                {
                    sendto(udp_schdl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&hsptl_c_addr, hsptl_c_addr_size);
                }
                cout << "The Scheduler has sent the result to Hospital B using UDP over port " << PORT_UDP << endl;
            }
            else if (assigned_hsptl_str == "C")
            {
                co_c.occupancy++;
                if (has_seat[0])
                {
                    sendto(udp_schdl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&hsptl_a_addr, hsptl_a_addr_size);
                }
                if (has_seat[1])
                {
                    sendto(udp_schdl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&hsptl_b_addr, hsptl_b_addr_size);
                } 
                if (has_seat[2])
                {
                    sendto(udp_schdl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&hsptl_c_addr, hsptl_c_addr_size);
                }
                cout << "The Scheduler has sent the result to Hospital C using UDP over port " << PORT_UDP << endl;
            }
            else 
            {
                if (has_seat[0])
                {
                    sendto(udp_schdl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&hsptl_a_addr, hsptl_a_addr_size);
                }
                if (has_seat[1])
                {
                    sendto(udp_schdl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&hsptl_b_addr, hsptl_b_addr_size);
                } 
                if (has_seat[2])
                {
                    sendto(udp_schdl_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&hsptl_c_addr, hsptl_c_addr_size);
                }
            }
        }
        cout << endl;
        close(tcp_clnt_sock);
    }
    //end
    close(udp_schdl_sock);
    close(tcp_serv_sock);
}

string from_which_hsptl(struct sockaddr_in hsptl_addr)
{
    if (hsptl_addr.sin_port == htons(PORT_HA))
    {
        return "A";
    }
    else if (hsptl_addr.sin_port == htons(PORT_HB))
    {
        return "B";
    }
    else if (hsptl_addr.sin_port == htons(PORT_HC))
    {
        return "C";
    }
}

string decide_assigned_hospital(vector<string> three_hospital, vector<double> three_score, vector<double> three_distance)
{
    string assigned_hsptl = "";
    if (three_distance[0] < 0 || three_distance[1] < 0 || three_distance[2] < 0)
    {
        assigned_hsptl = "None";
    }
    else
    {
        if (three_score[0] < 0 || three_score[1] < 0 || three_score[2] < 0)
        {
            assigned_hsptl = "None";
        }
        vector<double>::iterator max_score = max_element(three_score.begin(), three_score.end());
        int count_max_num = count(three_score.begin(), three_score.end(), *max_score);
        if (count_max_num == 1)
        {
            assigned_hsptl = three_hospital[max_score - three_score.begin()];
        }
        else if (count_max_num == 3)
        {
            vector<double>::iterator min_distance = min_element(three_distance.begin(), three_distance.end());
            assigned_hsptl = three_hospital[min_distance - three_distance.begin()];
        }
        else
        {
            int idx_a, idx_b;
            if (three_score[0] == three_score[1])
            {
                idx_a = 0;
                idx_b = 1;
            }
            else if (three_score[1] == three_score[2])
            {
                idx_a = 1;
                idx_b = 2;
            }
            else
            {
                idx_a = 0;
                idx_b = 2;
            }
            if (three_distance[idx_a] < three_distance[idx_b])
            {
                assigned_hsptl = three_hospital[idx_a];
            }
            else
            {
                assigned_hsptl = three_hospital[idx_b];
            }
        }
    }
    return assigned_hsptl;
}
