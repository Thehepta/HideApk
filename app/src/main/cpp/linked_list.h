//
// Created by chic on 2023/11/28.
//

#pragma once
#include "user_system.h"

/*android12 和android13 这个结构体的大小是不同的，会导致我们自己定义的soinfo结构体的成员变量错位*/
template<typename T>
struct LinkedListEntry {
    LinkedListEntry<T>* next;
    T* element;
};


template<typename T, typename Allocator>
class LinkedList {
    struct LinkedListHeader {
        LinkedListEntry<T>* head;
        LinkedListEntry<T>* tail;
    };
public:
//    template<typename F>
//    T* find_if(F predicate) const {
//        for (LinkedListEntry<T>* e = head_; e != nullptr; e = e->next) {
//            if (predicate(e->element)) {
//                return e->element;
//            }
//        }
//
//        return nullptr;
//    }


    template<typename F>
    T* find_if(F predicate) const {
        for (LinkedListEntry<T>* e = head(); e != nullptr; e = e->next) {
            if (predicate(e->element)) {
                return e->element;
            }
        }

        return nullptr;
    }
private:

    LinkedListEntry<T>* head() const {
              return header_ != nullptr ? header_->head : nullptr;
            }

    LinkedListHeader* header_;
//    LinkedListEntry<T>* tail_;
};



