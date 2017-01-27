
#include <iostream>
#include <thread>

#include "concurrent_hash_map.hpp"


using std::cout;
using std::endl;


template<typename K, typename V>
bool find(const chm::hash_map<K, V>& hmap, const K& key, V& value)
{
    if (hmap.find(key, value))
        cout << "found value " << value << " for key " << key << endl;
    else
        cout << "not found key " << key << endl;
}


void test_chm(chm::hash_map<int, int>& int_map)
{
    cout << "in thread function" << endl;
    int_map.insert(10, 100);
    int_map.insert(20, 200);
    int_map.insert(30, 300);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    int value;
    find<int, int>(int_map, 10, value);
    find<int, int>(int_map, 15, value);

    int_map.insert(15, 150);
    find<int, int>(int_map, 15, value);

    int_map.remove(10);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    find<int, int>(int_map, 10, value);
    int_map.clear();
    find<int, int>(int_map, 30, value);
}


int main()
{
    chm::hash_map<int, int> int_map;
    
    std::thread t1(test_chm, ref(int_map));
    std::thread t2(test_chm, ref(int_map));
    t1.join();
    t2.join();

    return 0;
}
