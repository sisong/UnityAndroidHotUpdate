//
//  main.cpp
//  VirtualApkPatch
//
//  Created by sisong on 2019/09/12.
//  Copyright © 2019 sisong. All rights reserved.
//
#include "../../patch/virtual_apk_patch.h"

#include <iostream>

int main(int argc, const char * argv[]) {
    if (argc!=1+7) return -1;
    const char c=argv[7]?(*argv[7]):'\0';
    bool  isCacheHotSo=(c!='\0')&&(c!='0')&&(c!='f')&&(c!='F');
    assert((c=='1')||(c=='t')||(c=='T'));
    int result=virtual_apk_patch(argv[1],argv[2],argv[3],argv[4],argv[5],argv[6],isCacheHotSo);
    if (result==kVApkPatch_ok)
        std::cout << "virtual_apk_patch() ok!\n";
    else
        std::cout << "virtual_apk_patch() return error: "<<result<<" !\n";
    return result;
}
