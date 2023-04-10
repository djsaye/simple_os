#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

int main ()
{
    int eth_fd, cnt,i;
    uint8_t buf[1024];
    uint8_t buffer[8];

    
    eth_fd = ece391_open((uint8_t*)"eth");

    while (0 != (cnt = ece391_read (eth_fd, buf, 1024))) {
        if (-1 == cnt) {
	        ece391_fdputs (1, (uint8_t*)"file read failed\n");
	        return 3;
        }
	}

    ece391_fdputs (1, (uint8_t*)"Your MAC Address is ");
    
    for (i=0; i<6; i++)
        if (-1 == ece391_write (1, ece391_itoa((uint32_t) buf[i], buffer ,16), cnt)) {
            return 3;
        }
    
    return 0;

}