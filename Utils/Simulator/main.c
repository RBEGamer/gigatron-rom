#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


typedef struct {
    uint16_t PC;
    uint8_t IR, D ,AC ,X ,Y, OUT, undef;
} CpuState;
uint8_t ROM[1<<16][2];
uint8_t RAM[1<<15];
uint8_t IN = 0xff;

CpuState cpuCycle(const CpuState S){
    CpuState T = S;

    T.IR = ROM[S.PC][0];
    T.D = ROM[S.PC][1];

    int ins = S.IR >> 5; //instruction
    int mod = (S.IR >> 2) & 7;//addr mode or condition
    int bus = S.IR & 3; //busmode
    int W = (ins == 6);//write instruction
    int J = (ins == 7); //jump instruction

    uint8_t lo= S.D;
    uint8_t hi = 0;
    uint8_t *to = NULL;

    int incX = 0;

    if(!J){
        switch (mod) {
#define E(p) (W?0:p) //diable AC and out loading whil ram write
            case 0: to = E(&T.AC);break;
            case 1: to = E(&T.AC);lo = S.X;break;
            case 2: to = E(&T.AC);hi=S.Y;break;
            case 3: to = E(&T.AC);lo = S.X;hi=S.Y;break;
            case 4: to = &T.X;break;
            case 5: to =&T.Y;break;
            case 6: to = E(&T.OUT);break;
            case 7: to = E(&T.OUT);lo = S.X; hi= S.Y; incX=1;break;

        }
    }
    uint16_t  addr = (hi << 8) | lo;

    int B = S.undef; //DATABUS
    switch (bus) {
        case 0: B=S.D;break;
        case 1: if(!W){B=RAM[addr&0x7fff];};break;
        case 2: B=S.AC;break;
        case 3: B=IN;break;
    }
    if(W){
        RAM[addr&0x7fff] = B;
    }

    uint8_t ALU;
    switch (ins) {
        case 0: ALU = B; break;
        case 1:ALU = S.AC & B; break;
        case 2: ALU = S.AC | B; break;
        case 3: ALU = S.AC^B;break;
        case 4: ALU = S.AC+B;break;
        case 5: ALU = S.AC-B;break;
        case 6: ALU = S.AC;break;
        case 7: ALU = -S.AC;break;
    }

    if(to){
        *to = ALU;
    }
    if(incX){
        T.X = S.X +1;
    }
    T.PC = S.PC +1;

    if(J){
        if(mod != 0){
            int cond = (S.AC>>7) + 2*(S.AC==0);
            if(mod & (1 << cond)){
                T.PC = (S.PC & 0xff00) | B;
            }else{
                T.PC = (S.Y << 8) | B;
            }
        }
    }
    return T;
}

void garble(uint8_t _mem[], int _len){
    for (int i = 0; i < _len; ++i) {
        _mem[i] = rand();
    }
}

int main(){
    CpuState cpuState;
    srand(time(NULL));
    garble((void*)ROM, sizeof(ROM));
    garble((void*)RAM, sizeof(RAM));
    garble((void*)&cpuState, sizeof(cpuState));

    FILE *fp = fopen("../../../gigatron.rom", "rb");
    if(!fp){
        fprintf(stderr,"FAILED TO OPEN ROM FILE");
        exit(EXIT_FAILURE);
    }
    fread(ROM,1,sizeof(ROM), fp);
    fclose(fp);
    int vgaX = 0, vgaY = 0;
    for (long   t = -2; ; t++) {
        if(t < 0){
            cpuState.PC = 0; //RESET
        }
        CpuState T = cpuCycle(cpuState);

        int hSync = (T.OUT & 0x40) - (cpuState.OUT & 0x40);
        int vSync = (T.OUT & 0x80) - (cpuState.OUT & 0x80);

        if(vgaX++ < 200){
            if(hSync){
                putchar('|');
            }else if(vgaX == 200){
                putchar('>');
            } else if(~cpuState.OUT & 0x80) {
                putchar('^');
            }else{
                putchar(32+(cpuState.OUT & 63));
            }

            if(hSync > 0){
                printf("%s line %d xout ",vgaX,T.AC);
            }
            vgaX = 0;
            vgaY++;
            T.undef = rand() & 0xff;
        }
        cpuState = T;
    }
    return 0;
}