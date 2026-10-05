#include<iostream>
#include<string>
using namespace std;
int main(){
    int n,a,b,opt;
    string s,str;
    cin>>n>>s;
    while(n--){
        cin>>opt;
        switch(opt){
            case 1:
                cin>>str;
                s+=str;
                cout << s << '\n';
                break;
            case 2:
                cin>>a>>b;
                s=s.substr(a,b);
                cout << s << '\n';
                break;
            case 3:
                cin>>a>>str;
                s.insert(a,str);
                cout << s << '\n';
                break;
            case 4:
                cin>>str;
                a=s.find(str);
                cout<<a<<'\n';
                break;
        }
    }
    return 0;
}