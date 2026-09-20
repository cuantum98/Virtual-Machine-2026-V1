#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


// Son todas las operaciones con las que trabajamos, es necesario para calcular el CC.
typedef enum {
    OP_MOV,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_CMP,
    OP_AND,
    OP_OR,
    OP_XOR,
    OP_NOT,
    OP_SHL,
    OP_SHR,
    OP_SAR,
    OP_SWAP
} OpType;

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
int verifyFile(FILE *file) {
    char id[6]={0};
    int8_t version;
    int16_t tamaño;

    if (fread(id, sizeof(char), 5, file)!=5 || strcmp(id,"VMX26")!=0){
        fclose(file);
        return 1;
    }
    else
        if (fread(&version, sizeof(int8_t), 1, file)!=1 || version!=1){
            fclose(file);
            return 1;
        }else
            return 0;
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
    registers[CS] =0;
    registers[DS] =1;
    registers[DS] = (registers[DS]<<16);
    registers[IP] = registers[CS];
    registers[AC] = 0;
}


//Funcion para obtener la direccion fisica a partir de la logica.
int32_t getDir(int32_t logicDir,int32_t listSegments[]){
    int lowByte=logicDir & L2Bt;
    int highByte=logicDir & H2Bt;
    int base=listSegments[highByte] & H2Bt;
    return (lowByte+base);
}


//Funcion que se encarga de obtener los datos de los operandos.
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
    int8_t operacion=mainMemory[registers[IP]]; //Traigo la operacion del IP
    registers[OPC]= operacion & 0b11111; //Obtengo codigo de operacion
    int32_t tipoa= (operacion & 0b00110000)>>4; //tipo operando A
    int32_t tipob= (operacion & 0b11000000)>>6;//tipo operando B
    registers[OP1]=(tipoa<<24);
    registers[OP2]=(tipob<<24);
    registers[OP2]+=getOp(tipob,mainMemory,registers[0]+1);
    registers[OP1]+=getOp(tipoa,mainMemory,registers[0]+1+tipob);
    registers[IP]+=1+tipoa+tipob;//Sumo el tamaño de la operacion
}


//funcion auxiliar para pasar un string binario a su valor numerico.
int stringToInt(char *bin){
    int resultado = 0;

    for (int i = 0; bin[i] != '\0'; i++) {
        if (bin[i] == '1') {
            resultado = resultado * 2 + 1;
        } else if (bin[i] == '0') {
            resultado = resultado * 2;
        }
    }
    return resultado;
}


//funcion auxiliar para transformar un numero entero a su representacion binaria como cadena de caracteres.
void intToString(char *auxS, int numero) {
    int i = 0;
    int n = numero;

    if (n == 0) {
        auxS[i++] = '0';
    } else {
        while (n != 0) {
            auxS[i++] = (n % 2) + '0';
            n /= 2;
        }
    }

    auxS[i] = '\0';

    // invertir la cadena
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = auxS[start];
        auxS[start] = auxS[end];
        auxS[end] = temp;
        start++;
        end--;
    }
}


//Funcion que carga una variable cargada en MBR a memoria dependiendo del MAR. 
void loadInMemory(int32_t *registers, int8_t mainMemory){
    int i;
    int opSize = (registers[MAR] & H2Bt) >> 16;
    int dir = registers[MAR] & L2Bt;
    int aux = registers[MBR];

    for (i=opSize-1;i>=0;i--){
        mainMemory[dir+i]=(int8_t)(aux & 0xFF);
        aux = aux>>8;
    }
}


//Funcion que lee una variable de la memoria y la carga en el MBR dependiendo del MAR. 
void readFromMemory(int32_t *registers, int8_t mainMemory){
    int i;
    int opSize = (registers[MAR] & H2Bt) >> 16;
    int dir = registers[MAR] & L2Bt;

    for (i=0;i<opSize;i++){
        registers[MBR] = registers[MBR]<<8;
        registers[MBR] += mainMemory[dir+i];
    }
}


