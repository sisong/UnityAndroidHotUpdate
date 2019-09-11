//
//  main.cpp
//  FixUnityJar
//
//  Created by sisong on 2019/9/10.
//  Copyright © 2019 sisong. All rights reserved.
//
#include "fix_unity_jar.h"

#include <iostream>

int main(int argc, const char * argv[]) {
    if (argc!=1+2) return -1;
    int result=fix_unity_jar(argv[1],argv[2]);
    if (result==kFixUnityJar_ok)
        std::cout << "fix_unity_jar() ok!\n";
    else
        std::cout << "fix_unity_jar() return error: "<<result<<" !\n";
    return result;
}
