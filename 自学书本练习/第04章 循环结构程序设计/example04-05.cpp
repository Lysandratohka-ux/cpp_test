#include<iostream>
#include<cstdlib>
#include<ctime>
using namespace std;
int main() {
    int guess,answer;
    srand(time(0));
    answer = rand() % 100 + 1; // 生成1到100之间的随机数
    do {
        cin>>guess;
        if(guess > answer)
            cout<<"Too high!"<<endl;
        if(guess < answer)
            cout<<"Too low!"<<endl;
    } while(guess != answer);
    cout<<"Congratulations! You guessed it right."<<endl;
    return 0;
}



