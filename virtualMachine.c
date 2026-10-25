#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stddef.h>


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
typedef struct {
    char *vmxPath;      // NULL si no se pasó
    char *vmiPath;      // NULL si no se pasó
    int   memKiB;       // 16 por defecto
    int   disassemble;  // 1 si vino -d
    int   paramCount;   // cantidad de params después de -p
    char **params;      // apunta dentro de argv (no hace falta copiar)
} VMArgs;

//Instrucción 
#define IP   0
#define OPC  1
#define OP1  2
#define OP2  3

// Acceso a memoria
#define LAR  4
#define MAR  5
#define MBR  6

//Registros Stack
#define SP 7
#define BP 8


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
#define ES   28
#define SS   29
#define KS   30
#define PS   31

#define NUM_SEGMENTOS 8

#define H2Bt 0xFFFF0000//Constantes para tomar los 2 bytes más significativos y los 2 menos significativos.
#define L2Bt 0x0000FFFF
#define H4Bt 0xFFFFFFFF00000000
#define L4Bt 0x00000000FFFFFFFF

int vmError = 0; //Se pone en 1 si la ejecucion termino por un error

void runtimeError(int32_t *registers, const char *msg);

int verifyFile(FILE *file);

void loadBytes(FILE *file, uint16_t cSize, int8_t *mainMemory);


int getSegmentEnd(int32_t reg, int32_t *listSegments);

int32_t getDir(int32_t logicDir, int32_t listSegments[]);

int32_t getOp(int tipo, int8_t *mainMemory, int rindex);

