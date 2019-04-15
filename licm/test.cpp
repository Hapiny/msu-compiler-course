#include <iostream>
#include <set>
#include <unordered_set>
#include <algorithm>

int main(int argc, char const *argv[])
{
    std::set<int> s1 = {1, 2, 3, 4, 5, 6, 7, 8};
    std::set<int> s2 = {7, 8, 9};
    std::set<int> s3 = {5, 3, 2, 2};
    std::cout << s3.size() << std::endl;

    std::cout << std::includes(s1.begin(), s1.end(), s2.begin(), s2.end()) << std::endl;
    std::cout << std::includes(s1.begin(), s1.end(), s3.begin(), s3.end()) << std::endl;
    return 0;
}
