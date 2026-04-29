#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdalign.h>
#include <string.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

void* rle_encode(void* s, size_t n){
    u8  previous  = ((u8*)s)[0];
    u8* source    = (u8*)s;
    size_t   previousN = 1, blkCounter = 0;
    size_t   frequencies[256] = {0};

    // count occurences of a value
    // to determine encoded data size
    for(size_t i = 0; i < n; i++)
        frequencies[source[i]]++;

    size_t msize = 0;
    
    for(size_t i = 0; i < 256; i++){
        if(frequencies[i] != 0)
            msize+=(frequencies[i] / 256 + 1);
    }
    msize*=2;
    u8* dest = (u8*)malloc(msize);
    
    for(size_t i = 1; i < n; i++){
        
        u8 fetched = source[i];
        if(fetched != previous){
            dest[2*blkCounter+1] = previousN;
            dest[2*blkCounter  ] = previous;
            previous    = fetched;
            previousN   = 1;
            blkCounter++;
            continue;
        }

        previousN++;
        if(previousN > 255){
            dest[2*blkCounter+1] = 255;
            dest[2*blkCounter  ] = previous;
            previousN = 1;
            blkCounter++;
        }

    }
    
    dest[2*blkCounter]   = previous;
    dest[2*blkCounter+1] = previousN;
    return dest;
}





u8 data[] = {  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // (255, 16)
                    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
                    0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa, // (170, 8)
                    0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,
                    0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,
                    0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,
                    0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,
                    0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,
                    0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,
                    0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,
                    0xbb,  // (187, 320)
};


u8 dummydata[128] = { [0 ... 128-1] = 0xFF };

void xor(void* a, void* b, void* c, size_t n){
    for(size_t i = 0; i < n; i++)
        ((u8*)c)[i] = ((u8*)a)[i] ^ ((u8*)b)[i];
}

/**
 * Generates a buffer where the data is a function of the index and a seed.
 * @param size The number of bytes to allocate.
 * @param seed An offset value to shift the data pattern.
 * @return A pointer to the allocated buffer, or NULL if allocation fails.
 */
uint8_t* garbage(size_t size, uint8_t seed) {
    uint8_t *buffer = (uint8_t*)malloc(size);

    if (buffer == NULL) {
        return NULL;
    }

    // A more chaotic pattern that still depends entirely on the seed
    for (size_t i = 0; i < size; i++) {
        // Multiplying by a prime number creates a pseudo-random distribution
        buffer[i] = (uint8_t)(((i * 31) + seed) % 256);
    }

    return buffer;
}

int main(int argc, char* argv[]){
    //void* encoded = rle_encode(data, sizeof(data)/sizeof(u8));
    size_t s = 1024;
    u8* g1 = garbage(s, 151);
    u8* g2 = garbage(s, 172);
    u8* c1 = (u8*)malloc(s);

    xor(g1, g2, c1, s);
    
    return 0;
}

