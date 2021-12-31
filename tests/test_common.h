#ifndef __WXBOX_TEST_COMMON_H
#define __WXBOX_TEST_COMMON_H

#include <gtest/gtest.h>
#include <iostream>

#define PrintUInt8Vector(vtUInt8)                                                                                          \
    {                                                                                                                      \
        std::cout << "[ ";                                                                                                 \
        std::for_each(vtUInt8.begin(), vtUInt8.end(), [&](auto byte) {                                                     \
            std::cout << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << uint16_t(byte) << " "; \
        });                                                                                                                \
        std::cout << "]" << std::endl;                                                                                     \
    }

#endif  // #ifndef __WXBOX_TEST_COMMON_H