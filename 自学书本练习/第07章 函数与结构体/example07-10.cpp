#include<iostream>
using namespace std;    
struct student {
    string name;
    int chinese,math,english;
};
student a[1024];
bool compare(const student& x, const student& y) {
    int totalX = x.chinese + x.math + x.english;
    int totalY = y.chinese + y.math + y.english;

    return abs(totalX - totalY) <= 10
        && abs(x.chinese - y.chinese) <= 5
        && abs(x.math - y.math) <= 5
        && abs(x.english - y.english) <= 5;
}
int main(){
    int n;
    cin>>n;
    for(int i=0;i<n;i++){
        cin>>a[i].name>>a[i].chinese>>a[i].math>>a[i].english;
    }
    for(int i=0;i<n;i++){
        for(int j=i+1;j<n;j++){
            if(compare(a[i],a[j])==1){
                cout<<a[i].name<<' '<<a[j].name<<endl;
            }
        }
    }
}