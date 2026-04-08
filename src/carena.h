#pragma once

/*
 * carena.h
 *
 * COMPILE-TIME CONFIGURATION FLAGS
 *
 * Define these before including this header, or pass via compiler flags (-D).
 *
 * _CARENA_STRICT_SIZE
 *   Enforces maximum size <= MDTMAX at carena_init time.
 *   Prevents wrap-around overflow at the cost of smaller max arena size.
 *   
 *
 * _CARENA_WIDTH  (default: 16)
 *   Determines maximum block size and header/footer overhead.
 *   Valid values: 8, 16, 32
 *   Example: -DCARENA_MDT_WIDTH=32
 *
 * _DEBUG
 *   Enables internal consistency checks and diagnostic printf output.
 * 
*/


#define _CARENA_STRICT_SIZE
#define _CARENA_WIDTH 16
#define _CARENA_DEBUG

 
typedef struct carena carena;

typedef enum {
    CARENA_OK = 0,
    CARENA_ERR_INVALID,   // NULL/uninitialized — programmer error
    CARENA_ERR_TOO_LARGE, // payload can never fit structurally
    CARENA_ERR_EMPTY,     // pop on empty arena
} carena_status;

carena_status carena_init(carena* ca, size_t n);
carena_status carena_free(carena* ca);
carena_status carena_push(carena* ca, void* data, size_t n);
carena_status carena_pop (carena* ca);


