
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string.h>

using namespace std;

#define INF 100000

struct Link
{
    int u, v;
    double weight;
};

vector<Link> links;    //content of lines in map.txt
vector<int> reindexed; //the indexes of this vector are re-indexed locations, the contents of this vector are original locations
int node_num;
//double** reindx_map;

double **read_map(); // read in map.txt and turn it into a map
double get_shortest_path(double **adj_matrix, int clnt_location, int hsptl_location);

double **read_map()
{

    //read in map.txt and put all line of data into a vector of struct Link
    int u, v;
    double weight;
    ifstream infile; //input stream
    infile.open("map.txt", ios::in);
    if (!infile.is_open())
    {
        cout << "Open file failure" << endl;
    }
    int i = 0;
    while (!infile.eof())
    { // loop until file ends
        infile >> u >> v >> weight;
        links.push_back({u, v, weight});
        i++;
    }
    infile.close(); //close file

    //re-index the nodes, put them into a vector called reindexed
    for (int i = 0; i < links.size(); i++)
    {
        reindexed.push_back(links[i].u);
        reindexed.push_back(links[i].v);
    }
    sort(reindexed.begin(), reindexed.end());                                     //sort the index number
    reindexed.erase(unique(reindexed.begin(), reindexed.end()), reindexed.end()); //erase all duplicate index numbers

    //2d array of map
    node_num = reindexed.size();

    double **reindx_map = new double *[node_num];
    for (int i = 0; i < node_num; i++)
    {
        reindx_map[i] = new double[node_num];
        for (int j = 0; j < node_num; j++)
        {
            reindx_map[i][j] = INF;
        }
    }

    for (int i = 0; i < links.size(); i++)
    {
        vector<int>::iterator iter_u = find(reindexed.begin(), reindexed.end(), links[i].u);
        int index_u = iter_u - reindexed.begin();
        vector<int>::iterator iter_v = find(reindexed.begin(), reindexed.end(), links[i].v);
        int index_v = iter_v - reindexed.begin();
        reindx_map[index_u][index_v] = links[i].weight;
        reindx_map[index_v][index_u] = links[i].weight;
        //double reslt = reindx_map[index_u][index_v];
    }
    return reindx_map;
}

// Dijkstra algrthom (code from google)
//v: client location, dis: distance from v to all other nodes.
void Dijk(double** A, int v, int max, double *dis, int* path)
{
    bool S[max]; // 判断是否已存入该点到S集合中
    int n = max;
    for (int i = 0; i < n; ++i)
    {
        dis[i] = A[v][i];
        S[i] = false; // 初始化
        path[i] = v;
    }
    dis[v] = 0;
    S[v] = true;
    for (int i = 1; i < n; i++)
    {
        int mindist = INF;
        int u = v; 
        // 找出当前未使用的点j的dist[j]最小值
        for (int j = 0; j < n; ++j)
            if ((!S[j]) && dis[j] < mindist)
            {
                u = j; // u保存当前邻接点中距离最小的点的号码
                mindist = dis[j];
            }
        S[u] = true;
        //更新dist
        for (int j = 0; j < n; j++)
            if ((!S[j]) && A[u][j] < INF)
            {
                if (dis[u] + A[u][j] < dis[j]) //在通过新加入的u点路径找到离v点更短的路径
                {
                    dis[j] = dis[u] + A[u][j]; //更新dist
                    path[j] = u;               //记录前驱顶点
                }
            }
    }
}

double get_shortest_path(double **adj_matrix, int clnt_location, int hsptl_location)
{
    vector<int>::iterator iter = find(reindexed.begin(), reindexed.end(), clnt_location);
    int clnt_loc_idx = iter - reindexed.begin();
    iter = find(reindexed.begin(), reindexed.end(), hsptl_location);
    int hsptl_loc_idx = iter - reindexed.begin();

    int max = node_num;
    double dis[max];
    int path[max];
    Dijk(adj_matrix, clnt_loc_idx, max, dis, path);

    double distance = dis[hsptl_loc_idx];
    return distance;
}

