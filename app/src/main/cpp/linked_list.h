//
// Created by chic on 2023/11/28.
//

#pragma once
#include "user_system.h"

template<typename T>
struct LinkedListEntry {
    LinkedListEntry<T>* next;
    T* element;
};


template<typename T, typename Allocator>
class LinkedList {

public:
    template<typename F>
    T* find_if(F predicate) const {
        for (LinkedListEntry<T>* e = head_; e != nullptr; e = e->next) {
            if (predicate(e->element)) {
                return e->element;
            }
        }

        return nullptr;
    }

private:
    LinkedListEntry<T>* head_;
    LinkedListEntry<T>* tail_;
};



