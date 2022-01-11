#include <modforinjecttest.hpp>

#ifdef _WIN32

#include <Windows.h>
#include <sstream>

void SayHiInject(char* str)
{
    if (str) {
        std::stringstream ss;
        ss << "Hello " << str;
        MessageBoxA(NULL, ss.str().c_str(), "ModForInjectTest", MB_OK);
    }
    else {
        MessageBoxA(NULL, "Hello Inject...", "ModForInjectTest", MB_OK);
	}
}

#else

#include <iostream>

void SayHiInject(char* str)
{
    if (str) {
        std::cout << "Hello " << str << std::endl;
    }
    else {
        std::cout << "Hello Inject..." << std::endl;
    }    
}

#endif