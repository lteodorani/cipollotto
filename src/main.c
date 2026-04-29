#include "cipollotto.h"
#include "ui.h"
#include "common.h"

#include <stdio.h>
#include <unistd.h>
#include <time.h>


int main(int argc, char** argv)
{
    if(argc == 1){
        printf("Please provide a .c8 ROM file\n");
        return -1;
    }


    chip8 cipollotto;
    chip8Init(&cipollotto, CIPOLLOTTO_VARIANT_CHIP8, argv[1]);

    if(ui_init() != 0) return -1;


    /*
        Ogni frame eseguo un batch di istruzioni chip-8. Se FPS divide
        IPS, eseguo semplicemente IPS/FPS istruzioni ogni frame. Chip8
        di solito viene emulato a 500 IPS, 60 FPS, quindi 500/60=8.333
        istruzioni al frame. Che farne della parte 0.333? Faccio frame
        da 8 istruzioni e quello dopo da 9, in modo che in media, su 2
        frame, avrò fatto: (8+9)/2 = 8,5. extraInstr è la 9 istruzione
        da fare nei frame dispari.
    */

    #define NANOS_IN_A_S 1000000000LL
    u64 fps  = cipollotto.clock.FPS;
    u64 ips  = cipollotto.clock.IPS;
    u64 ipf  = ips / fps;
    u64 frameTimeNs            = NANOS_IN_A_S / fps;
    u64 extraInstr = 0;

    struct timespec TnextRefresh;
    long frameCounter = 0;

    clock_gettime(CLOCK_MONOTONIC, &TnextRefresh);
    while(cipollotto.runState != CIPOLLOTTO_STATUS_HALTED){
        /*
        Calcola tempo assoluto del prossimo frame.
        necessita clock_nanosleep(), che usa timespec.
        Dopo aver aggiunto al tempo corrente il frametime,
        bisogna normalizzare la timespec: se i ns sono + di
        10^9, aggiungi 1 ai secondi (e diminuisci di 10^9 i ns)
        */
        TnextRefresh.tv_nsec += frameTimeNs; 
        if (TnextRefresh.tv_nsec >= NANOS_IN_A_S) {
            TnextRefresh.tv_sec++;
            TnextRefresh.tv_nsec -= NANOS_IN_A_S;
        }

        ui_input(&cipollotto);
        
        if(cipollotto.runState == CIPOLLOTTO_STATUS_PAUSED ||
           cipollotto.runState == CIPOLLOTTO_STATUS_CRASHED){
            ui_presentFB(&cipollotto);
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &TnextRefresh, NULL);
            continue;
        }

        
        u64 C = cipollotto.clock.cycle + (ipf + extraInstr);
        while(cipollotto.clock.cycle <  C){
            cipollotto.opExecute(&cipollotto);
        }
        extraInstr = !(extraInstr & 1L);

        //update timers
        if(cipollotto.state.DT > 0){cipollotto.state.DT--;}
        if(cipollotto.state.ST > 0){cipollotto.state.ST--;}

        #ifdef DRAW_FLAG
        if(cipollotto.drawFlag){
            ui_renderFB(&cipollotto);
            cipollotto.drawFlag = false;
            ui_presentFB(&cipollotto);
        }
        #else
            ui_renderFB(&cipollotto);
            ui_presentFB(&cipollotto);
        #endif
        
        
        ui_audio(&cipollotto);
                
        //wait until next frame
        frameCounter++;
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &TnextRefresh, NULL);
    }
    
    ui_destroy();
    return 0;
}
