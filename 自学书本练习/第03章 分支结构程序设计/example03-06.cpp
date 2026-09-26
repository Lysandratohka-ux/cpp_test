#include <iostream>
using namespace std;
int main() {
    char c;
    cin >> c;
    switch(c) {
        case 'G':
            cout << "Hello, my master!" << endl;
        case 'N':
            cout << "I am xiaoluo" << endl;
            break;
        case 'S':
            cout<<"Teinei teinei teinei~" << endl;
            break;
        case 'B':case'Q':
            cout<<"Bye bye!"<<endl;
            break;
        default:
            cout << "Sorry..." << endl;
    }
    return 0;
}
