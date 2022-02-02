// Filesystem data structure

#include "std_datatype.h"

// Untuk filesystem map
struct map_filesystem {
    bool is_filled[512];
};


// Untuk filesystem nodes
struct node_entry {
    byte parent_index;
    byte sector_index;
    char name[14];
};

struct node_filesystem {
    struct node_entry nodes[64];
};


// Untuk filesystem sector
struct sector_entry {
    byte sector_number[16];
};

struct sector_filesystem {
    struct sector_entry sector_list[32];
};
