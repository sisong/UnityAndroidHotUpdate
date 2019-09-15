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
    if (argc!=1+9) return -1;
    int threadNum=atoi(argv[9]);
    const char* hotApk=argv[3];
    int result=virtual_apk_patch(argv[1],argv[2],hotApk,argv[4],
                                 argv[5],argv[6],argv[7],argv[8],
                                 threadNum);
    if (result==kVApkPatch_ok)
        std::cout << "virtual_apk_patch() ok!\n";
    else
        std::cout << "virtual_apk_patch() return error: "<<result<<" !\n";
    
    if ((result==kVApkPatch_ok)&&(hotApk!=0)&&(strlen(hotApk)>0)){
        result=virtual_apk_merge(argv[1],argv[2],hotApk,argv[4],
                                 argv[5],argv[6],argv[8]);
        if (result==kVApkMerge_ok)
            std::cout << "virtual_apk_merge() ok!\n";
        else
            std::cout << "virtual_apk_merge() return error: "<<result<<" !\n";
    }
    
    return result;
}
