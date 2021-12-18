#include <iostream>
#include <Windows.h>

int main(int argc, char *argv[])
{
    system("pause");

    char *str1 = "hello, char string";
    wchar_t *str2 = L"hello, wchar_t string";

    std::cout << str1 << std::endl;
    std::wcout << str2 << std::endl;

    return 0;
}