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
void sortOrder(int &a,int &b,int &c){
    compareAndSwap(a, b);
    compareAndSwap(b, c);
    
    compareAndSwap(a, b);
}
int main(){
    int a, b, c;
    cin >> a >> b >> c;
    sortOrder(a,b,c);
    cout << a << ' ' << b << ' ' << c <<'\n';
    return 0;
}