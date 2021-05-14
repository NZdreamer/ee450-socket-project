#include <string.h>

using namespace std;

// hospital send their capacity and occupancy to the scheduler
struct hospitalCapa
{
    int capacity;
    int occupancy;
};

struct hospitalScore
{
    double score;
    double distance;
};
