#include <iostream>
#include <unordered_map>

void printMap(const std::unordered_map<int, int> *mapPtr);

int pow(int n){
   return n*n % 4; 
}

int main(){
    const int N = 5;
    std::unordered_map<int, int> myMap;
    for (int i = 0; i<N; i++){
        myMap[i] = pow(i);
    }

    printMap(none);

    return 0;
}

void printMap(const std::unordered_map<int, int> *mapPtr){
    if(mapPtr == nullptr){
        std::cout << "null";
        return;
    }
    for(const auto &pair: *mapPtr){
        std::cout << "Key: " << pair.first << 
            ", Value: " << pair.second << "\n";
    }
}
