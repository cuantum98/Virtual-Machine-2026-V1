#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//Instrucción 
#define IP   0
#define OPC  1
#define OP1  2
#define OP2  3

// Acceso a memoria
#define LAR  4
#define MAR  5
#define MBR  6

// Registros de propósito general
#define EAX  10
#define EBX  11
#define ECX  12
#define EDX  13
#define EEX  14
#define EFX  15

// Registros de estado control 
#define AC   16
#define CC   17

// Registros de segmento 
#define CS   26
#define DS   27

#define MAXMEMORY 16384

#define H2Bt 0xFFFF0000//Constantes para tomar los 2 bytes más significativos y los 2 menos significativos.
#define L2Bt 0x0000FFFF 


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
    fread(cSize, sizeof(int16_t), 1, file);
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
        i++;
    }
}


//Funcion para inicializar los registros más importantes.
void setRegisters(int32_t registers[32],int16_t cSize){
    registers[CS]=0;
    registers[DS]=1;
    registers[DS]= (registers[DS]<<16) +cSize;
    registers[IP]=registers[CS];
    registers[AC]=0;
}


//Funcion para obtener la direccion fisica a partir de la logica.
int32_t getDir(int32_t logicDir,int32_t listSegments[]){
    int lowByte=logicDir & L2Bt;
    int highByte=logicDir & H2Bt;
    int base=listSegments[highByte] & H2Bt;
    return (lowByte+base);
}

int32_t getOp(int tipo, int8_t *mainMemory,int rindex){
    int32_t opnd=0;
    for(int i=rindex;i<rindex+tipo;i++){
        opnd=opnd<<8;
        opnd+=mainMemory[i];
    }
    return opnd;
    
    
}
//Funcion para leer la siguiente instruccion
void readNextInst(int8_t *mainMemory, int32_t *registers){
    int8_t operacion=mainMemory[registers[0]]; //Traigo la operacion del IP
    registers[1]= operacion & 0b11111; //Obtengo codigo de operacion
    int32_t tipoa= (operacion & 0b00110000)>>4; //tipo operando A
    int32_t tipob= (operacion & 0b11000000)>>6;//tipo operando B
    registers[2]=(tipoa<<24);
    registers[3]=(tipob<<24);
    registers[3]+=getOp(tipob,mainMemory,registers[0]+1);
    registers[2]+=getOp(tipoa,mainMemory,registers[0]+1+tipob);



    

    registers[0]+=1+tipoa+tipob;//Sumo el tamaño de la operacion
}

void Sys (int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int op=registers[OP2] & 0x00FFFFFF;
    registers[LAR]=registers[EDX];
    registers[MAR]=(registers[ECX]&H2Bt) + getDir(registers[LAR],listSegments);
    if(op==1){
        //Tomar el input como str(?)

    }



        

    

}


void main(){
    int16_t cSize=0;
    int32_t registers[32]={0}; //serian los 32 registros de 4 bytes que se piden (aunque solo se usen 17 por ahora)
    int32_t listSegments[8]={0}; //serian los 8 segmentos de 4 bytes que se piden
    int8_t mainMemory[16384]={0}; //seria la memoria principal de 16kib
}