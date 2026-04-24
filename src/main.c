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
    chip8Init(&cipollotto, CIPOLLOTTO_VARIANT_SCHIP, argv[1]);

    if(ui_init() != 0) return -1;
    ui_refresh();


    #define NANOS_IN_A_S 1000000000L
    long framesPerSecond        = cipollotto.clock.FPS;
    long instructionsPerSecond  = cipollotto.clock.IPS;
    long instructionsPerFrame   = instructionsPerSecond / framesPerSecond;
    long frameTimeNs            = NANOS_IN_A_S / framesPerSecond;
    long extraInstr = 0;
    /*
        Ogni frame eseguo un batch di istruzioni chip-8. Se FPS divide
        IPS, eseguo semplicemente IPS/FPS istruzioni ogni frame. Chip8
        di solito viene emulato a 500 IPS, 60 FPS, quindi 500/60=8.333
        istruzioni al frame. Che farne della parte 0.333? Faccio frame
        da 8 istruzioni e quello dopo da 9, in modo che in media, su 2
        frame, avrò fatto: (8+9)/2 = 8,5. extraInstr è la 9 istruzione
        da fare nei frame dispari.
    */
    struct timespec TnextRefresh;
    long instrCounter = 0;
    long frameCounter = 0;

    info_print("Rom:%s\nVariant:%d\nState size:%d",
               argv[1], cipollotto.variant, sizeof(chip8));
    
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
        
        if(cipollotto.runState == CIPOLLOTTO_STATUS_PAUSED){
            ui_refresh();
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &TnextRefresh, NULL);
            continue;
        }

        long k;
        for(k=0; k<(instructionsPerFrame + extraInstr); k++){
            cipollotto.opExecute(&cipollotto);
        }
        instrCounter += k;
        extraInstr = !(extraInstr & 1L);

        //update timers
        if(cipollotto.state.DT > 0){cipollotto.state.DT--;}
        if(cipollotto.state.ST > 0){cipollotto.state.ST--;} //should emit sound

        if(cipollotto.drawFlag){
            renderFBtoUI(&cipollotto);
            cipollotto.drawFlag = false;
            ui_refresh();
        }

        frameCounter++;
        
        //wait untill next frame
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &TnextRefresh, NULL);
    }
    
    memdump(&cipollotto, "RAM.bin");
    ui_deinit();
    return 0;
}
