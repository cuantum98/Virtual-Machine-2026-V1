#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define 1Bt 0xFF000000; // 4 mascaras constantes que sirven para sacar los 4 distintos bytes de un int32_t.
#define 2Bt 0x00FF0000; // =
#define 3Bt 0x0000FF00; // =
#define 4Bt 0x000000FF; // =

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
void loadCodeSize(FILE *file, int16_t *size) {
    fread(size, sizeof(int16_t), 1, file);
}

//funcion que carga el codigo en la memoria principal.
void loadCode(FILE *file, int16_t size, int32_t *mainMemory) {
    int resto = size%4;
    int i;
    size = size div 4;
    if (resto>0){
        size++;
    }
    for (i=0; i<size; i++){
        fread(&mainMemory[i], sizeof(int32_t), 1, file);
    }
    mainMemory[size] = mainMemory[size] << resto*8;
}

void main(){
    int32_t registers[32]={0}; //serian los 32 registros de 4 bytes que se piden (aunque solo se usen 17 por ahora)
    int32_t mainMemory[4096]={0}; //seria la memoria principal de 16kib
}