//good example of a standard C++ LD edit-distance
//https://takeuforward.org/data-structure/edit-distance-dp-33/

#include <vector>
#include "iostream"

using namespace std;


int editDistance(string& S1, string& S2){

    int n = S1.size();
    int m = S2.size();

    vector<int> prev(m+1,0);
    vector<int> cur(m+1,0);
    int cost;
    for(int j=0;j<=m;j++){
        prev[j] = j;
    }

    for(int i=1;i<n+1;i++){
        cur[0]=i;

        for(int j=1;j<m+1;j++){
            if(S1[i-1]==S2[j-1]){
                cur[j] = 0+prev[j-1];
                cost = 0;
            }

            else {
                cur[j] = 1 + min(prev[j - 1], min(prev[j], cur[j - 1]));
                cost = 1;
            }
            if( (i > 1) &&
                (j > 1) &&
                (S1[i-1] == S2[j-2]) &&
                (S1[i-2] == S2[j-1])
                    )
            {
                cout <<"cur:"<<cur[j]<<" prev:"<<cur[j-1]<<" cost:"<<cost<<endl;
                cur[j] = std::min(
                        cur[j],
                        prev[j-1]  +cost  // transposition
                );
            }
        }for (auto i: cur)
            cout <<i;
        cout << endl;
        prev = cur;
    }
    for (auto i: cur)
    return prev[m];

}

int main() {

    string s1 = "hromisviridis";
    string s2 = "vromniigyidsi";

    cout << editDistance(s1,s2);
}
