//
//  main.cpp
//  HotCacheLibDemo
//
//  Created by sisong on 2019/8/20.
//  Copyright © 2019 sisong. All rights reserved.
//
#include "../../src/hot_cache_lib.h"

#include <iostream>

int main(int argc, const char * argv[]) {
    if (argc!=1+4) return -1;
    int result=hot_cache_lib_check(argv[1],argv[2],argv[3]);
    std::cout << "hot_cache_lib_check() return: "<<result<<"\n";
    result=hot_cache_lib(argv[1],argv[2],argv[3],argv[4]);
    if (result==kCacheLib_ok)
        std::cout << "hot_cache_lib() ok!\n";
    else
        std::cout << "hot_cache_lib() return error: "<<result<<" !\n";
    return result;
}
