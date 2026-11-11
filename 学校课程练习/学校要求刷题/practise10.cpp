#include<iostream>
using namespace std;
void compareAndSwap(int &right,int &left){
    if(right>left){
        int bowl;
        bowl=right;
        right=left;
        left=bowl;
    }
}
int main(){
    int a, b, c, d, e;
    cin >> a >> b >> c >> d >> e;
    compareAndSwap(a, b);
    compareAndSwap(b, c);
    compareAndSwap(c, d);
    compareAndSwap(d, e);

    compareAndSwap(a, b);
    compareAndSwap(b, c);
    compareAndSwap(c, d);

    compareAndSwap(a, b);
    compareAndSwap(b, c);

    compareAndSwap(a, b);

    cout << a << ' ' << b << ' ' << c << ' ' << d << ' ' << e << '\n';

    return 0;
}