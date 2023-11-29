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

    struct LinkedListHeader {
        LinkedListEntry<T>* head;
        LinkedListEntry<T>* tail;
    };
    LinkedListHeader* header_;
    DISALLOW_COPY_AND_ASSIGN(LinkedList);
};