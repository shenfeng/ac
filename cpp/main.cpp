#include "acindex.h"
#include "iostream"

using namespace std;

int main() {
//    cout << "---------------------------------" << endl;

    Pinyins pinyins;
//
//    cout << "---------------------------------" << endl;

    pinyins.Open("/home/feng/gcode/src/github.com/shenfeng/ac/scripts/pinyins");
//    cout << "---------------------------------" << endl;
//    auto p = pinyins.Get("支新闻");

//    cout << "---------------------------------" << p.first << endl;

//    cout << p.first << "\t" << p.second << "---------------------" <<  endl;

//    cout << "---------------------------------" << endl;

//    return 1;

    Indexes indexes;

    cout << "--------------------" << endl;

    indexes.Open("/Users/feng/gocode/src/github.com/shenfeng/ac/data", pinyins);


    AcIndex *idx = new AcIndex(pinyins);
//
////    idx->Open("/Users/feng/gocode/src/github.com/shenfeng/ac/data/company_tiny");
    idx->Open("/Users/feng/gocode/src/github.com/shenfeng/ac/data/company_tiny");
////    idx->Open("/tmp/t");
//
   for (int i = 0; i < 10; i++) {
        AcReq r("", "z", 1, 0);

        AcResult resp;
        indexes.Search(r, resp);
//        idx->Search(r, resp);
        printf("\n");
   }




    return 0;
}
