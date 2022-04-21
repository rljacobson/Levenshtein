//good example of a standard C++ LD edit-distance
//https://takeuforward.org/data-structure/edit-distance-dp-33/

#include "iostream"
#include <vector>

using namespace std;

int editDistance(string& S1, string& S2){

    int n = S1.size();
    int m = S2.size();

    vector<int> prev(m+1,0);
    vector<int> cur(m+1,0);

    for(int j=0;j<=m;j++){
        prev[j] = j;
    }

    for(int i=1;i<n+1;i++){
        cur[0]=i;

        for(int j=1;j<m+1;j++){

            if(S1[i-1]==S2[j-1])
                cur[j] = 0+prev[j-1];

            else cur[j] = 1+min(prev[j - 1],min(prev[j],cur[j-1]));
        }
        prev = cur;
    }

    return prev[m];

}

int main() {
    std::string search_term {"Chromis virojfp"};
    std::string db_return {"Chromis verator"};
    string s1 = "horse";
    string s2 = "ros";

    cout << "The minimum number of operations required is:  "<<editDistance(db_return,search_term);
}
