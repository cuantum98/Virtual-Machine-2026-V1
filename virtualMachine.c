#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MAXMEMORY 16384

#define 2HBt 0xFFFF0000; //Constantes para tomar los 2 bytes más significativos y los 2 menos significativos.
#define 2LBt 0x0000FFFF; 


//funcion que verifica si el archivo es valido, devuelve true si lo es y false si no lo es.
// Ademas cierra el archivo en caso de que no sea valido.
bool verifyFile(FILE *file) {
    char id[6]={0};
    int8_t version;
    int16_t tamaño;

    if (fread(id, sizeof(char), 5, file)!=5 || strcmp(id,"VMX26")==0){
        fclose(file);
        return false;
    }
    else
        if (fread(&version, sizeof(int8_t), 1, file)!=1 || version!=1){
            fclose(file);
            return false;
        }else
            return true;
}


//funcion que carga el tamaño del codigo en la variable size.
void loadCodeSize(FILE *file, int16_t *cSize) {
    fread(size, sizeof(int16_t), 1, file);
}


//funcion que carga el codigo en la memoria principal.
void loadCode(FILE *file, int16_t cSize, int8_t *mainMemory) {
    int i;
    for (i=0; i<cSize; i++){
        fread(&(mainMemory[i]), sizeof(int8_t), 1, file);
    }
    fclose(file);
}


//Funcion que inicializa la lista de segmentos.
void setSegments(int32_t listSegments[], int16_t cSize){
    //por ahora solo hay 2 segmentos (cs y ds) se inicializan en 0 y 1 respectivamente
    int i=2;
    listSegments[0]=cSize;
    listSegments[1]=cSize; 
    listSegments[1]=listSegments[1] << 16;     //Sumo a 0 los 2 bytes más significativos, luego hago un shif left de 2 bytes 
    listSegments[1]+= (MAXMEMORY - cSize);     //y sumo los bytes menos significativos 

    while (i < 8){
        listSegments[i]=0xFFFFFFFF;
    }
}


//Funcion para inicializar los registros más importantes.
void setRegisters(int32_t registers[32],int16_t cSize){
    int CS=26, DS=27, AC=16;
    registers[CS]=0;
    registers[DS]=1;
    registers[DS]= (registers[DS]<<16) +cSize;
    registers[0]=registers[CS];
    registers[AC]=0;
}


//Funcion para obtener la direccion fisica a partir de la logica.
int getDir(int32_t logicDir){
    int lowByte=logicDir & 2LBt;
    int highByte=logicDir & 2HBt;
    return (lowByte+highByte);
}

int32_t getOp(int tipoa,int tipob, int8_t *mainMemory,int ip){
    
}

void readNextInst(int8_t *mainMemory, int32_t *registers){
    int8_t operacion=mainMemory[registers[0]];
    registers[1]= operacion & 0b11111;
    registers[2]= (operacion & 0b00110000)>>4;
    registers[3]= (operacion & 0b11000000)>>6;
}

void main(){
    int16_t cSize=0;
    int32_t registers[32]={0}; //serian los 32 registros de 4 bytes que se piden (aunque solo se usen 17 por ahora)
    int32_t listSegments[8]={0}; //serian los 8 segmentos de 4 bytes que se piden
    int8_t mainMemory[16384]={0}; //seria la memoria principal de 16kib
}