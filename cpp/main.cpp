#include "acindex.h"

using namespace std;

int main() {

    AcIndex *idx = new AcIndex();
//    idx->Open("/Users/feng/gocode/src/github.com/shenfeng/ac/data/company_tiny");
    idx->Open("/home/feng/gcode/src/github.com/shenfeng/ac/data/company_tiny");
//    idx->Open("/tmp/t");

    AcReq r("z", 10, 0);
    AcResult resp;
    idx->Search(r, resp);

    return 0;
}