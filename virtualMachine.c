#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>


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

#define NUM_SEGMENTOS 2

#define H2Bt 0xFFFF0000//Constantes para tomar los 2 bytes más significativos y los 2 menos significativos.
#define L2Bt 0x0000FFFF
int verifyFile(FILE *file);

void loadCodeSize(FILE *file, int16_t *cSize);

void loadCode(FILE *file, int16_t cSize, int8_t *mainMemory);

void setSegments(int32_t listSegments[], int16_t cSize);

void setRegisters(int32_t registers[32]);

int getSegmentSize(int32_t reg, int32_t *listSegments);

int32_t getDir(int32_t logicDir, int32_t listSegments[]);

int32_t getOp(int tipo, int8_t *mainMemory, int rindex);

int32_t getValorOpnd(int32_t operando, int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void writeInOp1(int32_t valor, int8_t *mainMemory, int32_t *registers, int32_t listSegments[]);

void readNextInst(int8_t *mainMemory, int32_t *registers, int32_t listSegments[]);

int stringToInt(char *bin);

void loadInMemory(int32_t *registers, int8_t *mainMemory, int32_t listSegments[]);

void readFromMemory(int32_t *registers, int8_t *mainMemory, int32_t listSegments[]);

void SYS(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void setCC(OpType op, int32_t a, int32_t b, int32_t resultado, int32_t *registers);

void JMP(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void LDL(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void LDH(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void MOV(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void JN(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void JP(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void JZ(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void JC(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void JV(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void JNP(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void JNN(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void JNZ(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void NOT(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void ADD(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void SUB(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void MUL(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void DIV(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void XOR(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void SWAP(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void SHL(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void SHR(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void SAR(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void RND(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void CMP(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void AND(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void OR(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag);

void STOP(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag);

void INVALID(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag);

int main(int argc,char* argv[]){ //gcc virtualMachine.c -o vmx.exe -Wall -Wextra
    srand(time(NULL)); //inicializa semilla al iniciar programa
    int16_t cSize = 0;
    int32_t registers[32]={0}; //serian los 32 registros de 4 bytes que se piden (aunque solo se usen 17 por ahora)
    int32_t listSegments[8]={0}; //serian los 8 segmentos de 4 bytes que se piden
    int8_t mainMemory[16384]={0}; //seria la memoria principal de 16kib
    char *flagD, *opndNames[32]= {"SYS","JMP","JP","JN","JZ","JC","JV","JNP","JNN","JNZ","NOT"," "," "," "," ","STOP","MOV","ADD","SUB","MUL","DIV","CMP","AND","OR","XOR","SWAP","SHL","SHR","SAR","LDL","LDH","RND"};
    int flagaux;
     //VECTOR A OPERACIONES
    void (*op[32])(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag)={SYS, JMP, JP, JN, JZ, JC, JV, JNP,JNN, JNZ, NOT, INVALID, INVALID, INVALID, INVALID, STOP, MOV, ADD, SUB, MUL, DIV, CMP, AND, OR, XOR, SWAP, SHL, SHR, SAR, LDL, LDH, RND};
    if(argc<2 || argc>3){
        printf("CANTIDAD DE ARGUMENTOS INVALIDA\n");
        return 1;
    }

    FILE * arch = fopen(argv[1], "rb");

    if (!verifyFile(arch)){
        printf("ARCHIVO INVALIDO");
    }
    else{
        int32_t ultinst;
        if (argv[2]){
            flagD = argv[2];
        }
        else{
            flagD="a";
        }
        
        //Se cargan las variables del archivo y se setean los arrays.
        loadCodeSize(arch, &cSize);
        loadCode(arch, cSize, mainMemory);
        setRegisters(registers);
        setSegments(listSegments, cSize);
        flagaux = (strcmp(flagD,"-d")==0)? 1:0;
        
        while (registers[IP] != -1){
            ultinst=registers[IP];
            readNextInst(mainMemory, registers, listSegments);
            if (registers[OPC]<=31 && registers[OPC]>=0){
                if (flagaux){
                    printf("[%04x]   %08x    |   %-8s",(int16_t)getDir(ultinst,listSegments), mainMemory[getDir(ultinst,listSegments)], opndNames[registers[OPC]]);
                }
                op[registers[OPC]](mainMemory, registers, listSegments, flagaux);
                printf("\n");
            }
            else{
                printf("ERROR OPERACION INVALIDA");
                registers[IP]=-1;
            }
        }
        printf("Fin de proceso.");
        fclose(arch);
    }
    return 0;
}


//Funcion que verifica si el archivo es valido para su lectura.
int verifyFile(FILE *file) {
    char id[6]={0};
    int8_t version;

    if (file == NULL){
        printf("ARCHIVO INEXISTENTE.");
        return 0;
    }
    fread(id, sizeof(char), 5, file);
    if (strcmp(id,"VMX26")!=0){
        printf("%s",id);
        fclose(file);
        return 0;
    }
    else{
        fread(&version, sizeof(int8_t), 1, file);
        if (version != 1){
            printf("%d",version);
            fclose(file);
            return 0;
        }else
            return 1;
    }
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
    listSegments[1]=(cSize<<16);
    listSegments[1]+= (MAXMEMORY - cSize);     //y sumo los bytes menos significativos 

    while (i < 8){
        listSegments[i]=0xFFFFFFFF;
        i++;
    }
}


//Funcion para inicializar los registros más importantes.
void setRegisters(int32_t registers[32]){
    registers[CS] =0;
    registers[DS] =1;
    registers[DS] = (registers[DS]<<16);
    registers[IP] = registers[CS];
    registers[AC] = 0;
}


//Funcion para obtener la ultima posicion de memoria de un segmento determinado por un registro.
int getSegmentSize(int32_t reg, int32_t *listSegments){
    int baseCS = (listSegments[(reg >>16 & L2Bt)] >> 16) & L2Bt;
    int tamCS = listSegments[(reg >>16 & L2Bt)] & L2Bt;
    return baseCS+tamCS;
}


//Funcion para obtener la direccion fisica a partir de la logica.
int32_t getDir(int32_t logicDir, int32_t listSegments[]){
    int lowByte=logicDir & L2Bt;
    int highByte=(logicDir>>16) & L2Bt;
    if(highByte>=NUM_SEGMENTOS || listSegments[highByte]==-1 ){
        printf("ERROR: INDICE DE SEGMENTO INVALIDO");
        return -1;
        //error: segmento invalido
    }
    else{
        int base=(listSegments[highByte]>>16) & L2Bt;
        return (lowByte+base);
    }
}


//Funcion que se encarga de obtener los datos de los operandos dentro del codeSegment.
int32_t getOp(int tipo, int8_t *mainMemory,int rindex){
    int32_t opnd=0;
    for(int i=0;i<tipo;i++){
        opnd = opnd<<8 | (uint8_t)mainMemory[rindex + i];
    }
    return opnd;
}


//Funcion que obtiene el valor de un operando y lo devuelve
int32_t getValorOpnd(int32_t operando, int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int8_t tipo = (operando >> 24) & 0x000000FF;
    int16_t codReg;
    int32_t valor=0;
    int16_t offset;
    char *registersNames[32] = {"IP","OPC","OP1","OP2","LAR","MAR","MBR"," "," "," ","EAX","EBX","ECX","EDX","EEX","EFX","AC","CC"," "," "," "," "," "," "," "," ","CS","DS"," "," "," "," "};
    
    switch (tipo) {
        case 1:{ //operando de registro
            codReg = operando & 0b11111; //obtengo el codigo
            valor = registers[codReg];
            if (flag){
                printf("[%s]",registersNames[codReg]);
            }
            
            break;
        }
        case 2:{ //operando inmediato
            valor = operando & 0xFFFF;
            if (flag){
                printf("%-8d", valor);
            }
            break;
        }
        case 3: { //operando de memoria;
            offset = (operando >> 8) & L2Bt;
            codReg = operando & 0b11111;
            if (flag){
                if (offset == 0){
                    printf("[%s]",registersNames[codReg]);
                    }
                else{
                    if (offset>0){
                        printf("[%s+%d]",registersNames[codReg],offset);
                    }
                    else{
                        printf("[%s%d]",registersNames[codReg],offset);
                    }
                }
            }
        
            registers[LAR] = registers[codReg] + offset;
            registers[MAR] = (4 << 16); 
            readFromMemory(registers, mainMemory,listSegments);
            valor = (registers[MBR]);
        }
    }
    return valor;
}


//Funcion que guarda en el operando 1 un valor proveniente de una operacion.
void writeInOp1(int32_t valor, int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int32_t tipo = (registers[OP1] >> 24) & 0X0000000F;
    int8_t codReg;
    int16_t offset;
    switch (tipo) {
        case 1:{ //Operando de registro
            codReg = registers[OP1] & 0b11111;
            registers[codReg] = valor;
            break;
        }
        case 3:{ //operando de memoria
            offset = (int16_t) ((registers[OP1] >>8) & L2Bt);
            codReg = registers[OP1] & 0b11111;
            registers[LAR]=registers[codReg] + offset;
            registers[MAR] = (4 << 16); 
            registers[MBR] = valor;
            loadInMemory(registers,mainMemory,listSegments);
        }
    }

}


//Funcion para leer la siguiente instruccion
void readNextInst(int8_t *mainMemory, int32_t *registers, int32_t listSegments[]){
    if (registers[IP]!=-1){
        int dir = getDir(registers[IP],listSegments);
        if ((dir != -1) && dir < getSegmentSize(registers[CS], listSegments)){
            int8_t operacion=mainMemory[dir]; //Traigo la operacion del IP
            registers[OPC]= operacion & 0b11111; //Obtengo codigo de operacion
            int32_t tipoa= (operacion >> 4) & 0b11; //tipo operando A
            int32_t tipob= (operacion >> 6) & 0b11;//tipo operando B
            registers[OP1]=(tipoa<<24);
            registers[OP2]=(tipob<<24);
            registers[OP2]+=getOp(tipob,mainMemory,getDir(registers[IP]+1, listSegments));
            registers[OP1]+=getOp(tipoa,mainMemory,getDir(registers[IP]+1+tipob, listSegments));
            registers[IP]+=1+tipoa+tipob;//Sumo el tamaño de la operacion
        }
        else{
            printf("Error de segmento al leer siguiente instruccion\n");
            STOP(mainMemory, registers, listSegments,0);
            return;
        }
    }
    else{
        return;
    }
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
    uint32_t i = 0,n=(uint32_t) numero;
    strcpy(auxS,"");

    if (n == 0) {
        auxS[i++] = '0';
    } else {
        while (n != 0) {
            auxS[i++] = (n % 2) + '0';
            n >>= 1;
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
void loadInMemory(int32_t *registers, int8_t *mainMemory,int32_t listSegments[]){
    int i;
    registers[MAR]=registers[MAR]&H2Bt;
    int32_t fDir = getDir(registers[LAR],listSegments);
    if(fDir != -1){
        if ((fDir + ((registers[MAR]>>16)&L2Bt)) <= getSegmentSize(registers[LAR], listSegments)){
            registers[MAR]+=fDir;
            int opSize = (registers[MAR] >>16) & L2Bt;
            int dir = registers[MAR] & L2Bt;
            uint32_t aux = registers[MBR];
            for (i=opSize-1;i>=0;i--){
                mainMemory[dir+i]=(int8_t)(aux & 0xFF);
                aux = aux>>8;
            }
        }
        else{
            printf("Error de segmento al cargar dato en memoria\n");
            STOP(mainMemory, registers, listSegments,0);
            return;
        }
    }
    else{
        printf("Error de segmento al obtener direccion fisica del registro LAR\n");
        return;
    }
}


//Funcion que lee una variable de la memoria y la carga en el MBR dependiendo del MAR. 
void readFromMemory(int32_t *registers, int8_t *mainMemory,int32_t listSegments[]){
    int i;
    int32_t fDir = getDir(registers[LAR],listSegments);
    registers[MAR]=registers[MAR]&H2Bt;
    if (fDir !=-1){
        if ((fDir + ((registers[MAR]>>16)&L2Bt)) <= getSegmentSize(registers[LAR], listSegments)){
            registers[MAR]+= fDir;
            int opSize = (registers[MAR] >>16) &L2Bt;
            int dir = registers[MAR] & L2Bt;
            registers[MBR]=0;

            for (i=0;i<opSize;i++){
                registers[MBR] = registers[MBR]<<8;
                registers[MBR] += mainMemory[i+dir];
            }
        }
        else{
            printf("Error de segmento al leer dato de la memoria\n");
            STOP(mainMemory, registers, listSegments,0);
            return;
        }
    }
    else{
        printf("Error de segmento al obtener direccion fisica del registro LAR\n");
        return;
    }
}


//Funcion SYS
void SYS (int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int op = getValorOpnd(registers[OP2],mainMemory,registers,listSegments,flag);
    int i;
    char auxS[33];
    int32_t aux=registers[ECX]&L2Bt, tam=(registers[ECX]>>16)&L2Bt;
    registers[LAR]=registers[EDX];
    registers[MAR]=(registers[ECX]&H2Bt);

    if(op==1){
        switch (registers[EAX])
        {
        case 1:
            for (i=0;i<aux;i++){
                printf("\n");
                printf("[%04x]:",getDir(registers[LAR],listSegments));
                scanf(" %d",&registers[MBR]);
                loadInMemory(registers,mainMemory,listSegments);
                registers[LAR]+=tam;
            }    
        break;
        case 2:
            for (i=0;i<aux;i++){
                    printf("\n");
                    char c;
                    printf("[%04x]:",getDir(registers[LAR],listSegments));
                    scanf(" %c",&c);

                    registers[MBR]=(int32_t)(unsigned char) c;
                    loadInMemory(registers,mainMemory,listSegments);
                    registers[LAR]+=tam;
            }
        break;
        case 4:
            for (i=0;i<aux;i++){
                printf("\n");
                printf("[%04x]:",getDir(registers[LAR],listSegments));
                scanf(" %o",&registers[MBR]);
                loadInMemory(registers,mainMemory,listSegments);
                registers[LAR]+=tam;
            }
        break;
        case 8:
            for (i=0;i<aux;i++){
                printf("\n");
                printf("[%04x]:",getDir(registers[LAR],listSegments));
                scanf(" %x",&registers[MBR]);
                loadInMemory(registers,mainMemory,listSegments);
                registers[LAR]+=tam;
            }
        break;
        case 16:
            for (i=0;i<aux;i++){
                printf("\n");
                printf("[%04x]:",getDir(registers[LAR],listSegments));
                scanf(" %s",auxS);
                registers[MBR]=(int32_t)stringToInt(auxS);
                loadInMemory(registers,mainMemory,listSegments);
                registers[LAR]+=tam;
            }
        break;
        }
    }
    else{
        for (i=0;i<aux;i++){
            readFromMemory(registers,mainMemory,listSegments); // lee UNA vez, deja el valor en MBR
            printf("\n");
            printf("[%04x]: ",getDir(registers[LAR],listSegments));
            if (registers[EAX] & 0x01){ // bit 0: decimal
                printf(" %d",registers[MBR]);
            }
            if (registers[EAX] & 0x02){ // bit 1: caracteres
                printf(" %c",registers[MBR]);
            }
            if (registers[EAX] & 0x04){ // bit 2: octal
                printf(" 0o%o",registers[MBR]);
            }
            if (registers[EAX] & 0x08){ // bit 3: hexadecimal
                printf("0x %x",registers[MBR]);
            }
            if (registers[EAX] & 0x10){ // bit 4: binario
                intToString(auxS,registers[MBR]);
                printf("0b %s",auxS);
            }
            
        
            registers[LAR]+=tam; // avanza UNA sola vez por celda, no por formato
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
            C = (suma > 0xFFFFFFFF) ? 1 : 0;
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


//Funcion JMP
void JMP (int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers, listSegments, flag);

    if ((getSegmentSize(registers[IP],listSegments) > valor) && (listSegments[(registers[IP]>>16) & L2Bt]>>16 & L2Bt) < valor){
        registers[IP] =valor;
    }
    else{
        printf("Salto fuera de segmento");
        STOP(mainMemory, registers, listSegments, flag);
    }
}


//Funcion LOAD DATA LOW
void LDL (int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag) {
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers, listSegments, flag);
    if (flag){
        printf(", \t");
    }
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
    valor = valor & 0xFFFF;
    a =(a& 0xFFFF0000) | valor;
    writeInOp1(a, mainMemory, registers, listSegments);
}


//Funcion LOAD DATA HIGH
void LDH (int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag) {
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers, listSegments, flag);
    if (flag){
        printf(", \t");
    }
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
    a = (a&0x0000FFFF) + (valor<<16);
    writeInOp1(a, mainMemory, registers, listSegments);
}


//Funcion MOV
void MOV(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t a=getValorOpnd(registers[OP1],mainMemory,registers,listSegments,flag);
    if (flag){
        printf(", \t");
    }
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);

    writeInOp1(valor, mainMemory, registers,listSegments);
    setCC(OP_MOV,0,0,valor,registers);
}


//FUNCIONES JUMP CONDICIONALES --------------------- !!
void JN(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    if (((registers[CC] & 0x80000000)>>31)){
        JMP(mainMemory,registers,listSegments, flag);
}
}


void JP(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    if (((registers[CC] & 0xC0000000)>>30) == 0){
        JMP(mainMemory,registers,listSegments, flag);
    }
}


void JZ(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    if (((registers[CC] & 0x40000000)>>30)){
        JMP(mainMemory,registers,listSegments, flag);
    }
}


void JC(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    if (((registers[CC] & 0x20000000)>>29)){
        JMP(mainMemory,registers,listSegments, flag);
    }
}


void JV(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    if (((registers[CC] & 0x10000000)>>28)){
        JMP(mainMemory,registers,listSegments, flag);
    }
}


void JNP(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    if ((((registers[CC] & 0x80000000)>>31)) || (((registers[CC] & 0x40000000)>>30))){
        JMP(mainMemory,registers,listSegments, flag);
    }
}


void JNN(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    if (!(registers[CC] & 0x80000000)>>31){
        JMP(mainMemory,registers,listSegments, flag);
    }
}


void JNZ(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    if (!(registers[CC] & 0x40000000)>>30){
        JMP(mainMemory,registers,listSegments, flag);
    }
}

// FIN DE LAS OPERACIONES JMP ------------------------ !!


//Funcion NOT
void NOT(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers, listSegments, flag);
    valor = ~valor;
    writeInOp1(valor, mainMemory, registers, listSegments);
    setCC(OP_NOT,0,0,valor,registers);
}


//Funcion ADD
void ADD(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    if (flag){
        printf(", \t");
    }
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);

    valor = a + b; //*a = *a +b

    writeInOp1(valor, mainMemory, registers,listSegments);

    setCC(OP_ADD,a,b,valor,registers);
}


//Funcion SUB
void SUB(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    if (flag){
        printf(", \t");
    }
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);

    valor = a - b;

    writeInOp1(valor, mainMemory, registers,listSegments);

    setCC(OP_SUB,a,b,valor,registers);
}


//Funcion MUL
void MUL(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    if (flag){
        printf(", \t");
    }
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);

    valor = a * b;

    writeInOp1(valor, mainMemory, registers,listSegments);

    setCC(OP_MUL,a,b,valor,registers);
}


//Funcion DIV
void DIV(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    if (flag){
        printf(", \t");
    }
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
        if(b==0){
        printf("ERROR: DIVISION POR 0 NO EVALUADA");
        STOP(mainMemory, registers, listSegments, flag);
    }
    else{

        valor = a / b;

        registers[AC]=a%b; //guardo resto de division entera en AC

        writeInOp1(valor, mainMemory, registers,listSegments);
        

        setCC(OP_DIV,a,b,valor,registers);
    }
}


//Funcion XOR
void XOR(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    if (flag){
        printf(", \t");
    }
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);

    valor = a ^ b;

    writeInOp1(valor, mainMemory, registers,listSegments);

    setCC(OP_XOR,a,b,valor,registers);
}


//Funcion SWAP
void SWAP(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag){
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    if (flag){
        printf(", \t");
    }
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);


    int32_t aux, tipo;
    int8_t codReg;
    int16_t offset;


    aux = a;
    a = b;
    b = aux;
    writeInOp1(a, mainMemory, registers, listSegments);

    tipo = (registers[OP2] >> 24) & 0xF;

    switch (tipo) {
        case 1: //Operando de registro

            codReg = registers[OP2] & 0b11111;

            registers[codReg] = b;
            break;
        case 3: //operando de memoria
            offset = (registers[OP2] >>8) & L2Bt;
            codReg = registers[OP2] & 0b11111;

            registers[LAR]=registers[codReg] + offset;
            registers[MAR] = (4 << 16); 
            registers[MBR] = b;
            loadInMemory(registers,mainMemory,listSegments);
    }
}


//---Shifters---


//Funcion SHL
void SHL(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    uint32_t valor;
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    if (flag){
        printf(", \t");
    }
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
    valor=a<<b;
    writeInOp1(valor, mainMemory, registers,listSegments);
    setCC(OP_SHL,a,b,valor,registers);

}


//Funcion SHR
void SHR(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    uint32_t valor; //al ser unsigned, el shift agrega 0 por izquierda
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    if (flag){
        printf(", \t");
    }
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
    valor=(uint32_t)a>>b;
    writeInOp1(valor, mainMemory, registers,listSegments);
    setCC(OP_SHR,a,b,valor,registers);

}


//Funcion SAR
void SAR(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    if (flag){
        printf(", \t");
    }
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
    valor=a>>b; //como aca valor es signed, propaga el signo normalmente
    writeInOp1(valor, mainMemory, registers,listSegments);
    setCC(OP_SAR,a,b,valor,registers);

}

//Fin Shifters


//Funcion RND
void RND(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor;
    if (flag){
        printf(", \t");
    }
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
    
    valor=rand() % (b+1);
    writeInOp1(valor, mainMemory, registers,listSegments);

}


//Funcion CMP
void CMP(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    if (flag){
        printf(", \t");
    }
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
    valor=a-b;
    setCC(OP_CMP,a,b,valor,registers);
}


//Funcion AND
void AND(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    if (flag){
        printf(", \t");
    }
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
    valor = a & b;

    writeInOp1(valor, mainMemory, registers,listSegments);

    setCC(OP_AND,a,b,valor,registers);

}


//Funcion OR
void OR(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    if (flag){
        printf(", \t");
    }
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);

    valor = a | b;

    writeInOp1(valor, mainMemory, registers,listSegments);

    setCC(OP_OR,a,b,valor,registers);

}


//Funcion STOP
void STOP(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    registers[IP] = -1;
}


//Funcion invalida, tira error.
void INVALID(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    printf("ERROR CODIGO DE OPERACION INVALIDA");
    STOP(mainMemory, registers, listSegments, flag);
}