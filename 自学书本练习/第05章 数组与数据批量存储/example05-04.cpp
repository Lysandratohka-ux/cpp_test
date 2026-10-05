#include<iostream>
using namespace std;
int main(){
    int l,m,tree[10010]={0},a,b,s=0;
    cin>>l>>m;
    for(int i=0;i<m;i++){
        cin>>a>>b;
        for(int j=a;j<=b;j++){
            tree[j]=1;
        }
    }
    for(int i=0;i<=l;i++){
        if(tree[i]==0){
            s+=1;
        }
    }
    cout<<s<<endl;
    return 0;
}