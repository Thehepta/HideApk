//
// Created by thehepta on 2023/12/17.
//

#pragma one


#include "linker_soinfo.h"

struct SymbolLookupLib {
    uint32_t gnu_maskwords_ = 0;
    uint32_t gnu_shift2_ = 0;
    ElfW(Addr)* gnu_bloom_filter_ = nullptr;

    const char* strtab_;
    size_t strtab_size_;
    const ElfW(Sym)* symtab_;
    const ElfW(Versym)* versym_;

    const uint32_t* gnu_chain_;
    size_t gnu_nbucket_;
    uint32_t* gnu_bucket_;

    soinfo* si_ = nullptr;

    bool needs_sysv_lookup() const { return si_ != nullptr && gnu_bloom_filter_ == nullptr; }
};