#include<iostream>
using namespace std;
int main() {
    int k,coin=0,day=0;    //k指总天数，需要输入；coin代表总金币数
    cin>>k;
    for(int i=1;;i++){
        for(int j=1;j<=i;j++){    //j表示第几组中的第几天
            coin+=i;
            day++;
            if(day==k) {
            cout<<coin<<endl;
            return 0;}
        }
    }
}