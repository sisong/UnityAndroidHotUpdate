//
//  main.cpp
//  VirtualApkPatch
//
//  Created by sisong on 2019/09/12.
//  Copyright © 2019 sisong. All rights reserved.
//
#include "../../patch/virtual_apk_patch.h"
#include "../../../ApkDiffPatch/HDiffPatch/_clock_for_demo.h"

#include <iostream>

int main(int argc, const char * argv[]) {
    double time0=clock_s();
    if (argc!=1+9) return -1;
    int threadNum=atoi(argv[9]);
    const char* hotApk=argv[3];
    const char* newSoDir=argv[6];
    int result=virtual_apk_patch(argv[1],argv[2],hotApk,argv[4],
                                 argv[5],newSoDir,argv[7],argv[8],
                                 threadNum);
    if (result==kVApkPatch_ok)
        printf("virtual_apk_patch() ok!\n");
    else
        printf("virtual_apk_patch() return error: %d !\n",result);
    double time1=clock_s();
    printf("virtual_apk_patch time: %.3f s\n\n",(time1-time0));
    
    if ((result==kVApkPatch_ok)&&(newSoDir!=0)&&(strlen(newSoDir)>0)
            &&(hotApk!=0)&&(strlen(hotApk)>0)){
        result=virtual_apk_merge(argv[1],argv[2],hotApk,argv[4],
                                 argv[5],newSoDir,argv[8]);
        printf("\n");
        if (result==kVApkMerge_ok)
            printf("virtual_apk_merge() ok!\n");
        else
            printf("virtual_apk_merge() return error: %d !\n",result);
        double time2=clock_s();
        printf("virtual_apk_merge time: %.3f s\n",(time2-time1));
    }
    return result;
}
