#include <stdio.h>

int main(void) {
    char buffer[4096];
    int r = 0;

    do {
        r = fread(buffer,4096,1,stdin);
    } while (r != 0);

    return 0;
}
