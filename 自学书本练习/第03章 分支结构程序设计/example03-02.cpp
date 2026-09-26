#include <iostream>
using namespace std;
int main() {
    int a;
    cin >> a;
    bool p1,p2;
    p1=a%2==0;
    p2=4<a&&a<=12;
    cout << (p1 && p2)<<' ';
    cout << (p1 || p2) << ' ';
    cout << (p1!=p2) << ' ';
    cout << (!p1&&!p2) << endl  ;
    

}
    