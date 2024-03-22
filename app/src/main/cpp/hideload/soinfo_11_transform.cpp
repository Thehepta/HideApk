//
// Created by chic on 2024/3/22.
//
#include "soinfo_11_transform.h"


void android_11_soinfo_transorm(soinfo* si, soinfo_11_transform *si_11){

    si->phdr = si_11->phdr;
    si->phnum = si_11->phnum;
    si->base = si_11->base;
    si->size = si_11->size;
    si->dynamic = si_11->dynamic;
    si->flags_ = si_11->flags_;
    si->strtab_ = si_11->strtab_;
    si->symtab_ = si_11->symtab_;
    si->nbucket_ = si_11->nbucket_;
    si->nchain_ = si_11->nchain_;
    si->bucket_ = si_11->bucket_;
    si->chain_ = si_11->chain_;
    si->plt_rela_ = si_11->plt_rela_;
    si->plt_rela_count_ = si_11->plt_rela_count_;
    si->rela_ = si_11->rela_;
    si->rela_count_ = si_11->rela_count_;
    si->preinit_array_ = si_11->preinit_array_;
    si->preinit_array_count_ = si_11->preinit_array_count_;
    si->init_array_ = si_11->init_array_;
    si->init_array_count_ = si_11->init_array_count_;
    si->fini_array_ = si_11->fini_array_;
    si->fini_array_count_ = si_11->fini_array_count_;
    si->init_func_ = si_11->init_func_;
    si->fini_func_ = si_11->fini_func_;
    si->constructors_called = si_11->constructors_called;
    si->load_bias = si_11->load_bias;
    si->version_ = si_11->version_;
    si->rtld_flags_ = si_11->rtld_flags_;
    si->dt_flags_1_ = si_11->dt_flags_1_;
    si->strtab_size_ = si_11->strtab_size_;
    si->gnu_nbucket_ = si_11->gnu_nbucket_;
    si->gnu_bucket_ = si_11->gnu_bucket_;
    si->gnu_chain_ = si_11->gnu_chain_;
    si->gnu_maskwords_ = si_11->gnu_maskwords_;
    si->gnu_shift2_ = si_11->gnu_shift2_;
    si->gnu_bloom_filter_ = si_11->gnu_bloom_filter_;
    si->android_relocs_ = si_11->android_relocs_;
    si->android_relocs_size_ = si_11->android_relocs_size_;
    si->soname_ = si_11->soname_;
    si->versym_ = si_11->versym_;
    si->verdef_ptr_ = si_11->verdef_ptr_;
    si->verdef_cnt_ = si_11->verdef_cnt_;
    si->verneed_ptr_ = si_11->verneed_ptr_;
    si->verneed_cnt_ = si_11->verneed_cnt_;


}