#include "utils.h"
#include "os.h"

_i
main(void){
#ifdef _RELEASE
    os.daemonize("/");
#endif

    return 0;
}
