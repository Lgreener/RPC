#include "rocket/common/log.h"
#include "rocket/common/config.h"

#include <pthread.h>

void* fun(void*){

    int i = 20;

    while(i--){
        DEBUGLOG("debug this is thread in %s","fun");
        INFOLOG("info this is thread in %s","fun");
    }
    return NULL;

}

int main(){

    rocket::Config::SetGlobalConfig("../conf/rocket.xml");

    rocket::Logger::InitGlobalLogger();

    pthread_t thread;
    pthread_create(&thread, NULL, &fun, NULL);


    int i = 20;

    while(i--){
    DEBUGLOG("test debug log %s","1l");
    INFOLOG("test info log %s","1l");
    }

    pthread_join(thread, NULL);
    return 0;
    
}