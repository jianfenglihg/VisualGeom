#include "TestGeometry.h"


int main(){
    test test_obj;
    test_obj.generateVisualData(8,100);
    test_obj.testTriangulation();
    test_obj.testPnP();
    return 0;
}
