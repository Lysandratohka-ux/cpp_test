#include<iostream>
using namespace std;
bool compareLocation (double x1,double y1,double x2,double y2){
    return (x1-x2)*(x1-x2)+(y1-y2)*(y1-y2)<1.0;
}
int main(){
    double x,y;
    cin>>x>>y;
    if(compareLocation(x,y,-2,2)){
        cout<<1;
    }else if(compareLocation(x,y,-2,-2)){
        cout<<2;
    }else if(compareLocation(x,y,2,-2)){
        cout<<3;
    }else if(compareLocation(x,y,2,2)){
        cout<<4;
    }else{
        cout<<-1;
    }
    return 0;
}