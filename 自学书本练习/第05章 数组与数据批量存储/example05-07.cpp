#include<iostream>
using namespace std;
int main(){
    int a[22][22][22],w,x,h;
    cin>>w>>x>>h;
    for (int i=1;i<=w;i++){
        for(int j=1;j<=x;j++){
            for(int k=1;k<=h;k++){
                a[i][j][k]=1;
            }
        }
    }
    int q;
    cin>>q;
    for(int i=0;i<q;i++){
        int x1,y1,z1,x2,y2,z2;
        cin>>x1>>y1>>z1>>x2>>y2>>z2;
        for(int j=x1;j<=x2;j++){
            for(int k=y1;k<=y2;k++){
                for(int l=z1;l<=z2;l++){
                    a[j][k][l]=0;
                }
            }
        }
    }
    int ans=0;
    for(int i=1;i<=w;i++){
        for(int j=1;j<=x;j++){
            for(int k=1;k<=h;k++){
                ans+=a[i][j][k];
            }
        }
    }
    cout<<ans<<endl;
    return 0
}