#include<iostream>
using namespace std;
int main(){
    int a[68][68]={},n,m;
    cin>>n>>m;
    for(int i=0;i<m;i++){
        int x1,y1,x2,y2;
        cin>>x1>>y1>>x2>>y2;
        for(int j=x1;j<=x2;j++){
            for(int k=y1;k<=y2;k++){
                a[j][k]++;
            }
        }
    }
    for(int i=0;i<=n;i++){
        for(int j=0;j<=n;j++){
            cout<<"(i,j) "<<a[i][j]<<" "<<endl;
        }
        cout<<'\n';
    }
    return 0;
}