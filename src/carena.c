#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdalign.h>
#include <string.h>
#include "carena.h"

#define _CARENA_DEBUG
#define _CARENA_STRICT_SIZE

#ifdef _CARENA_DEBUG
    #include <stdio.h>
#endif

#ifndef CARENA_WIDTH
  #define CARENA_WIDTH 16
#endif

#if CARENA_WIDTH == 8
  typedef uint8_t mdt_t;
#elif CARENA_WIDTH == 16
  typedef uint16_t mdt_t;
#elif CARENA_WIDTH == 32
  typedef uint32_t mdt_t;
#else
  #error "Unsupported CARENA_WIDTH"
#endif

typedef struct{
    mdt_t bytes_next_block;
    mdt_t payload_size;
} hdr_t;

#define SZMETA_T sizeof(mdt_t)
#define SZHEAD_T sizeof(hdr_t)
#define MDTMAX ((mdt_t)~(mdt_t)0)

struct carena{
    void* buff;
    size_t top;
    size_t bot;
    size_t size;
    size_t free;        //Free bytes
    size_t block_count; //Block Count
};


static inline void write_header(void* hdr_ptr, size_t bnb, size_t ps){
    ((hdr_t*)hdr_ptr)->bytes_next_block = bnb;
    ((hdr_t*)hdr_ptr)->payload_size = ps;
}

static inline void read_header(void* hdr_ptr, void* dest){
    memcpy(dest, hdr_ptr, sizeof(hdr_t));
}

static inline void patch_header(void* hdr_ptr, size_t incr){
    ((hdr_t*)(hdr_ptr))->bytes_next_block+=incr;
}

static inline void write_footer(void* hdr_ptr, size_t bpb){
    *(mdt_t*)hdr_ptr = bpb;
}

carena_status carena_init(carena* ca, size_t n){

    if( ca == NULL )
        return CARENA_ERR_INVALID;

    #ifdef _CARENA_STRICT_SIZE
    if( n > MDTMAX )
        return CARENA_ERR_TOO_LARGE;
    #endif

    ca->buff = calloc(1, n);

    if(ca->buff == NULL)
        return CARENA_ERR_INVALID;

    ca->top = 0;
    ca->bot = 0;
    ca->block_count = 0;
    ca->size = n;
    ca->free = n;
    return CARENA_OK;
}

carena_status carena_free (carena* ca){
    if( ca == NULL)
        return CARENA_ERR_INVALID;

    free(ca->buff);
    memset(ca, 0, sizeof(carena));
    return CARENA_OK;

}

static inline size_t align_up(size_t n, size_t a) {
    return (n + a - 1) & ~(a - 1);
}

static inline void update_free(carena* ca){
    size_t tmp = ca->bot - ca->top;
    if( ca->top >= ca->bot)
       tmp += ca->size;
    
    ca->free = tmp;
}

static inline void carena_evict_oldest(carena* ca){

    size_t bnb = ((hdr_t*)(ca->buff + ca->bot))->bytes_next_block;
    
    ca->bot = (ca->bot + bnb) % ca->size;
    if(ca->bot == ca->top){
        ca->bot = 0;
        ca->top = 0;
        #ifdef _CARENA_DEBUG
        if( ca->block_count == 0){
            printf("Structure corrupted\n");
            exit(1);
        }
        #endif
    }
    ca->block_count--;
    update_free(ca);
}

