/* Copyright 2014 DEI - Universita' di Bologna

author    DEI - Universita' di Bologna
          Alessandro Capotondi - alessandro.capotondi@unibo.it
          Giuseppe Tagliavini  - giuseppe.tagliavini@unibo.it
          Andrea Marongiu      - amarongiu@unibo.it
          
info      support functions */

#include "appsupport.h"

void abort() {
    printf("ABORT!\n");
    while(1);
}

void exit(int status) {
    printf("EXIT!\n");
    while(1);
}

