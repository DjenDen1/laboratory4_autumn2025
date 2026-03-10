#include <iostream>
#include "pool_allocator.h"
#include "container1.h"
#include <map>

using ALLOC1 = PoolMapAllocator<std::pair<const int, int>, 10 >;
using ALLOC2 = PoolMapAllocator<int,10>;
int factorial(int number)
{
    int result = 1;
    for(int index = 1; index <= number; index++)
    {
        result*=index;
    }

    return result;
}
int main()
{

    std::map<int , int> map_1;
    for(int index = 0; index < 10; index++)
    {
        map_1[index] = factorial(index);
    }


    std::map<int, int, std::less<int>, ALLOC1> map2;
    for(int index = 0; index < 10; index++)
    {
        map2[index]= factorial(index);

    }

    for(const auto p : map2)
    {
        std::cout << p.first << " " << p.second << std::endl;
    }

    ConsistentContainer<int> cont1;
    for(int index = 0; index < 10; index++)
    {
        cont1.push_back(index);
    }

    ConsistentContainer<int , ALLOC2> cont2;
    for(int index = 0; index < 10; index++)
    {
        cont2.push_back(index);
    }

    for(auto c : cont1)
    {
        std::cout << c << " ";
    }



    
    return 0;
}