int32_t getValorOpnd(int32_t operando, int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void writeInOp(int32_t operando, int32_t valor, int8_t *mainMemory, int32_t *registers,int32_t listSegments[]);

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

void PUSH(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void POP (int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void CALL(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void RET (int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag);

void INVALID(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag);

void DISASSEMBLER(int8_t *mainMemory, int32_t *registers, int32_t listSegments[]);

int parseArgs(int argc, char *argv[], VMArgs *args);

int endsWith(const char *s, const char *suf);

int readHeader(FILE *file, int version, uint16_t *segSize, uint16_t *entrypoint,int32_t memsize);

int buildSegments(int32_t *listSegments,int32_t *registers,int *segReg,uint16_t *segSize,int32_t memsize);

int pushValue(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int32_t valor);

int popValue(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int32_t *valor);

int getSegmentBase(int32_t reg, int32_t *listSegments);

int getSegmentSize(int32_t reg, int32_t *listSegments);



int main(int argc,char* argv[]){ //gcc virtualMachine.c -o vmx.exe -Wall -Wextra
    VMArgs args;
    if(!parseArgs(argc,argv,&args)){
        printf("ARGUMENTOS INVALIDOS\n");
        return 1;
    }
    srand(time(NULL)); //inicializa semilla al iniciar programa
    int32_t registers[32]={0}; //serian los 32 registros de 4 bytes que se piden
    int32_t listSegments[8]={0}; //serian los 8 segmentos de 4 bytes que se piden
    int32_t memsize=args.memKiB*1024; //Traduzco el parametro m de KiB a bytes
    int8_t *mainMemory = calloc(memsize, 1);//seria la memoria principal de tamaño variable
    int segReg[6] = {PS, KS, CS, DS, ES, SS};
    uint16_t segSize[6] = {0};
    uint16_t entryPoint = 0;
    if(mainMemory==NULL){
        printf("ERROR AL INICIALIZAR MEMORIA\n");
        return 1;
    }
    void (*op[32])(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag)={SYS, JMP, JP, JN, JZ, JC, JV, JNP,JNN, JNZ, NOT, PUSH, POP, CALL, RET, STOP, MOV, ADD, SUB, MUL, DIV, CMP, AND, OR, XOR, SWAP, SHL, SHR, SAR, LDL, LDH, RND};
    
    if(args.vmxPath == NULL){
        printf("CARGA DESDE .VMI AUN NO IMPLEMENTADA");
        free(mainMemory);
        return 1;
    }

    FILE * arch = fopen(args.vmxPath, "rb");
    int version = verifyFile(arch);


    if (version){
        
        int32_t Lultinst,Fultinst;
        int8_t ultinst;
        
        //Se cargan las variables del archivo y se setean los arrays.
        if(!readHeader(arch,version,segSize,&entryPoint,memsize)){
            fclose(arch);
            free(mainMemory);
            return 1;

        }
        if(!buildSegments(listSegments,registers,segReg,segSize,memsize)){
            fclose(arch);
            free(mainMemory);
            return 1;
        }
        int baseCS = getSegmentBase(registers[CS], listSegments);
        loadBytes(arch, segSize[2], mainMemory + baseCS);

        if (registers[KS] != -1) {
            int baseKS = getSegmentBase(registers[KS],listSegments);
            loadBytes(arch, segSize[1], mainMemory + baseKS);
        }
        // Inicialización de la pila: SP al final del SS; se apilan argv, argc y RET
         if (registers[SS] != -1) {
            registers[SP] = registers[SS] + segSize[5];
            if (!pushValue(mainMemory, registers, listSegments, -1) ||   // argv -1 es un placeholder hasta implemetar -p
                !pushValue(mainMemory, registers, listSegments, 0)  ||   // argc 0 es un placeholder hasta implementar -p
                !pushValue(mainMemory, registers, listSegments, -1)) {   // RET
                fclose(arch);
                free(mainMemory);
                return 1;
            }
        }
        


   

        if (args.disassemble){
            registers[IP] = registers[CS];
            DISASSEMBLER(mainMemory, registers, listSegments);
        }
        registers[IP]=registers[CS] | entryPoint;
        
        
        while (registers[IP] != -1){
            Lultinst=registers[IP];
            readNextInst(mainMemory, registers, listSegments);

            if (registers[IP] == -1){
                break;
            }
            
            if (registers[OPC]<=31 && registers[OPC]>=0){
                op[registers[OPC]](mainMemory, registers, listSegments, args.disassemble);
            }
            else{
                runtimeError(registers, "ERROR OPERACION INVALIDA");
            }
        }
        fclose(arch);
        if (vmError){
            free(mainMemory);
            return 1;
        }
        printf("Fin de proceso.\n");
    }
    else{
        free(mainMemory);
        return 1;
    }
    free(mainMemory);//Libera la memoria principal al finalizar el ciclo
    return 0;
}


//Informa un error de ejecucion, detiene la maquina virtual y marca que termino con error
void runtimeError(int32_t *registers, const char *msg){
    if (vmError) //Solo se informa el primer error
        return;
    printf("%s\n", msg);
    registers[IP] = -1;
    vmError = 1;
}


//Funcion auxiliar para detectar extension de archivo
int endsWith(const char *s, const char *suf) {
    size_t ls = strlen(s), lsuf = strlen(suf);
    return ls >= lsuf && _stricmp(s + ls - lsuf, suf) == 0; //_stricmp no distingue mayusculas (solo Windows)
}


//Funcion para cargar los parametros en un struct
int parseArgs(int argc, char *argv[], VMArgs *args){
    args->vmxPath = NULL;
    args->vmiPath = NULL;
    args->memKiB = 16;
    args->disassemble = 0;
    args->paramCount = 0;
    args->params = NULL;
    for (int i = 1; i < argc; i++) {
        char *a = argv[i];

        if (strcmp(a, "-p") == 0) {
            args->params = &argv[i + 1];
            args->paramCount = argc - i - 1;
            break;
        }
        else if (strcmp(a, "-d") == 0)
            args->disassemble = 1;
        else if (strncmp(a, "m=", 2) == 0) {
            char *fin;
            long m = strtol(a + 2, &fin, 10);
            if (fin == a + 2 || *fin != '\0' || m <= 0 || m > 64)
                return 0;
            args->memKiB = (int)m;
        }
        else if (endsWith(a, ".vmx")) {
            if (args->vmxPath != NULL) return 0;
            args->vmxPath = a;
        }
        else if (endsWith(a, ".vmi")) {
            if (args->vmiPath != NULL) return 0;
            args->vmiPath = a;
        }
        else
            return 0;
            
    }
    if (args->vmxPath == NULL && args->vmiPath == NULL)
        return 0;
    if (args->vmxPath == NULL) {
        args->paramCount = 0;
        args->params = NULL;
    }
    return 1;
}


//----------------------------DISASSEMBLER-------------------------------
void DISASSEMBLER(int8_t *mainMemory, int32_t *registers, int32_t listSegments[]) {
    int32_t Lultinst, ultinst, Fultinst;

    char *opndNames[32] = {
        "SYS","JMP","JP","JN","JZ","JC","JV","JNP",
        "JNN","JNZ","NOT","PUSH","POP","CALL","RET","STOP",
        "MOV","ADD","SUB","MUL","DIV","CMP","AND","OR",
        "XOR","SWAP","SHL","SHR","SAR","LDL","LDH","RND"
    };

    char *registersNames[32] = {
        "IP","OPC","OP1","OP2","LAR","MAR","MBR","SP","BP"," ",
        "EAX","EBX","ECX","EDX","EEX","EFX","AC","CC"," "," ",
        " "," "," "," "," "," ","CS","DS","ES","SS","KS","PS"
    };

    int tamCS = getSegmentSize(registers[CS],listSegments);

    while ((registers[IP] & 0xFFFF) < tamCS) {
        Lultinst = registers[IP];
        readNextInst(mainMemory, registers, listSegments);

        if (registers[OPC] >= 0 && registers[OPC] <= 31) {
            Fultinst = getDir(Lultinst, listSegments);
            ultinst = mainMemory[Fultinst];

            int32_t tipoa = (ultinst >> 4) & 0b11;
            int32_t tipob = (ultinst >> 6) & 0b11;

            char bytesStr[64] = "";
            char opA[64] = "";
            char opB[64] = "";

            int8_t tipoA = (registers[OP1] >> 24) & 0xFF;
            int8_t tipoB = (registers[OP2] >> 24) & 0xFF;

            int16_t codReg;
            int32_t valor;
            int16_t offset;

            // ---- Armar bytes ----
            for (int i = 0; i < 1 + tipoa + tipob; i++) {
                char temp[8];
                sprintf(temp, "%02X ", (uint8_t)mainMemory[Fultinst + i]);
                strcat(bytesStr, temp);
            }

            // ---- Operando A ----
            switch (tipoA) {
                case 1:
                    codReg = registers[OP1] & 0b11111;
                    sprintf(opA, "%s", registersNames[codReg]);
                    break;

                case 2:
                    valor = (int16_t)(registers[OP1] & 0xFFFF);
                    sprintf(opA, "%d", valor);
                    break;

                case 3:
                    offset = (int16_t)((registers[OP1] >> 8) & 0xFFFF);
                    codReg = registers[OP1] & 0b11111;
                    if (offset == 0) {
                        sprintf(opA, "[%s]", registersNames[codReg]);
                    } else if (offset > 0) {
                        sprintf(opA, "[%s+%d]", registersNames[codReg], offset);
                    } else {
                        sprintf(opA, "[%s%d]", registersNames[codReg], offset);
                    }
                    break;

                default:
                    strcpy(opA, "");
                    break;
            }

            // ---- Operando B ----
            switch (tipoB) {
                case 1:
                    codReg = registers[OP2] & 0b11111;
                    sprintf(opB, "%s", registersNames[codReg]);
                    break;

                case 2:
                    valor = (int16_t)(registers[OP2] & 0xFFFF);
                    sprintf(opB, "%d", valor);
                    break;

                case 3:
                    offset = (int16_t)((registers[OP2] >> 8) & 0xFFFF);
                    codReg = registers[OP2] & 0b11111;
                    if (offset == 0) {
                        sprintf(opB, "[%s]", registersNames[codReg]);
                    } else if (offset > 0) {
                        sprintf(opB, "[%s+%d]", registersNames[codReg], offset);
                    } else {
                        sprintf(opB, "[%s%d]", registersNames[codReg], offset);
                    }
                    break;

                default:
                    strcpy(opB, "");
                    break;
            }

            // ---- Impresión final ----
            if (tipoA == 0 && tipoB == 0) {
                printf("[%04X]  %-18s   | %-4s\n",
                    (uint16_t)Fultinst,
                    bytesStr,
                    opndNames[registers[OPC]]);
            }else if (tipoA == 0 && tipoB != 0) {
                printf("[%04X]  %-18s   | %-4s %s\n",
                    (uint16_t)Fultinst,
                    bytesStr,
                    opndNames[registers[OPC]],
                    opB);
            } else {
                printf("[%04X]  %-18s   | %-4s %s, %s\n",
                    (uint16_t)Fultinst,
                    bytesStr,
                    opndNames[registers[OPC]],
                    opA,
                    opB);
            }
        }
    }
}
//-----------------------------------------------------------------------



//Funcion que verifica si el archivo es valido para su lectura.
int verifyFile(FILE *file) {
    char id[6]={0};
    int8_t version;

    if (file == NULL){
        printf("ARCHIVO INEXISTENTE\n");
        return 0;
    }
    fread(id, sizeof(char), 5, file);
    if (strcmp(id,"VMX26")!=0){
        printf("ARCHIVO INVALIDO: Identificador incorrecto: %s\n",id);
        fclose(file);
        return 0;
    }
    else{
        fread(&version, sizeof(int8_t), 1, file);
        if (version != 1 && version!=2){
            printf("VERSION INVALIDA: %d\n",version);
            fclose(file);
            return 0;
        }else
            return version; //Ahora devuelve numero de version
    }
}


//Lee cabecera dependiendo version
int readHeader(FILE *file, int version, uint16_t *segSize, uint16_t *entrypoint,int32_t memsize){
    uint8_t b[2];
    int pos[5] = {2, 3, 4, 5, 1};
    for(int i=0;i<6;i++)
        segSize[i]=0;
    *entrypoint=0;
    if(version==1){

        if (fread(b, 1, 2, file) != 2) {
            printf("Cabecera invalida");
            return 0;
        }
         segSize[2]=(uint16_t)((b[0] << 8) | b[1]); //CS
        if(segSize[2]>memsize){
            printf("Codigo mas grande que memoria\n");
            return 0;
        }
         segSize[3]=(uint16_t) (memsize-segSize[2]); //DS = resto de la memoria


    }
    else{
        for(int i=0;i<6;i++){
            if (fread(b, 1, 2, file) != 2) {
                printf("Cabecera invalida");
                return 0;
            }
            uint16_t val = (b[0] << 8) | b[1];
            if(i<5)
                segSize[pos[i]]=val; //inicializa tamaño de los segmentos
            else
                *entrypoint=val; //Offset del entrypoint
            
            
        }

    }
    return 1;


    
}


//Configura tabla de segmentos
int buildSegments(int32_t *listSegments,int32_t *registers,int *segReg,uint16_t *segSize,int32_t memsize){
    int base=0,iseg=0;
    for(int i=0;i<6;i++){
        if(segSize[i]>0){
            int tamanio=segSize[i];
            listSegments[iseg] =(base<<16)|tamanio;
            registers[segReg[i]]=iseg<<16;
            base+=tamanio;
            iseg++;
        }
        else{
            registers[segReg[i]]=-1;
        }
    }
    for (; iseg < 8; iseg++)
        listSegments[iseg] = -1;
    if(base>memsize){
        printf("MEMORIA INSUFICIENTE\n");
        return 0;
    }
    return 1;
}


void setParamSegment(int32_t *listSegments, int argc, char *argv[], int8_t *mainMemory, int32_t *registers) {
    if (argc <= 0 || registers[PS] == -1) {
        return;
    }
    
    int offset = 0;
    int stringOffsets[argc];
    
    // 1. Guardar todas las cadenas (nombre archivo + parámetros)
    for (int i = 0; i < argc; i++) {
        stringOffsets[i] = offset;
        int len = strlen(argv[i]) + 1;
        for (int j = 0; j < len; j++) {
            mainMemory[offset + j] = argv[i][j];
        }
        offset += len;
    }
    
    // 2. Guardar arreglo argv con punteros aws cada string
    for (int i = 0; i < argc; i++) {
        int32_t puntero = (0x0000 << 16) | (stringOffsets[i] & 0xFFFF);
        
        mainMemory[offset + 0] = (puntero >> 24) & 0xFF;
        mainMemory[offset + 1] = (puntero >> 16) & 0xFF;
        mainMemory[offset + 2] = (puntero >> 8) & 0xFF;
        mainMemory[offset + 3] = puntero & 0xFF;
        
        offset += 4;
    }
}



//Extrae un valor de la pila, devuelve 1 si salio bien y 0 si hubo underflow, guarda en valor el elemento popeado
int popValue(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int32_t *valor){
    if(registers[SS] == -1){
        runtimeError(registers, "STACK UNDERFLOW");
        return 0;
    }
    int32_t tamSS=getSegmentSize(registers[SS],listSegments);
    int offsetSP = registers[SP] & 0xFFFF;
    if (offsetSP + 4 > tamSS){
        runtimeError(registers, "STACK UNDERFLOW");
        return 0;
    }
    registers[LAR] = registers[SP];
    registers[MAR] = 4 << 16;      // readFromMemory saca el tamaño de la parte alta de MAR
    readFromMemory(registers, mainMemory, listSegments);
    *valor = registers[MBR];
    registers[SP]+=4; //Se popeo con exito, se mueve el tope
    return 1;
}


//Pushea un valor a la pila, devuelve 1 si salio bien y 0 si hubo overflow, guarda en valor el elemento pusheado
int pushValue(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int32_t valor){
    if(registers[SS] == -1){
        runtimeError(registers, "STACK OVERFLOW");
        return 0;
    }
    registers[SP]-=4;
    if (registers[SP]<registers[SS]){
        runtimeError(registers, "STACK OVERFLOW");
        return 0;
    }
    registers[LAR] = registers[SP];
    registers[MAR] = 4 << 16;      //loadInMemory saca el tamaño de la parte alta de MAR
    registers[MBR]=valor; //Cargo valor en el MBR
    loadInMemory(registers, mainMemory, listSegments);
    return 1;
}


int getSegmentBase(int32_t reg, int32_t *listSegments){ //Funcion que obtiene la base del segmento al que apunta reg
    return (listSegments[(reg >> 16) & 0xFFFF] >> 16) & 0xFFFF;
}

int getSegmentSize(int32_t reg, int32_t *listSegments){ //Funcion que obtiene el tamaño del segmento al que apunta reg
    return listSegments[(reg >> 16) & 0xFFFF] & 0xFFFF;
}





//funcion que carga el codigo en la memoria principal.
void loadBytes(FILE *file, uint16_t cSize, int8_t *mainMemory) {
    int i;
    for (i=0; i<cSize; i++){
        if(fread(&(mainMemory[i]), sizeof(int8_t), 1, file)!=1);
    }
}








//Funcion para obtener la ultima posicion de memoria de un segmento apuntado por un registro.
int getSegmentEnd(int32_t reg, int32_t *listSegments){
    return getSegmentBase(reg, listSegments) + getSegmentSize(reg, listSegments);

}


//Funcion para obtener la direccion fisica a partir de la logica.
int32_t getDir(int32_t logicDir, int32_t listSegments[]){
    int lowByte=logicDir & L2Bt;
    int highByte=(logicDir>>16) & L2Bt;
    if(highByte>=NUM_SEGMENTOS || listSegments[highByte]==-1 ){
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
    if (flag == 1){
        switch (tipo) {
            case 1:{ //operando de registro
                codReg = operando & 0b11111; //obtengo el codigo
                valor = registers[codReg];

                break;
            }
            case 2:{ //operando inmediato
                valor = (int16_t)(operando & 0xFFFF);
                break;
            }
            case 3: { //operando de memoria;
                offset = (int16_t)((operando >> 8) & L2Bt);
                codReg = operando & 0b11111;
                int32_t segmentCode = (registers[codReg] >> 16) & 0xFFFF;
                int32_t baseOffset = (registers[codReg] & 0xFFFF) + offset;
                // Formar la dirección lógica: [segmentCode:baseOffset]
                registers[LAR] = (segmentCode << 16) | (baseOffset & 0xFFFF);
                registers[MAR] = (4 << 16); 
                readFromMemory(registers, mainMemory,listSegments);
                valor = (registers[MBR]);
            }
        }
    }
    else if (flag == 2){
        switch (tipo) {
            case 1:{ //operando de registro
                codReg = operando & 0b11111;
                int opndType = (operando >> 5) & 0b11; 

                if (opndType == 0){
                    valor = registers[codReg];
                }
                else if (opndType == 1){
                    valor = registers[codReg]  & 0xFF;
                }
                else if (opndType == 2){
                    valor = (registers[codReg] >> 8) & 0xFF;
                }
                else{
                    valor = registers[codReg] & L2Bt;
                }

                break;
            }
            case 2:{ //operando inmediato
                valor = (int16_t)(operando & 0xFFFF);
                break;
            }
            case 3: { //operando de memoria;
                offset = (int16_t)((operando >> 8) & L2Bt);
                codReg = operando & 0b11111;
                int32_t segmentCode = (registers[codReg] >> 16) & 0xFFFF;
                int32_t baseOffset = (registers[codReg] & 0xFFFF) + offset;
                // Formar la dirección lógica: [segmentCode:baseOffset]

                registers[LAR] = (segmentCode << 16) | (baseOffset & 0xFFFF);

                int opndType = (operando >> 5) & 0b111; 
                
                if (opndType == 0 || opndType == 4){
                    registers[MAR] = (4 << 16);
                }
                else if (opndType == 1){
                    registers[MAR] = (1 << 16);
                }
                else{
                    registers[MAR] = (2 << 16);
                }

                readFromMemory(registers, mainMemory,listSegments);
                valor = (registers[MBR]);
            }
        }
    }
    
    return valor;
}


//Funcion que guarda en el operando 1 un valor proveniente de una operacion.
void writeInOp(int32_t operando, int32_t valor, int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t tipo = (operando >> 24) & 0X0000000F;
    int8_t codReg;
    int16_t offset;
    if (flag == 1){
        switch (tipo) {
            case 1:{ //Operando de registro
                codReg = operando & 0b11111;
                registers[codReg] = valor;
                break;
            }
            case 3:{ //operando de memoria
                offset = (int16_t)((operando >> 8) & L2Bt);
                codReg = operando & 0b11111;
                int32_t segmentCode = (registers[codReg] >> 16) & 0xFFFF;
                int32_t baseOffset = (registers[codReg] & 0xFFFF) + offset;
                // Formar la dirección lógica: [segmentCode:baseOffset]
                registers[LAR] = (segmentCode << 16) | (baseOffset & 0xFFFF);
                registers[MAR] = (4 << 16); 
                registers[MBR] = valor;
                loadInMemory(registers,mainMemory,listSegments);
            }
        }
    }
    else{
        switch (tipo) {
            case 1:{ //Operando de registro
                codReg = operando & 0b11111;

                int opndType = (operando >> 5) & 0b11; 

                if (opndType == 0){
                    registers[codReg] = valor;
                }
                else if (opndType == 1){
                    registers[codReg] = (registers[codReg] & 0xFFFFFF00) | (valor & 0xFF);
                }
                else if (opndType == 2){
                    registers[codReg] = (registers[codReg] & 0xFFFF00FF) | ((valor & 0xFF) << 8);
                }
                else{
                    registers[codReg] = (registers[codReg] & 0xFFFF0000) | (valor & L2Bt);
                }

                break;
            }
            case 3:{ //operando de memoria
                offset = (int16_t)((operando >> 8) & L2Bt);
                codReg = operando & 0b11111;
                int32_t segmentCode = (registers[codReg] >> 16) & 0xFFFF;
                int32_t baseOffset = (registers[codReg] & 0xFFFF) + offset;
                // Formar la dirección lógica: [segmentCode:baseOffset]
                registers[LAR] = (segmentCode << 16) | (baseOffset & 0xFFFF);
                int opndType = (operando >> 5) & 0b111; 
                
                if (opndType == 0 || opndType == 4){
                    registers[MAR] = (4 << 16);
                    registers[MBR] = valor;
                }
                else if (opndType == 1){
                    registers[MAR] = (1 << 16);
                    registers[MBR] = valor << 24; // Coloca el valor en el byte más alto
                }
                else{
                    registers[MAR] = (2 << 16);
                    registers[MBR] = valor << 16; // Coloca el valor en los dos bytes más altos
                }
                loadInMemory(registers,mainMemory,listSegments);
            }
        }
    }

}


//Funcion para leer la siguiente instruccion
void readNextInst(int8_t *mainMemory, int32_t *registers, int32_t listSegments[]){
    if (registers[IP]!=-1){
        int dir = getDir(registers[IP],listSegments);

        if (dir == -1) {
            runtimeError(registers, "Error de segmento al leer siguiente instruccion");
            return;
        }

        int32_t tamSegmento = getSegmentEnd(registers[CS], listSegments);

        if (dir >= tamSegmento){
            STOP(mainMemory, registers, listSegments, 0);
            return;
        }
        
        int8_t operacion = mainMemory[dir];
        int32_t tipoA = (operacion >> 4) & 0b11;
        int32_t tipoB = (operacion >> 6) & 0b11;
        int tamOp = 1 + tipoA + tipoB;
        
        if ((dir != -1) && (dir + tamOp - 1 < tamSegmento)){
            registers[OPC] = operacion & 0b11111;
            registers[OP1] = (tipoA << 24);
            registers[OP2] = (tipoB << 24);
            
            // Usar 'dir' (dirección física actual) directamente
            // NO 'registers[IP]' que será modificado
            registers[OP2] += getOp(tipoB, mainMemory, dir + 1);
            registers[OP1] += getOp(tipoA, mainMemory, dir + 1 + tipoB);
            
            // Ahora incrementa IP
            registers[IP] += 1 + tipoA + tipoB;
        }
        else{
            runtimeError(registers, "Error de segmento al leer siguiente instruccion");
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
void loadInMemory(int32_t *registers, int8_t *mainMemory, int32_t listSegments[]){
    if (vmError) //La ejecucion ya fue abortada, no se accede a memoria
        return;
    int32_t fDir = getDir(registers[LAR], listSegments);
    
    if(fDir != -1){

        // Extraer información de MAR
        int16_t opSize = (registers[MAR] >> 16) & 0xFFFF;  // Tamaño en bits altos
        registers[MAR] = (registers[MAR] & 0xFFFF0000) | ((uint16_t)fDir);
        // Validar que el acceso esté dentro del segmento
        int limSegmento = getSegmentEnd(registers[LAR], listSegments);
        
        // fDir es la dirección física de inicio
        // Necesitamos verificar que [fDir ... fDir + opSize - 1] esté dentro del segmento
        if ((fDir + opSize) > limSegmento) {
            printf("Intento de acceso: [%d, %d), Límite del segmento: %d\n", fDir, fDir + opSize, limSegmento);
            runtimeError(registers, "Error de segmento al cargar dato en memoria");
            return;
        }
        
        // Escribir en memoria
        uint32_t aux = (uint32_t)registers[MBR];
        for (size_t i = 0; i < opSize; i++) {
            size_t shift = 8 * (opSize - 1 - i);
            mainMemory[fDir + i] = (int8_t)((aux >> shift) & 0xFF);
        }
    }
    else{
        runtimeError(registers, "Error de segmento al obtener direccion fisica del registro LAR");
    }
}


//Funcion que lee una variable de la memoria y la carga en el MBR dependiendo del MAR. 
void readFromMemory(int32_t *registers, int8_t *mainMemory, int32_t listSegments[]){
    if (vmError) //La ejecucion ya fue abortada, no se accede a memoria
        return;
    int32_t fDir = getDir(registers[LAR], listSegments);    
    if (fDir != -1){
        // Extraer información de MAR
        int16_t opSize = (registers[MAR] >> 16) & 0xFFFF;  // Tamaño en bits altos
        registers[MAR] = (registers[MAR] & 0xFFFF0000) | ((uint16_t)fDir);
        // Validar que el acceso esté dentro del segmento
        int limSegmento = getSegmentEnd(registers[LAR], listSegments);
        
        if ((fDir + opSize) > limSegmento) {
            printf("Intento de acceso: [%d, %d), Límite del segmento: %d\n", fDir, fDir + opSize, limSegmento);
            runtimeError(registers, "Error de segmento al leer dato de la memoria");
            return;
        }
        
        // Leer desde memoria
        registers[MBR] = 0;
        uint32_t aux = 0;
        
        for (int i = 0; i < opSize; i++){
            aux = (aux << 8) | ((uint8_t)mainMemory[fDir + i]);
        }
        
        // Convertir a signed si es necesario
        if (opSize == 4){
            registers[MBR] = (int32_t)aux;
        } else {
            uint32_t bits = (uint32_t)(opSize * 8);
            uint32_t m = 1u << (bits - 1);
            registers[MBR] = (int32_t)((aux ^ m) - m);
        }
    }
    else{
        runtimeError(registers, "Error de segmento al obtener direccion fisica del registro LAR");
    }
}


//Funcion SYS
void SYS (int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int op = getValorOpnd(registers[OP2],mainMemory,registers,listSegments,flag);
    int i;
    char auxS[33];
    int32_t aux=registers[ECX]&L2Bt, tam=(registers[ECX]>>16)&L2Bt;
    registers[LAR] = registers[EDX];
    registers[MAR] = (registers[ECX]&H2Bt);

    switch (op){
        case 1:
            switch (registers[EAX]){
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
                        printf("[%04x]:",getDir(registers[LAR],listSegments));
                        scanf(" %o",&registers[MBR]);
                        loadInMemory(registers,mainMemory,listSegments);
                        registers[LAR]+=tam;
                    }
                break;
                case 8:
                    for (i=0;i<aux;i++){
                        printf("[%04x]:",getDir(registers[LAR],listSegments));
                        scanf(" %x",&registers[MBR]);
                        loadInMemory(registers,mainMemory,listSegments);
                        registers[LAR]+=tam;
                    }
                break;
                case 16:
                    for (i=0;i<aux;i++){
                        printf("[%04x]:",getDir(registers[LAR],listSegments));
                        scanf(" %s",auxS);
                        registers[MBR]=(int32_t)stringToInt(auxS);
                        loadInMemory(registers,mainMemory,listSegments);
                        registers[LAR]+=tam;
                    }
                break;
            }
            break;

        case 2:
            for (i=0;i<aux;i++){
                    readFromMemory(registers,mainMemory,listSegments); // lee UNA vez, deja el valor en MBR
                    printf("\n[%04x]: ",getDir(registers[LAR],listSegments));
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
                        printf(" 0x%x",registers[MBR]);
                    }
                    if (registers[EAX] & 0x10){ // bit 4: binario
                        intToString(auxS,registers[MBR]);
                        printf(" 0b%s",auxS);
                    }
                    printf("\n");
                
                    registers[LAR]+=tam; // avanza UNA sola vez por celda, no por formato
                }
            break;
        
        case 3:
            tam = registers[ECX];
            if (tam == -1){
                char *auxS[4096];
            }
            else{
                char *auxS[tam + 1];
            }
            fscanf("%s",auxS);
            int len = strlen(auxS) + 1;
            registers[MAR] = (1 << 16);
            for (i = 0; i < len; i++){
                registers[MBR] = auxS[i];
                loadInMemory(registers,mainMemory,listSegments);
                registers[LAR]+=1;
            }
        break;

        case 4:
            char *auxS[4096];
            int fdir = getDir(registers[LAR],listSegments);
            int i = 0;
            registers[MAR]=(1<<16);
            while (mainMemory[fdir + i] != '\0'){
                readFromMemory(registers,mainMemory,listSegments);
                auxS[i] = (char)registers[MBR];
                i += 1;
            }
            printf("\n[%04x]: ",getDir(registers[LAR],listSegments));
            auxS[i] = '\0';
            printf("%s\n", auxS);
        break;

        case 0:
            #ifdef _WIN32
                system("cls");
            #else
                system("clear");
            #endif
        break;

        case 15:
        break;

        default:
            runtimeError(registers, "SYS: Operacion invalida");
            break;
    }
    registers[LAR] = registers[EDX];
}


// Funcion para setear los valores del registro CC.
void setCC(OpType op, int32_t a, int32_t b, int32_t resultado, int32_t *registers) {
    int N = 0, Z = 0, C = 0, V = 0;

    // N y Z se calculan igual para todas las operaciones
    N = (resultado < 0) ? 1 : 0;
    Z = (resultado == 0) ? 1 : 0;

    // C y V dependen de la operaciÃ³n
    switch (op) {
        case OP_ADD: {
            uint64_t sumaC = (uint64_t)(uint32_t)a + (uint64_t)(uint32_t)b;
            int64_t sumaV = (int64_t)a+(int64_t)b;

            C = (((sumaC >> 32) & L4Bt)!=0);

            V = (sumaV != (int32_t)sumaV);
            break;
        }

        case OP_SUB:
        case OP_CMP: {
            uint64_t restaC = (uint64_t)(uint32_t)a + (uint64_t)(~(uint32_t)b+1);
            int64_t restaV = (int64_t)a - (int64_t)b;

            C = ((restaC>>32)&L4Bt)!=0;

            V = (restaV != (int32_t)restaV);
            break;
        }

        case OP_MUL: {
            uint64_t prodC = (uint64_t)(uint32_t)a *(uint64_t)(uint32_t)b;
            int64_t prodV=(int64_t)a * (int64_t)b;
            C = (((prodC >> 32) & L4Bt)!=0);

            V = ((prodV) != ((int32_t)prodV));
            break;
        }

        case OP_DIV: {
            C=0;
            if(a==INT32_MIN && b==-1){
                V=1;
            }
            else{
                V=0;
            }
            break;
        }

        case OP_SHL: {
            uint64_t shiftC = (uint64_t)(uint32_t)a << (uint64_t)(uint32_t)b;
            int64_t shiftV= (int64_t)a << (int64_t)b;

            C = (((shiftC >> 32) & L4Bt)!=0);
            V = ((shiftV) != ((int32_t)(shiftV)));
            break;
        }

        case OP_SHR:
        case OP_SAR:
        case OP_NOT:
        case OP_MOV:
        case OP_AND:
        case OP_OR:
        case OP_XOR:

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

    if (((listSegments[(registers[CS] >> 16) & L2Bt] & L2Bt) > valor) &&  (valor>=0)){
        registers[IP] =(registers[IP] & H2Bt)+valor;
    }
    else{
        runtimeError(registers, "Salto fuera del segmento de codigo");
    }
}


//Funcion LOAD DATA LOW
void LDL (int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag) {
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers, listSegments, flag);
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
    valor = valor & 0xFFFF;
    a =(a& 0xFFFF0000) | valor;
    writeInOp(registers[OP1],a, mainMemory, registers, listSegments, flag);
}


//Funcion LOAD DATA HIGH
void LDH (int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag) {
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers, listSegments, flag);
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
    a = (a & 0x0000FFFF) | ((valor & 0xFFFF) << 16);
    writeInOp(registers[OP1],a, mainMemory, registers, listSegments, flag);
}


//Funcion MOV
void MOV(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t a=getValorOpnd(registers[OP1],mainMemory,registers,listSegments,flag);
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);

    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments, flag);
    setCC(OP_MOV,0,0,valor,registers);
}


//FUNCIONES JUMP CONDICIONALES --------------------- !!
void JN(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    if (((registers[CC] & 0x80000000)>>31) & L2Bt){
        JMP(mainMemory,registers,listSegments, flag);
}
}


void JP(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    if ((((registers[CC] & 0xC0000000)>>30) & L2Bt) == 0){
        JMP(mainMemory,registers,listSegments, flag);
    }
}


void JZ(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    if (((registers[CC] & 0x40000000)>>30) & L2Bt){
        JMP(mainMemory,registers,listSegments, flag);
    }
}


void JC(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    if (((registers[CC] & 0x20000000)>>29) & L2Bt){
        JMP(mainMemory,registers,listSegments, flag);
    }
}


void JV(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    if (((registers[CC] & 0x10000000)>>28) & L2Bt){
        JMP(mainMemory,registers,listSegments, flag);
    }
}


void JNP(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    if ((((registers[CC] & 0x80000000)>>31) & L2Bt) || (((registers[CC] & 0x40000000)>>30) & L2Bt)){
        JMP(mainMemory,registers,listSegments, flag);
    }
}


void JNN(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    if ((((registers[CC] & 0x80000000)>>31) & L2Bt) == 0){
        JMP(mainMemory,registers,listSegments, flag);
    }
}


void JNZ(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    if ((((registers[CC] & 0x40000000)>>30) & L2Bt) == 0){
        JMP(mainMemory,registers,listSegments, flag);
    }
}

// FIN DE LAS OPERACIONES JMP ------------------------ !!


//Funcion NOT
void NOT(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers, listSegments, flag);
    int32_t resultado = ~valor;
    writeInOp(registers[OP2],resultado, mainMemory, registers, listSegments, flag);
    setCC(OP_NOT,valor,0,resultado,registers);
}

//Funciones de pila -------------------------------- !!
void PUSH(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag){
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers, listSegments, flag);
    pushValue(mainMemory, registers, listSegments, valor);
}

void POP(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag){
    int32_t valor;
    if (popValue(mainMemory, registers, listSegments, &valor))
        writeInOp(registers[OP2], valor, mainMemory, registers, listSegments, flag);
}

//Funcion CALL
void CALL(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag){
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers, listSegments, flag);

    if (valor < 0 || valor >= getSegmentSize(registers[CS], listSegments)){
        runtimeError(registers, "Salto fuera del segmento de codigo");
        return;
    }
    if (pushValue(mainMemory, registers, listSegments, registers[IP]))
        registers[IP] = (registers[IP] & H2Bt) | (valor & L2Bt);
}

