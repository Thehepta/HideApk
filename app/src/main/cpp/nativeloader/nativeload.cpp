//
// Created by chic on 2023/11/30.
//


#include "nativeload.h"

int android_namespace_user_t::user_print() {
    return a;
}
android_namespace_user_t *bbs = new android_namespace_user_t() ;

int text(){
    return bbs->a;
}