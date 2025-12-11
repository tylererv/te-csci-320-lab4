#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lab4.h"


Page_Entry* page_table;
TLB_Table tlb;
Main_Memory main_mem;
FILE* backing_store_fp;

// Implement get_page_from() and get_offset_from() functions below this line
unsigned char get_page_from(int logical_address){
   return (logical_address >> 8) & 0x00FF;
}
unsigned char get_offset_from(int logical_address){
   return logical_address & 0x00FF;
}

// page table functions
void init_page_table(){
    page_table = (Page_Entry*) malloc(sizeof(Page_Entry) * 256);
    for(int i = 0; i < (1 << PAGE_ADDR_BITS); i++){
        page_table[i].is_valid = 0;
    }
}

void close_page_table(){
    free(page_table);
}

short page_table_lookup(unsigned char page){
    if(page_table[page].is_valid) {
        return page_table[page].frame;
    }
    return -1;
}

void update_page_table(unsigned char page, unsigned char frame) {
    page_table[page].frame = frame;
    page_table[page].is_valid = 1;
}

//TLB functions
void init_tlb() {
    tlb.entry = (TLB_Entry*) malloc(sizeof(TLB_Entry) * TLB_LEN);
    tlb.length = TLB_LEN;
    tlb.head = 0;
    tlb.tail = 0;
    tlb.is_full = 0;
    for(int i = 0; i < TLB_LEN; i++){
        tlb.entry[i].is_valid = 0;
    }
}

short tlb_lookup(unsigned char page){
    for(int i = 0; i < tlb.length; i++){
        if(tlb.entry[i].is_valid && tlb.entry[i].page == page){
            return tlb.entry[i].frame;
        }
    }
    return -1;
}

void update_tlb(unsigned char page, unsigned char frame){
    tlb.entry[tlb.head].page = page;
    tlb.entry[tlb.head].frame = frame;
    tlb.entry[tlb.head].is_valid = 1;
    tlb.head = (tlb.head + 1) % tlb.length;
    if(tlb.is_full == 0 && tlb.head == tlb.tail){
        tlb.is_full = 1;
    }
    if(tlb.is_full){
        tlb.tail = (tlb.tail + 1) % tlb.length;
    }
}

void close_tlb(){
    free(tlb.entry);
}

// Physical memory functions
void init_physical_memory(){
    main_mem.mem = (char*) malloc(sizeof(char) * 256 * 256);
    main_mem.next_available_frame = 0;
}

void free_physical_memory(){
    free(main_mem.mem);
}

// Returns the signed value stored at the logical_address
signed char get_value(int logical_address) {
    unsigned char frame_number;
    unsigned char offset = get_offset_from(logical_address);
    unsigned char page = get_page_from(logical_address);
    short tlb_result = tlb_lookup(page);
    if(tlb_result != -1) {
        frame_number = (unsigned char) tlb_result;
    } else {
        short page_table_result = page_table_lookup(page);
        if(page_table_result != -1){
            frame_number = (unsigned char) page_table_result;
            update_tlb(page, frame_number);
        } else {
            increment_page_fault_count();
            frame_number = (unsigned char) main_mem.next_available_frame;
            roll_in(page, frame_number);
            update_page_table(page, frame_number);
            update_tlb(page, frame_number);
            main_mem.next_available_frame++;
        }
    }
    int physical_address = (frame_number << 8) | offset;
    return main_mem.mem[physical_address];
}

// Backing store functions
void init_backing_store(char* filename){
    backing_store_fp = fopen(filename, "rb");
    if(backing_store_fp == NULL){
        fprintf(stderr, "Error opening backing store file.\n");
        exit(1);
    }
}

int roll_in(unsigned char page, unsigned char frame){
    if(backing_store_fp == NULL){
        fprintf(stderr, "Backing store file not opened.\n");
        exit(1);
    }

    if(fseek(backing_store_fp, page * 256, SEEK_SET) != 0){
        fprintf(stderr, "Error seeking in backing store file.\n");
        exit(1);
    }

    size_t read_bytes = fread(&main_mem.mem[frame * 256], sizeof(char), 256, backing_store_fp);
    if(read_bytes != 256){
        fprintf(stderr, "Error reading from backing store file.\n");
        exit(1);
    }

    return 0;
}
void close_backing_store(){
    if(backing_store_fp != NULL){
        fclose(backing_store_fp);
    }
}