//Funcion SYS
void Sys (int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int op=registers[OP2] & 0x00FFFFFF;
    char *auxS;
    int32_t aux=registers[ECX]&2LBt, tam=(registers[ECX]&H2Bt)>>16;
    registers[LAR]=registers[EDX];
    registers[MAR]=(registers[ECX]&H2Bt) + getDir(registers[LAR],listSegments);

    if(op==1){
        switch (registers[EAX])
        {
        case 1:
            for (i=0;i<aux;i++){
                scanf(" %d",&registers[MBR]);
                loadInMemory(registers,mainMemory);
                registers[MAR]+=i*tam;
            }    
        break;
        case 2:
            for (i=0;i<aux;i++){
                    scanf(" %c",&registers[MBR]);
                    loadInMemory(registers,mainMemory);
                    registers[MAR]+=i*tam;
            }
        break;
        case 4:
            for (i=0;i<aux;i++){
                scanf(" %o",&registers[MBR]);
                loadInMemory(registers,mainMemory);
                registers[MAR]+=i*tam;
            }
        break;
        case 8:
            for (i=0;i<aux;i++){
                scanf(" %x",&registers[MBR]);
                loadInMemory(registers,mainMemory);
                registers[MAR]+=i*tam;
            }
        break;
        case 10:
            for (i=0;i<aux;i++){
                scanf(" %s",auxS);
                registers[MBR]=(int32_t)stringToInt(auxS);
                loadInMemory(registers,mainMemory);
                registers[MAR]+=i*tam;
            }
        break;
        }
    }
    else{
        switch (registers[EAX])
        {
        case 1:
            for (i=0;i<aux;i++){
                readFromMemory(registers,mainMemory);
                printf(" %d",registers[MBR]);
                registers[MAR]+=i*tam;
            }    
        break;
        case 2:
            for (i=0;i<aux;i++){
                readFromMemory(registers,mainMemory);
                    printf(" %c",registers[MBR]);
                    registers[MAR]+=i*tam;
            }
        break;
        case 4:
            for (i=0;i<aux;i++){
                readFromMemory(registers,mainMemory);
                printf(" %o",registers[MBR]);
                registers[MAR]+=i*tam;
            }
        break;
        case 8:
            for (i=0;i<aux;i++){
                readFromMemory(registers,mainMemory);
                printf(" %x",registers[MBR]);
                registers[MAR]+=i*tam;
            }
        break;
        case 10:
            for (i=0;i<aux;i++){
                readFromMemory(registers,mainMemory);
                intToString(auxS,registers[MBR]);
                printf("%s",auxS);
                registers[MAR]+=i*tam;
            }
        break;
        }
    }
}


// Funcion para setear los valores del registro CC.
void setCC(OpType op, int32_t a, int32_t b, int32_t resultado, int32_t *registers) {
    int N = 0, Z = 0, C = 0, V = 0;

    // N y Z se calculan igual para todas las operaciones
    N = (resultado < 0) ? 1 : 0;
    Z = (resultado == 0) ? 1 : 0;

    // C y V dependen de la operación
    switch (op) {
        case OP_ADD: {
            uint64_t suma = (uint64_t)(uint32_t)a + (uint64_t)(uint32_t)b;
            C = (suma > 0xFFFFFFFFULL) ? 1 : 0;
            V = (((a ^ resultado) & (b ^ resultado)) >> 31) & 1;
            break;
        }

        case OP_SUB:
        case OP_CMP: {
            C = ((uint32_t)a < (uint32_t)b) ? 1 : 0;
            V = (((a ^ b) & (a ^ resultado)) >> 31) & 1;
            break;
        }

        case OP_MUL: {
            int64_t prod = (int64_t)a * (int64_t)b;
            C = (prod != (int64_t)(int32_t)prod) ? 1 : 0;
            V = C;
            break;
        }

        case OP_DIV: {
            if (b == 0) {
                C = 0;
                V = 1;
            } else if (a == INT32_MIN && b == -1) {
                C = 0;
                V = 1;
            } else {
                C = 0;
                V = 0;
            }
            break;
        }

        case OP_SHL: {
            if (b > 0 && b <= 32) {
                C = ((uint32_t)a >> (32 - b)) & 1;
                V = ((a >> 31) != (resultado >> 31)) ? 1 : 0;
            } else {
                C = 0;
                V = 0;
            }
            break;
        }

        case OP_SHR: {
            if (b > 0 && b <= 32) {
                C = ((uint32_t)a >> (b - 1)) & 1;
            }
            V = 0;
            break;
        }

        case OP_SAR: {
            if (b > 0 && b <= 32) {
                C = ((uint32_t)a >> (b - 1)) & 1;
            }
            V = 0;
            break;
        }

        case OP_MOV:
        case OP_AND:
        case OP_OR:
        case OP_XOR:
        case OP_NOT:
        case OP_SWAP:
            C = 0;
            V = 0;
            break;
    }

    // Escribir N, Z, C, V en los bits 31, 30, 29, 28 del registro CC
    // Preserva los 28 bits reservados
    registers[CC] = (registers[CC] & 0x0FFFFFFF) | ((uint32_t)N << 31) | ((uint32_t)Z << 30) | ((uint32_t)C << 29) | ((uint32_t)V << 28);
}

