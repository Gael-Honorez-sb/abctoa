#pragma once
#define H_VAL(str) #str
#define H_TOSTRING(str) H_VAL(str)

#define HOLDER_VENDOR "Nozon"
#define HOLDER_VERSION_NUM 0.1


#define ARCH_VERSION         H_TOSTRING(HOLDER_VERSION_NUM) 

#ifndef MAYA_VERSION
   #define MAYA_VERSION "Any"
#endif
