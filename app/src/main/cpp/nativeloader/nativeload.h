//
// Created by chic on 2023/12/1.
//

#ifndef HIDEAPK_NATIVELOAD_H
#define HIDEAPK_NATIVELOAD_H



struct android_namespace_user_t{

    android_namespace_user_t(){
        a=100;
    }

    int a;
    int user_print();
private:
    bool is_isolated_;
    bool is_greylist_enabled_;
    bool is_also_used_as_anonymous_;
};



#endif //HIDEAPK_NATIVELOAD_H
