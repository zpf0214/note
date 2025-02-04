#include<iostream>
#include<vector>
#include<concepts>
#include<functional>

using namespace std;

template<typename T>
concept Arithemtic = is_arithmetic_v<T>;

template<Arithemtic T>
using Matrix = vector<vector<T>>;


template<Arithemtic T>
class Solution {
    public:
        Matrix<T> permute(vector<T> nums) {
            const int n = nums.size();
            Matrix<T> ans;
            function<void(const int)> dfs;
            dfs = [&](const int idx) {
                if(idx >= n){
                    if(idx == n) {
                        ans.emplace_back(nums);
                    }
                    return;
                }

                for(int i=idx; i<n; i++) {
                    swap(nums[idx], nums[i]);
                    dfs(idx+1);
                    swap(nums[idx], nums[i]);
                }
            };

            dfs(0);
            return ans;
        }
};

int main() {
    Solution<int> solu;
    auto ans = solu.permute({1,2,3});
    for(const auto& nums: ans){
        for(const auto& elem: nums){
            cout << elem << " ";
        }
        cout << '\n';
    }
    return 0;
}