//Funcion RET
void RET(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag){
    int32_t valor;
    if (popValue(mainMemory, registers, listSegments, &valor))
        registers[IP] = valor;
}




//Funcion ADD
void ADD(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);

    valor = a + b; //*a = *a +b

    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);

    setCC(OP_ADD,a,b,valor,registers);
}


//Funcion SUB
void SUB(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);

    valor = a + (~(uint32_t)b+1);

    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);

    setCC(OP_SUB,a,b,valor,registers);
}


//Funcion MUL
void MUL(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);

    valor = a * b;

    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);

    setCC(OP_MUL,a,b,valor,registers);
}


//Funcion DIV
void DIV(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor=0, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
        int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
        if(b==0){
            runtimeError(registers, "ERROR: DIVISION POR 0 NO EVALUADA");
        }
    else{
        
            valor = a / b;

            registers[AC]=a%b; //guardo resto de division entera en AC
        writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);
        

        setCC(OP_DIV,a,b,valor,registers);
    }
}


//Funcion XOR
void XOR(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);

    valor = a ^ b;

    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);

    setCC(OP_XOR,a,b,valor,registers);
}


//Funcion SWAP
void SWAP(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int flag){
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
    writeInOp(registers[OP1],b, mainMemory, registers, listSegments);
    writeInOp(registers[OP2],a,mainMemory,registers,listSegments);
    setCC(OP_XOR, a ^ b, a, b, registers);
}


