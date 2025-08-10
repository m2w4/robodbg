#include <stdio.h>
#include <stdint.h>

int main( ) {
    unsigned int input = 0xafafafaf;
    if(input == 0x42424242)
    {
        return 0;
    }
    return -1;
}
