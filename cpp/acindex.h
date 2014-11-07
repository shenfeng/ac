#include "acitem.h"

struct AcIndexItem {
    int index;
    int data;
    int show;
    float score;
};

class AcIndex {
    std::string name;
    Buffer data;
    Buffer index;

    std::vector<AcIndexItem> indexes;

    int Open(std::string path) {
        
    }
};