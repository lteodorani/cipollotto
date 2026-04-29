#include "common.h"

static struct{
    size_t* buff;
    size_t  top;
    size_t  size;
    size_t  elcount;
} cqueue = {
    .buff = 
}



cqueue_init(size_t n){
    //check pointers, accepts only powers of two
    if(cs == NULL || __builtin_popcountll(n) != 1)
        return false;

    cs->buff = NULL;

    cs->buff = (size_t*)calloc(n, sizeof(size_t));
    if(cs->buff == NULL)
        return false;

    cs->size = n;
    cs->top  = 0;
    cs->elcount = 0;
    return true;
}


bool cqueue_push(cqueue* cs, size_t v){
    if(cs == NULL || cs->buff == NULL)
        return false;
    
    cs->buff[cs->top] = v;
    cs->top = (cs->top + 1) % cs->size; // ACTHUNG?! see _pop modulo logic
    if(cs->elcount < cs->size)
        cs->elcount++;
    
    return true;
}

bool cqueue_pop(cqueue* cs, size_t* d){

    // err if cs, d or cs initialized, err also if queue is empty
    if(cs == NULL || d == NULL || cs->buff == NULL || cs->elcount == 0)
        return false;
    
    /*
     The correct formula for unsigned indices is
        cs->top = (cs->top + cs->size - 1) % cs->size
        
     Here n is a power of two, so we can use instead
        cs->top = (cs->top - 1) % cs->size
    */
    cs->top = (cs->top - 1) % cs->size;
    cs->elcount--;
    *d = cs->buff[cs->top];
    return true;
    
}

void cqueue_print(cqueue* cs){
    for(size_t i = 0; i < cs->size; i++)
        printf("[%03lu]", cs->buff[i]);

    printf("\n");
    for(size_t i = 0; i< cs->size; i++){
        if(i == cs->top && i == (cs->top - cs->elcount))
            printf(" b|t ");

        else if(i == (cs->top - cs->elcount))
            printf("  b  ");

        else if(i == cs->top)
            printf("  t  ");

        else
            printf("  %lu  ", i);
    }
    printf(" (%lu)\n\n", cs->elcount);
}