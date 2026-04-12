#include "cipollotto.h"
#include "ui.h"
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char** argv)
{
    if(argc == 1){
        printf("Please provide a .c8 ROM file\n");
        return -1;
    }

    FILE* fROM = fopen(argv[1], "rb");
    if(fROM == NULL){
        printf("Error occured opening %s\n", argv[1]);
        return 1;
    }

    fseek(fROM, 0, SEEK_END);
    long fsize = ftell(fROM);
    fseek(fROM, 0, SEEK_SET);
    if(fsize > (MEMORY_END - PROG_START_ADDR)){
        printf("Provided ROM exceeds chip-8 memory limit of 4kB.\n");
        return -2;
    }



    chip8 cipollotto;
    chip8Init(&cipollotto, CIPOLLOTTO_VARIANT_SCHIP);
    size_t read = fread(cipollotto.MEM+PROG_START_ADDR, 1, fsize, fROM);
    if(read != (size_t)fsize)
        return -4;
    fclose(fROM);


    if(ui_init() != 0) return -1;
    ui_refresh();


    #define NANOS_IN_A_S 1000000000L
    long framesPerSecond        = cipollotto.chipOptions.FPS;
    long instructionsPerSecond  = cipollotto.chipOptions.IPS;
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

    info_print("Rom:%s\nVariant:%d",
               argv[1], cipollotto.chipVariant);
    
    clock_gettime(CLOCK_MONOTONIC, &TnextRefresh);
    while(cipollotto.running != CIPOLLOTTO_STATUS_HALTED){
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
        
        if(cipollotto.running == CIPOLLOTTO_STATUS_PAUSED){
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
        if(cipollotto.DT > 0){cipollotto.DT--;}
        if(cipollotto.ST > 0){cipollotto.ST--;} //should emit sound

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
