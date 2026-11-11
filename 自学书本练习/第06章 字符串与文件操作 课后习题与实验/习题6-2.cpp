#include<iostream>
#include<string>
using namespace std;
int main(){
    int n;
    string s;
    cin>>n>>s;
    int ans=0;
    for(size_t i=0;i+1<n;i++){
        if(s[i]=='V'&&s[i+1]=='K'){
            ans++;
            s[i]='X';
            s[i+1]='X';
        }
    }
    for(size_t i=0;i+1<n;i++){
        if((s[i]=='V'&&s[i+1]=='V')||(s[i]=='K'&&s[i+1]=='K')){
            ans++;
            break;
        }
    }
    cout<<ans;
    return 0;
}