void Jmp (int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int8_t tipo = registers[OP2] >>> 24; //leo el tipo de operando, que es su byte mas significativo, OP1 no guarda su tipo??
    //el OP1 deberia guardar en su byte mas significativo, no lo hace el getOP?
    switch (tipo){ //debo hacer desplazamiento logico o aritmetico? o da igual?
        case '1': //operando de registro
            int8_t codReg = registers[OP2] & 0b11111  //int8_t o solo int?
            registers[IP] =  registers[registers[codReg]];
            break;
        case '2': //operando inmediato
            int16_t valor = registers[OP2] & 0xFFFF;
            registers[IP] = valor; //debo validar que no se salga del code segment?
            break;
        case '3': //operando de memoria
            int16_t offset = ( (registers[OP2] << 8) >>> 16) ; //saco el codigo de operando con el <<8
            int8_t codReg = registers[OP2] && 0b11111;
            registers[IP] = registers[codReg] + offset;
    }

}

void LDL (int8_t *mainMemory, int32_t *registers,int32_t listSegments[]) {
    int8_t tipo = registers[OP2] >> 24; 

    switch (tipo) {
        case '1': //operando de registro
            int16_t codReg = registers[OP2] & 0b11111; //obtengo el codigo
            int16_t valor = registers[codReg] & 0xFFFF0000; 
            break;
        case '2': //operando inmediato
            int16_t valor = registers[OP2] & 0xFFFF;
            break;
        case '3': //operando de memoria;
            int16_t offset = registers[OP2] & 0xFFFF00;
            int8_t codReg = registers[OP2] & 0b11111;
            int16_t valor = mainMemory[getDir(registers[codReg]) + offset];
    }
    
    tipo = registers[OP1] >> 24;

    switch (tipo) {
        case '1': 
            codReg = registers[OP1] & 0b11111;
            registers[codReg] &= valor; //sobrescribe los 2 bits menos significativos
            break;
        case '3': //operando de memoria
            offset = registers[OP1] & 0xFFFF00;
            codReg = registers[OP1] & 0b11111;
            mainMemory[getDir(registers[codReg]) + offset] &= valor; 
    }


}

void LDH (int8_t *mainMemory, int32_t *registers,int32_t listSegments[]) {
     int8_t tipo = registers[OP2] >> 24; 

    switch (tipo) {
        case '1': //operando de registro
            int16_t codReg = registers[OP2] & 0b11111; //obtengo el codigo
            int16_t valor = registers[codReg] & 0xFFFF; //los 2 bytes menos significativos del registro que apunta el OP2
            break;
        case '2': //operando inmediato
            int16_t valor = registers[OP2] & 0xFFFF;
            break;
        case '3': //operando de memoria;
            int16_t offset = registers[OP2] & 0xFFFF00;
            int8_t codReg = registers[OP2] & 0b11111;
            int16_t valor = mainMemory[getDir(registers[codReg]) + offset];
    }
    
    tipo = registers[OP1] >> 24;

    switch (tipo) {
        case '1': 
            codReg = registers[OP1] & 0b11111;
         registers[codReg] &= (valor<<16) +0xFFFF; //sobrescribe los 2 bits menos significativos
            break;
        case '3': //operando de memoria
            offset = registers[OP1] & 0xFFFF00;
            codReg = registers[OP1] & 0b11111;
            mainMemory[getDir(registers[codReg]) + offset] &= (valor<<16) +0xFFFF; 
    }

}

void MOV(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int8_t tipo = registers[OP2] >> 24; 

    switch (tipo) {
        case '1': //operando de registro
            int16_t codReg = registers[OP2] & 0b11111; //obtengo el codigo
            int16_t valor = registers[codReg];
            break;
        case '2': //operando inmediato
            int16_t valor = registers[OP2] & 0xFFFF;
            break;
        case '3': //operando de memoria;
            int16_t offset = (registers[OP2] & 0x00FFFF00) >> 8;
            int8_t codReg = registers[OP2] & 0b11111;
            int16_t valor = mainMemory[getDir(registers[codReg]) + offset];
    }

    tipo = registers[OP1] >> 24;

    switch (tipo) {
        case '1': //Operando de registro
            codReg = registers[OP1] & 0b11111;
            registers[codReg] = valor; //
            break;
        case '3': //operando de memoria
            offset = (registers[OP1] & 0x00FFFF00) >> 8;
            codReg = registers[OP1] & 0b11111;
            mainMemory[getDir(registers[codReg]) + offset] = valor; 
    }
    setCC(OP_MOV,)
}

void main(){
    int16_t cSize=0;
    int32_t registers[32]={0}; //serian los 32 registros de 4 bytes que se piden (aunque solo se usen 17 por ahora)
    int32_t listSegments[8]={0}; //serian los 8 segmentos de 4 bytes que se piden
    int8_t mainMemory[16384]={0}; //seria la memoria principal de 16kib
}