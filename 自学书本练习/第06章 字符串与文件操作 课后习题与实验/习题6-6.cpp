#include<iostream>
#include<string>
using namespace std;
int main(){
    string starName,groupName;
    cin>>starName>>groupName;
    int starNum=1,groupNum=1;
    for(size_t i=0;i<starName.length();i++){
        starNum=(starName[i]-'A'+1)*starNum;
    }
    for(size_t i=0;i<groupName.length();i++){
        groupNum=(groupName[i]-'A'+1)*groupNum;
    }
    if(starNum%47==groupNum%47){
        cout<<"GO";
    }else{
        cout<<"STAY";
    }
    return 0;
}