carena_status carena_push(carena* ca, void* data, size_t n){
    if(ca == NULL || ca->buff == NULL || data == NULL)
        return CARENA_ERR_INVALID;

    size_t padding   = align_up(n, alignof(mdt_t)) - n;
    size_t blocksize = SZHEAD_T + n + padding + SZMETA_T;
    size_t dead_space= ca->size - ca->top;

    if( blocksize > MDTMAX)
        return CARENA_ERR_TOO_LARGE;

    if( blocksize > ca->size )
        return CARENA_ERR_TOO_LARGE;

    void* curr_block_hdr_ptr = ca->buff + ca->top;

    /*
    write_ptr: pointer where the block will be written.
                It is computed from ca->top, so it must
                be computed only AFTER ca->top is computed
                
                That includes the block eviction loop
                since it can set ca->top = 0 when the
                arena is emptied fully.  
    */
    void* write_ptr;
        
    // Prepare header and footer content for the block
    size_t curr_block_bnb = blocksize;
    size_t curr_block_bpb = blocksize;

    if( ca->top == ca->bot ){
        ca->top = 0;
        ca->bot = 0;
        #ifdef _CARENA_DEBUG
        if(ca->block_count != 0){
            printf("top=bot but block_count!=0\n");
            exit(1);
        }
        #endif
    }

    
    if(ca->top > ca->bot && blocksize > dead_space){

        // Patch previous header and footer to include
        // 'dead space' between the ca->top and end of buff
        mdt_t  bpb = *(mdt_t*)(curr_block_hdr_ptr - SZMETA_T); 
        void*  prev_block_hdr_ptr  = curr_block_hdr_ptr - bpb;
        size_t tmp = ((hdr_t*)prev_block_hdr_ptr)->bytes_next_block + dead_space;

        curr_block_bpb += dead_space;

        // curr_block_bpb and bytes_next_block have just 
        // been incremented.. need to check for overflow
        // Superfluous if _CARENA_STRICT_SIZE is set
        #ifdef _CARENA_STRICT_SIZE
        if( tmp > MDTMAX || curr_block_bpb > MDTMAX)
            return CARENA_ERR_TOO_LARGE;
        #endif

        patch_header(prev_block_hdr_ptr, dead_space);

        // Wrap top to 0, Update ca->free bytes
        // to reflect the new top
        ca->top = 0;
        update_free(ca);
    }


    // Evict old blocks
    while( blocksize > ca->free )
        carena_evict_oldest(ca);


    write_ptr = ca->buff + ca->top;

    // Write block
    void* curr_block_payl_ptr = write_ptr + SZHEAD_T;
    void* curr_block_pad_ptr  = curr_block_payl_ptr + n;
    void* curr_block_ftr_ptr  = curr_block_pad_ptr  + padding;

    // Writes here a guaranteed against overflow
    write_header(write_ptr, curr_block_bnb,       n);
    write_footer(curr_block_ftr_ptr, curr_block_bpb);

    memcpy(curr_block_payl_ptr, data,   n);
    memset(curr_block_pad_ptr, 0, padding);

    ca->top += blocksize;
    ca->block_count++;
    update_free(ca);

    #ifdef _CARENA_DEBUG
    if(ca->top == ca->bot){
        printf("Structure Corrupted\n");
        exit(2);
    }
    #endif

    return CARENA_OK;

}


carena_status carena_pop(carena* ca){

    if( ca == NULL || ca->buff == NULL)
        return CARENA_ERR_INVALID;

    if( ca->block_count == 0)
        return CARENA_ERR_EMPTY;

    
    mdt_t bpb = *(mdt_t*)(ca->buff + ca->top - SZMETA_T);
    ca->top = (ca->top - bpb + ca->size) % ca->size;

    // When there are at least 2 blocks and we pop, we must
    // patch back the header of the first block because it
    // might include some dead space byte counts. This might
    // happen when a block is pushed that does not fit, and
    // then immediately popped.
    if (ca->block_count > 1) {
        void* prev_block_ftr_ptr = ca->buff + ca->top - SZMETA_T;
        mdt_t prev_block_size    = *(mdt_t*)(prev_block_ftr_ptr);
        void* prev_block_hdr_ptr = ca->buff + ca->top - prev_block_size;
        ((hdr_t*)(prev_block_hdr_ptr))->bytes_next_block = prev_block_size;
    }
    ca->block_count--;
    update_free(ca);
    return CARENA_OK;
}
