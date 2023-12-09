// fix_unity_jar.cpp
// Created by sisong on 2019-09-10.

#include "fix_unity_jar.h"
#include <assert.h>
#include <string>
#include <vector>
#include <algorithm>
#include "../../../ApkDiffPatch/src/patch/Zipper.h"
#include "../../../ApkDiffPatch/HDiffPatch/file_for_patch.h"

    static const char* kUnityPlayer_class="com/unity3d/player/UnityPlayer.class";
    
    static inline std::string zipFile_name(UnZipper* self,int fileIndex){
        int nameLen=UnZipper_file_nameLen(self,fileIndex);
        const char* nameBegin=UnZipper_file_nameBegin(self,fileIndex);
        return std::string(nameBegin,nameBegin+nameLen);
    }
    
    
    #define  check(value,err_code) { \
        if (!(value)){ printf(#value" ERROR!\n");  \
            result=err_code; if (!_isInClear){ goto clear; } } }
    
    static int fixClass(Zipper* zipper,UnZipper* srcZip,int srcFileIndex,uint32_t* out_newCrc32){
        int result=kFixUnityJar_ok;
        bool _isInClear=false;
        
        size_t _codeSize=(size_t)UnZipper_file_uncompressedSize(srcZip,srcFileIndex);
        std::vector<TByte> code(_codeSize,0);
        TByte* code_end=code.data()+code.size();
        {//decompress
            hpatch_TStreamOutput _codeStream;
            mem_as_hStreamOutput(&_codeStream,code.data(),code_end);
            check(UnZipper_fileData_decompressTo(srcZip,srcFileIndex,&_codeStream),
                  kFixUnityJar_decompressError);
        }
        {//edit
            //find one only
            const TByte kLibMain[]={1,0,4,'m','a','i','n'};
            const TByte kLibNull[]={1,0,4,'n','u','l','l'};
            const TByte* _temp_pos=0;
            TByte* pos= std::search(code.data(),code_end,kLibMain,kLibMain+sizeof(kLibMain));
            check(pos!=code_end, kFixUnityJar_fixClassError);
            _temp_pos=std::search(pos+1,code_end,kLibMain,kLibMain+sizeof(kLibMain));
            check(_temp_pos==code_end, kFixUnityJar_fixClassError);
            //replace
            assert(sizeof(kLibMain)==sizeof(kLibNull));
            memcpy(pos,kLibNull,sizeof(kLibNull));
        }
        {//new crc32
            *out_newCrc32=(uint32_t)crc32(crc32(0,0,0),code.data(),(uInt)code.size());
            check(Zipper_file_append_set_new_crc32(zipper,*out_newCrc32),
                  kFixUnityJar_outputJarFileError)
        }
        {//save
            check(Zipper_file_append_begin(zipper,srcZip,srcFileIndex,false,code.size(),0),
                  kFixUnityJar_outputJarFileError);
            check(Zipper_file_append_part(zipper,code.data(),code.size()),
                  kFixUnityJar_outputJarFileError);
            check(Zipper_file_append_end(zipper),
                  kFixUnityJar_outputJarFileError);
        }
    clear:
        _isInClear=true;
        return result;
    }
    
    
    const int  kZipAlignSize=1;
    const int  kCompressLevel=6;
    const int  kDefaultMemLevel=MAX_MEM_LEVEL;
    const bool isAlwaysReCompress=false;
    
    int fix_unity_jar(const char* unity_classes_jar,const char* out_new_jar){
        int  result=kFixUnityJar_ok;
        bool _isInClear=false;
        int  fileCount=0;
        int  foundClassIndex=-1;
        uint32_t newCrc32=0;
        UnZipper unzipper;
        Zipper   zipper;
        UnZipper_init(&unzipper);
        Zipper_init(&zipper);
        
        check(UnZipper_openFile(&unzipper,unity_classes_jar), kFixUnityJar_inputJarFileError);
        fileCount=UnZipper_fileCount(&unzipper);
        
        check(Zipper_openFile(&zipper,out_new_jar,fileCount,kZipAlignSize,false,
                              kCompressLevel,kDefaultMemLevel), kFixUnityJar_outputJarFileError);
        
        for (int i=0; i<fileCount; ++i) {
            std::string fileName=zipFile_name(&unzipper,i);
            // printf("\"%s\"\n",fileName.c_str());
            if (fileName==kUnityPlayer_class){
                check(foundClassIndex<0,kFixUnityJar_classFileError);
                foundClassIndex=i;
                int fixResult=fixClass(&zipper,&unzipper,i,&newCrc32);
                check(fixResult==kFixUnityJar_ok, fixResult);
                printf("  NOTE: fixed \"%s\"\n",fileName.c_str());
            }else{ // copy
                check(Zipper_file_append_copy(&zipper,&unzipper,i,isAlwaysReCompress),
                      kFixUnityJar_inputJarFileError);
            }
        }
        printf("\n");
        check(foundClassIndex>=0, kFixUnityJar_classFileError);
        
        //not need check(Zipper_copyExtra_before_fileHeader(&zipper,&unzipper),kFixUnityJar_inputJarFileError);
        for (int i=0; i<fileCount; ++i) {
            if (i==foundClassIndex){
                check(Zipper_fileHeader_append_set_new_crc32(&zipper,newCrc32),
                      kFixUnityJar_outputJarFileError);
            }
            check(Zipper_fileHeader_append(&zipper,&unzipper,i),kFixUnityJar_inputJarFileError);
        }
        check(Zipper_endCentralDirectory_append(&zipper,&unzipper), kFixUnityJar_inputJarFileError);

        printf("src fileCount:%d\n",fileCount);
        
    clear:
        _isInClear=true;
        check(UnZipper_close(&unzipper), kFixUnityJar_inputJarFileError);
        check(Zipper_close(&zipper), kFixUnityJar_outputJarFileError);
        return result;
    }


