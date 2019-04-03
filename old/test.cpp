#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>

using namespace std;

vector<string> vec_intersect(vector<string> &v1, vector<string> &v2) {
	vector<string> result;
	for (auto elem : v1) {
		if (find(v2.begin(), v2.end(),elem) != v2.end())
			result.push_back(elem);
	}
	return result;
}

template<typename S,typename T>
bool contains(S v, T elem) {
    if (find(v.begin(), v.end(), elem) == v.end())  
        return false;
    else
        return true;
}

int main() {
	vector<int> v1 = {3,2,1,5,4};
	if (contains(v1,4))
		cout << "CONTAINTS" << endl;
	else
		cout << "NOT IN VECTOR" << endl;
	
	for (auto x : v1)
		cout << x << " ";
	cout << endl;

	set<int> v2(v1.begin(), v1.end());
	for (auto x : v2) 
		cout << x << " ";
	cout << endl;

	vector<int> v3(v2.begin(), v2.end());
	for (auto x : v3)
		cout << x << " ";
	cout << endl;
}	