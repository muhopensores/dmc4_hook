#pragma once

/* 8554 */
struct __declspec(align(4)) MtStringSTRING {
    int ref;
    unsigned int length;
    char str[1];
};

/* 8555 */
struct MtString {
    MtStringSTRING* value;
};