//---Shifters---


//Funcion SHL
void SHL(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    uint32_t valor;
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
    valor = ((uint32_t)a) << b;
    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);
    setCC(OP_SHL,a,b,valor,registers);

}


//Funcion SHR
void SHR(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    uint32_t valor; //al ser unsigned, el shift agrega 0 por izquierda
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
    valor=(uint32_t)a>>b;
    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);
    setCC(OP_SHR,a,b,valor,registers);

}


//Funcion SAR
void SAR(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
    valor=a>>b; //como aca valor es signed, propaga el signo normalmente
    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);
    setCC(OP_SAR,a,b,valor,registers);

}

//Fin Shifters


//Funcion RND
void RND(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor;
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);

    valor=rand() % (b+1);
    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);

}


//Funcion CMP
void CMP(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
    valor=a-b;
    setCC(OP_CMP,a,b,valor,registers);
}


//Funcion AND
void AND(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
    valor = a & b;

    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);

    setCC(OP_AND,a,b,valor,registers);

}


//Funcion OR
void OR(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, flag);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);

    valor = a | b;

    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);

    setCC(OP_OR,a,b,valor,registers);

}


//Funcion STOP
void STOP(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    registers[IP] = -1;
}


//Funcion invalida, tira error.
void INVALID(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    runtimeError(registers, "ERROR CODIGO DE OPERACION INVALIDA");
}