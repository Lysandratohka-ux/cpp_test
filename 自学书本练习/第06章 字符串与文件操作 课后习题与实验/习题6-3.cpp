#include<iostream>
#include<string>
using namespace std;
int main() {
	int ansBoy=0,ansGirl=0;
	string st;
	cin>>st;
	for(int i=0;i+2<st.length();i++){
		if (st[i]=='b'||st[i+1]=='o'||st[i+2]=='y')
			ansBoy++; 
    }
    for(int i=0;i+3<st.length();i++){        
		if (st[i]=='g'||st[i+1]=='i'||st[i+2]=='r'||st[i+3]=='l')
			ansGirl++;
    }
    cout<<ansBoy<<endl;
	cout<<ansGirl<<endl; 
	return 0;
}