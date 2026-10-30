#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stddef.h>
#include <ctype.h>


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
typedef struct {
    char *vmiPath;      // NULL si no se paso .vmi (los SYS F se ignoran)
    int   memKiB;       // tamaño de la memoria para el header de la imagen
    int   pasoAPaso;    // 1 si se presiono Enter en el ultimo breakpoint
} VMContext;
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

int32_t getValorOpnd(int32_t operando, int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void writeInOp(int32_t operando, int32_t valor, int8_t *mainMemory, int32_t *registers,int32_t listSegments[]);

void readNextInst(int8_t *mainMemory, int32_t *registers, int32_t listSegments[]);

int stringToInt(char *bin);

void loadInMemory(int32_t *registers, int8_t *mainMemory, int32_t listSegments[]);

void readFromMemory(int32_t *registers, int8_t *mainMemory, int32_t listSegments[]);

void SYS(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void setCC(OpType op, int32_t a, int32_t b, int32_t resultado, int32_t *registers);

void JMP(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void LDL(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void LDH(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void MOV(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void JN(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void JP(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void JZ(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void JC(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void JV(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void JNP(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void JNN(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void JNZ(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void NOT(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void ADD(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void SUB(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void MUL(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void DIV(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void XOR(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void SWAP(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void SHL(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void SHR(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void SAR(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void RND(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void CMP(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void AND(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void OR(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx);

void STOP(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx);

void PUSH(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void POP (int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void CALL(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void RET (int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void INVALID(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx);

void DISASSEMBLERV2(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], uint16_t entryPoint);

int parseArgs(int argc, char *argv[], VMArgs *args);

int endsWith(const char *s, const char *suf);

int readHeader(FILE *file, int version, uint16_t *segSize, uint16_t *entrypoint,int32_t memsize);

int buildSegments(int32_t *listSegments,int32_t *registers,int *segReg,uint16_t *segSize,int32_t memsize);

int pushValue(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int32_t valor);

int popValue(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int32_t *valor);

int reservarPila(int32_t *registers);

void escribirTope(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int32_t valor);

int getSegmentBase(int32_t reg, int32_t *listSegments);

int getSegmentSize(int32_t reg, int32_t *listSegments);

int setParamSegment(int32_t *listSegments,VMArgs * args, int8_t *mainMemory, int32_t *registers);

int paramSegmentSize(VMArgs *args);

void descartarLinea();

int guardarImagen(const char *path, int32_t *registers, int32_t *listSegments, int8_t *mainMemory, int memKiB);

int breakpoint(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx);

void escribeBigEndian(FILE *file, int32_t valor);

int32_t leeBigEndian(FILE *file);

int cargarVMI(const char *path, int32_t *registers, int32_t *listSegments, int8_t **mainMemory, int *memKiB);


int cargarVMX(VMArgs *args, int32_t *registers, int32_t *listSegments, int8_t *mainMemory, int32_t memsize);

int tamOperando(int32_t operando);


int main(int argc,char* argv[]){ //gcc virtualMachine.c -o vmx.exe -Wall -Wextra
    VMArgs args;
    VMContext ctx={0};
    if(!parseArgs(argc,argv,&args)){
        printf("ARGUMENTOS INVALIDOS\n");
        return 1;
    }
    srand(time(NULL)); //inicializa semilla al iniciar programa
    int32_t registers[32]={0}; //serian los 32 registros de 4 bytes que se piden
    int32_t listSegments[8]={0}; //serian los 8 segmentos de 4 bytes que se piden
    int8_t *mainMemory = NULL;
    void (*op[32])(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx)={SYS, JMP, JP, JN, JZ, JC, JV, JNP,JNN, JNZ, NOT, PUSH, POP, CALL, RET, STOP, MOV, ADD, SUB, MUL, DIV, CMP, AND, OR, XOR, SWAP, SHL, SHR, SAR, LDL, LDH, RND};

    ctx.vmiPath=args.vmiPath;
    if (args.vmxPath != NULL){      // .vmx (con o sin .vmi): se ejecuta el programa
        ctx.memKiB=args.memKiB;
        mainMemory = calloc(ctx.memKiB*1024, 1);
        if(mainMemory==NULL){
            printf("ERROR AL INICIALIZAR MEMORIA\n");
            return 1;
        }
        if (!cargarVMX(&args, registers, listSegments, mainMemory, ctx.memKiB*1024)){
            free(mainMemory);
            return 1;
        }
    }
    else{                           // solo .vmi: se retoma la imagen
        if (!cargarVMI(args.vmiPath, registers, listSegments, &mainMemory, &ctx.memKiB)){
            free(mainMemory);     
            return 1;
        }
    }
        

    int abortado=0;
    while (registers[IP] != -1){
        readNextInst(mainMemory, registers, listSegments);

        if (registers[IP] == -1){
            break;
        }
        
        if (registers[OPC]<=31 && registers[OPC]>=0){
            op[registers[OPC]](mainMemory, registers, listSegments, &ctx);
        }
        else{
            runtimeError(registers, "ERROR OPERACION INVALIDA");
        }
        if (ctx.pasoAPaso && !vmError){   // SYS F o paso a paso
            if (!breakpoint(mainMemory, registers, listSegments, &ctx)){
                abortado = 1;
                break;
            }
        }
    }
    free(mainMemory);
    if (vmError)
        return 1;
    if(!abortado)
        printf("Fin de proceso.\n");
    else
        printf("Proceso abortado por usuario\n");
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
            long m = strtol(a + 2, &fin, 10); //Convierte valor string de la memoria a long, guarda en el puntero doble a char fin la cadena que no se pudo convertir a long
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






//----------------------------DISASSEMBLER V2-------------------------------
void DISASSEMBLERV2(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], uint16_t entryPoint) {
    int32_t Lultinst, ultinst, Fultinst;

    char *opndNames[32] = {
        "SYS","JMP","JP","JN","JZ","JC","JV","JNP",
        "JNN","JNZ","NOT","PUSH","POP","CALL","RET","STOP",
        "MOV","ADD","SUB","MUL","DIV","CMP","AND","OR",
        "XOR","SWAP","SHL","SHR","SAR","LDL","LDH","RND"
    };

    char *registersNames[50] = {
        "IP", "OPC", "OP1", "OP2", "LAR", "MAR", "MBR", "SP", "BP", " ",
        "EAX", "EBX", "ECX", "EDX", "EEX", "EFX", "AC", "CC", " ", " ",
        " ", " ", " ", " ", " ", " ", "CS", "DS", "ES", "SS", "KS", "PS",
        
        "AL", "AH", "AX",
        
        "BL", "BH", "BX",
        
        "CL", "CH", "CX",
        
        "DL", "DH", "DX",
        
        "EL", "EH", "EX",
        
        "FL", "FH", "FX"
    };

    if (registers[KS] != -1) {
        int baseKS = getSegmentBase(registers[KS], listSegments);
        int tamKS = getSegmentSize(registers[KS], listSegments);
        
        if (tamKS > 0) {

            int offset = 0;

            while (offset < tamKS) {
                int stringStart = baseKS + offset;
                Fultinst = stringStart;
                
                int stringSize = 0;
                while (offset + stringSize < tamKS && mainMemory[baseKS + offset + stringSize] != '\0') {
                    stringSize++;
                }
                
                if (stringSize > 0 || (offset + stringSize < tamKS && mainMemory[baseKS + offset + stringSize] == '\0')) {
                    stringSize++;
                    
                    // bytes: todos si son hasta 7, si no los primeros 6 y ".."
                    char bytesStr[32] = "";
                    int mostrar = (stringSize > 7) ? 6 : stringSize;
                    for (int i = 0; i < mostrar; i++) {
                        char temp[8];
                        sprintf(temp, "%02X ", (uint8_t)mainMemory[stringStart + i]);
                        strcat(bytesStr, temp);
                    }
                    if (stringSize > 7)
                        strcat(bytesStr, ".. ");

                    // string completo, con '.' en los caracteres no imprimibles
                    printf(" [%04X] %-21s| \"", (uint16_t)Fultinst, bytesStr);
                    for (int i = 0; i < stringSize - 1; i++) {      // stringSize - 1: sin el '\0'
                        unsigned char c = (unsigned char)mainMemory[stringStart + i];
                        putchar(isprint(c) ? c : '.');
                    }
                    printf("\"\n");
                    
                    offset += stringSize;
                } else {
                    break;
                }
            }

        }
    }


    int tamCS = getSegmentSize(registers[CS], listSegments);

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
            int16_t offset;
            int sector;

            for (int i = 0; i < 1 + tipoa + tipob; i++) {
                char temp[8];
                sprintf(temp, "%02X ", (uint8_t)mainMemory[Fultinst + i]);
                strcat(bytesStr, temp);
            }

            switch (tipoA) {
                case 1: {
                    sector = (registers[OP1] >> 5) & 0b11;
                    codReg = registers[OP1] & 0b11111;
                    
                    int idx = codReg;
                    if (sector != 0 && codReg >= EAX && codReg <= EFX)
                        idx = 32 + (codReg - EAX) * 3 + (sector - 1);   // AL AH AX, BL BH BX, ..., FL FH FX
                    
                    sprintf(opA, "%s", registersNames[idx]);
                    break;
                }

                case 3: {
                    int modifier = (registers[OP1] >> 5) & 0b111;
                    codReg = registers[OP1] & 0b11111;
                    offset = (int16_t)((registers[OP1] >> 8) & 0xFFFF);
                    char *modStr = (modifier == 1) ? "b" : (modifier == 2) ? "w" : "l";   // sin prefijo = 4 bytes = l
                    sprintf(opA, "%s[%s%+d]", modStr, registersNames[codReg], offset);     // %+d: siempre con signo (+0, +4, -4)
                    break;
                }

                default:
                    strcpy(opA, "");
                    break;
            }

            switch (tipoB) {
                case 1: {
                    sector = (registers[OP2] >> 5) & 0b11;
                    codReg = registers[OP2] & 0b11111;
                    
                    int idx = codReg;
                    if (sector != 0 && codReg >= EAX && codReg <= EFX)
                        idx = 32 + (codReg - EAX) * 3 + (sector - 1);   // AL AH AX, BL BH BX, ..., FL FH FX
                    
                    sprintf(opB, "%s", registersNames[idx]);
                    break;
                }

                case 2: {
                    int32_t valor = (int16_t)(registers[OP2] & 0xFFFF);
                    sprintf(opB, "%d", valor);
                    break;
                }

                case 3: {
                    int modifier = (registers[OP2] >> 5) & 0b111;
                    codReg = registers[OP2] & 0b11111;
                    offset = (int16_t)((registers[OP2] >> 8) & 0xFFFF);
                    char *modStr = (modifier == 1) ? "b" : (modifier == 2) ? "w" : "l";   // sin prefijo = 4 bytes = l
                    sprintf(opB, "%s[%s%+d]", modStr, registersNames[codReg], offset);     // %+d: siempre con signo (+0, +4, -4)
                    break;
                }

                default:
                    strcpy(opB, "");
                    break;
            }

            char marca = ((Lultinst & 0xFFFF) == entryPoint) ? '>' : ' ';

            if (tipoA == 0 && tipoB == 0)               // sin operandos
                printf("%c[%04X] %-21s| %-4s\n",
                    marca, (uint16_t)Fultinst, bytesStr, opndNames[registers[OPC]]);
            else if (tipoA == 0)                        // un operando (esta en OP2)
                printf("%c[%04X] %-21s| %-4s %10s\n",
                    marca, (uint16_t)Fultinst, bytesStr, opndNames[registers[OPC]], opB);
            else {                                      // dos operandos
                char opAComa[70];
                sprintf(opAComa, "%s,", opA);           // la coma va pegada al operando A
                printf("%c[%04X] %-21s| %-4s %11s %10s\n",
                    marca, (uint16_t)Fultinst, bytesStr, opndNames[registers[OPC]], opAComa, opB);
            
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
//Descarta lo que quede en la linea de entrada, incluido el '\n'
void descartarLinea(){
    int c=getchar();
    while(c!='\n' && c!=EOF)
        c=getchar();
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


int setParamSegment(int32_t *listSegments, VMArgs *args, int8_t *mainMemory, int32_t *registers) {
    if (registers[PS] == -1) {
        return -1;
    }
    int basePS = getSegmentBase(registers[PS],listSegments);
    int offset = 0;
    int stringOffsets[args->paramCount+1];
    
    // 1. Guardar todas las cadenas (nombre archivo + parámetros)
    
    for (int i = 0; i < args->paramCount+1; i++) {
        stringOffsets[i] = offset;
        char *str = (i == 0) ? args->vmxPath : args->params[i - 1];
        int len = strlen(str)+1;
        for (int j = 0; j < len; j++) {
            mainMemory[basePS + offset + j] = str[j];
        }
        offset += len;
    }
    

    // 2. Guardar arreglo argv con punteros a cada string
    int inicioArgv=offset;
    for (int i = 0; i < args->paramCount + 1; i++) {

        int32_t puntero = (0x0000 << 16) | (stringOffsets[i] & 0xFFFF);
        
        mainMemory[basePS + offset + 0] = (puntero >> 24) & 0xFF;
        mainMemory[basePS + offset + 1] = (puntero >> 16) & 0xFF;
        mainMemory[basePS + offset + 2] = (puntero >> 8) & 0xFF;
        mainMemory[basePS + offset + 3] = puntero & 0xFF;
        
        offset += 4;
    }
  return inicioArgv;
}

int paramSegmentSize(VMArgs *args){
    int s;
    if(args->params==NULL)
        return 0;
    else{
        s=strlen(args->vmxPath)+1;
        for(int i=0;i<args->paramCount;i++){
            s+=strlen(args->params[i])+1;

        }
        s+=(args->paramCount+1)*4;
        return s;

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
    return 1; //el SP lo incrementa quien llama, despues de asignar
}

//Reserva 4 bytes en la pila (pasos 1 y 2 de PUSH). Devuelve 0 si hubo overflow
int reservarPila(int32_t *registers){
    if(registers[SS] == -1){
        runtimeError(registers, "STACK OVERFLOW");
        return 0;
    }
    registers[SP]-=4;
    if (registers[SP]<registers[SS]){
        runtimeError(registers, "STACK OVERFLOW");
        return 0;
    }
    return 1;
}

//Escribe un valor de 4 bytes en el tope de la pila
void escribirTope(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int32_t valor){
    registers[LAR] = registers[SP];
    registers[MAR] = 4 << 16;      //loadInMemory saca el tamaño de la parte alta de MAR
    registers[MBR]=valor; //Cargo valor en el MBR
    loadInMemory(registers, mainMemory, listSegments);
}


//Pushea un valor a la pila, devuelve 1 si salio bien y 0 si hubo overflow, guarda en valor el elemento pusheado
int pushValue(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], int32_t valor){
    if (!reservarPila(registers))
        return 0;
    escribirTope(mainMemory, registers, listSegments, valor);
    return 1;
}


int getSegmentBase(int32_t reg, int32_t *listSegments){ //Funcion que obtiene la base del segmento al que apunta reg
    return (listSegments[(reg >> 16) & 0xFFFF] >> 16) & 0xFFFF;
}

int getSegmentSize(int32_t reg, int32_t *listSegments){ //Funcion que obtiene el tamaño del segmento al que apunta reg
    return listSegments[(reg >> 16) & 0xFFFF] & 0xFFFF;
}

//Funciones .vmi -------------------------------------------------- !!
//Guarda la imagen de la maquina virtual en el archivo .vmi
//Header (8) + registros (32 x 4) + tabla de segmentos (8 x 4) + memoria completa
int guardarImagen(const char *path, int32_t *registers, int32_t *listSegments, int8_t *mainMemory, int memKiB){
    FILE *file = fopen(path, "wb");
    if (file == NULL){
        return 0;
    }
    uint8_t header[8] = {'V','M','I','2','6', 1, (memKiB >> 8) & 0xFF, memKiB & 0xFF};
    fwrite(header, 1, 8, file);
    for (int i = 0; i < 32; i++)
        escribeBigEndian(file, registers[i]);
    for (int i = 0; i < NUM_SEGMENTOS; i++)
        escribeBigEndian(file, listSegments[i]);
    fwrite(mainMemory, 1, memKiB * 1024, file);
    fclose(file);
    return 1;
}
//Lee un entero de 4 bytes en big endian del archivo
int32_t leeBigEndian(FILE *file){
    uint8_t b[4] = {0};
    fread(b, 1, 4, file);
    return (int32_t)(((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3]);
}


//Carga la memoria, los registros y la tabla de segmentos desde un archivo de imagen .vmi
//Reserva la memoria con el tamaño indicado en la imagen. Devuelve 0 si hubo error
int cargarVMI(const char *path, int32_t *registers, int32_t *listSegments, int8_t **mainMemory, int *memKiB){
    uint8_t header[8];
    FILE *file = fopen(path, "rb");
    if (file == NULL){
        printf("ARCHIVO DE IMAGEN INEXISTENTE: %s\n", path);
        return 0;
    }
    if (fread(header, 1, 8, file) != 8 || memcmp(header, "VMI26", 5) != 0 || header[5] != 1){
        printf("ARCHIVO DE IMAGEN INVALIDO\n");
        fclose(file);
        return 0;
    }
    *memKiB = (header[6] << 8) | header[7];
    *mainMemory = calloc(*memKiB * 1024, 1);
    if (*mainMemory == NULL){
        printf("ERROR AL INICIALIZAR MEMORIA\n");
        fclose(file);
        return 0;
    }
    for (int i = 0; i < 32; i++)
        registers[i] = leeBigEndian(file);
    for (int i = 0; i < NUM_SEGMENTOS; i++)
        listSegments[i] = leeBigEndian(file);
    if (fread(*mainMemory, 1, *memKiB * 1024, file) != (size_t)(*memKiB * 1024)){
        printf("ARCHIVO DE IMAGEN INCOMPLETO\n");
        fclose(file);
        return 0;
    }
    fclose(file);
    return 1;
}


//Funcion auxiliar, escribe un entero de 4 bytes en el archivo en big endian
void escribeBigEndian(FILE *file, int32_t valor){
    uint8_t b[4];
    b[0] = (valor >> 24) & 0xFF;
    b[1] = (valor >> 16) & 0xFF;
    b[2] = (valor >> 8) & 0xFF;
    b[3] = valor & 0xFF;
    fwrite(b, 1, 4, file);
}

//Carga un programa .vmx: segmentos, codigo, constantes, Param Segment, pila e IP
//Deja la VM lista para ejecutar. Devuelve 0 si hubo error
int cargarVMX(VMArgs *args, int32_t *registers, int32_t *listSegments, int8_t *mainMemory, int32_t memsize){
    int segReg[6] = {PS, KS, CS, DS, ES, SS};
    uint16_t segSize[6] = {0};
    uint16_t entryPoint = 0;

    FILE * arch = fopen(args->vmxPath, "rb");
    int version = verifyFile(arch);
    if(!version)
        return 0;
        
    //Se cargan las variables del archivo y se setean los arrays.
    if(!readHeader(arch,version,segSize,&entryPoint,memsize)){
        fclose(arch);
        return 0;

    }
    if(version==2){
        int tamPS=paramSegmentSize(args); 
        if(tamPS > 0xFFFF){ //Verifico que no se pase de uint16_t (tamaño del arreglo segSize)
                printf("PARAMETROS DEMASIADO GRANDES\n");
                fclose(arch);
                return 0;
        }
        segSize[0] = tamPS; //Setea en el registro de tamaños el tamaño de PS  

        
    }
    if(!buildSegments(listSegments,registers,segReg,segSize,memsize)){
        fclose(arch);
        return 0;
    }
    int baseCS = getSegmentBase(registers[CS], listSegments);
    loadBytes(arch, segSize[2], mainMemory + baseCS);

    if (registers[KS] != -1) {
        int baseKS = getSegmentBase(registers[KS],listSegments);
        loadBytes(arch, segSize[1], mainMemory + baseKS);
    }
    //Inicializacion Param Segment
    int32_t argcVal=0, argvVal=-1;
    if(registers[PS]!=-1){
        int offArgv = setParamSegment(listSegments, args, mainMemory, registers);
        argcVal = args->paramCount + 1;
        argvVal = (registers[PS] & 0xFFFF0000) | offArgv;

    }
    // Inicialización de la pila: SP al final del SS; se apilan argv, argc y RET
    if (registers[SS] != -1) {
    registers[SP] = registers[SS] + segSize[5];
    if (!pushValue(mainMemory, registers, listSegments, argvVal) || 
        !pushValue(mainMemory, registers, listSegments, argcVal)  || 
        !pushValue(mainMemory, registers, listSegments, -1)) {   // RET
        fclose(arch);
        return 0;
    }
}





    if (args->disassemble){
        registers[IP] = registers[CS];
        DISASSEMBLERV2(mainMemory, registers, listSegments, entryPoint);
    }

    registers[IP]=registers[CS] | entryPoint;
    fclose(arch);
    return 1;




}

//Breakpoint: guarda la imagen y espera la accion del usuario
//g: continua | Enter: ejecuta una instruccion y vuelve a frenar | q: aborta
//Devuelve 0 si el usuario aborto la ejecucion
int breakpoint(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx){
    int c;
    if (!guardarImagen(ctx->vmiPath, registers, listSegments, mainMemory, ctx->memKiB)){
        runtimeError(registers, "ERROR AL GUARDAR LA IMAGEN");
        return 1;
    }
    do {
        printf("[BREAKPOINT] g: continuar | q: salir | Enter: paso a paso\n");
        c = getchar();
        if(c == EOF)
            return 0;
        if (c != '\n')
            descartarLinea();           // consume el resto de la linea (ej: "g\n")
    } while (c != 'g' && c != 'q' && c != '\n');

    if (c == 'q')
        return 0;
    ctx->pasoAPaso = (c == '\n');       // Enter: sigue paso a paso; g: corre libre
        return 1;
}

//-------------------------------------- Fin funciones VMI !!

//funcion que carga el codigo en la memoria principal.
void loadBytes(FILE *file, uint16_t cSize, int8_t *mainMemory) {
    int i;
    for (i=0; i<cSize; i++){
        fread(&(mainMemory[i]), sizeof(int8_t), 1, file);
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
int32_t getValorOpnd(int32_t operando, int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    int8_t tipo = (operando >> 24) & 0x000000FF;
    int16_t codReg;
    int32_t valor=0;
    int16_t offset;
        switch (tipo) {
            case 1:{ //operando de registro
                codReg = operando & 0b11111;
                int opndType = (operando >> 5) & 0b11; 

                if (opndType == 0){
                    valor = registers[codReg];
                }
                else if (opndType == 1){
                    valor = (int8_t) (registers[codReg]  & 0xFF);
                }
                else if (opndType == 2){
                    valor = (int8_t) ((registers[codReg] >> 8) & 0xFF);
                }
                else{
                    valor =(int16_t) (registers[codReg] & L2Bt);
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
    
    
    return valor;
}


//Funcion que guarda en el operando 1 un valor proveniente de una operacion.
void writeInOp(int32_t operando, int32_t valor, int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int32_t tipo = (operando >> 24) & 0X0000000F;
    int8_t codReg;
    int16_t offset;
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
                }
                else if (opndType == 1){
                    registers[MAR] = (1 << 16);
                }
                else{
                    registers[MAR] = (2 << 16);
                }
                registers[MBR]=valor;
                loadInMemory(registers,mainMemory,listSegments);
            }
        }

}

//Devuelve el tamaño en bytes del operando: sector de registro o modificador de memoria
int tamOperando(int32_t operando){
    int tipo = (operando >> 24) & 0xFF;
    if (tipo == 1){                             // registro: 00 = 4, 01 = byte bajo, 10 = 3er byte, 11 = 2 bytes
        int sector = (operando >> 5) & 0b11;
        if (sector == 0) return 4;
        if (sector == 3) return 2;
        return 1;
    }
    if (tipo == 3){                             // memoria: 001 = b, 010 = w, 100 o sin prefijo = l
        int mod = (operando >> 5) & 0b111;
        if (mod == 1) return 1;
        if (mod == 2) return 2;
    }
    return 4;
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
        for (int i = 0; i < opSize; i++) {
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
void SYS (int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    int op = getValorOpnd(registers[OP2],mainMemory,registers,listSegments,ctx);
    int i;
    char auxS[33];
    int32_t aux=registers[ECX]&L2Bt, tam=(registers[ECX]>>16)&L2Bt;
    if(op!=15 && op!=0){ //La consigna dice explicitamente que estos llamados SYS no modifican registros
        registers[LAR] = registers[EDX];
        registers[MAR] = (registers[ECX]&H2Bt);
    }

    switch (op){
        case 1:{
            switch (registers[EAX]){
                case 1:
                    for (i=0;i<aux;i++){
                        printf("\n");
                        printf("[%04x]:",getDir(registers[LAR],listSegments));
                        scanf(" %d",&registers[MBR]);
                        descartarLinea();
                        loadInMemory(registers,mainMemory,listSegments);
                        registers[LAR]+=tam;
                    }    
                break;
                case 2:
                    for (i=0;i<aux;i++){
                            char c;
                            printf("[%04x]:",getDir(registers[LAR],listSegments));
                            scanf(" %c",&c);
                            descartarLinea();
                            registers[MBR]=(int32_t)(unsigned char) c;
                            loadInMemory(registers,mainMemory,listSegments);
                            registers[LAR]+=tam;
                    }
                break;
                case 4:
                    for (i=0;i<aux;i++){
                        printf("[%04x]:",getDir(registers[LAR],listSegments));
                        scanf(" %o",&registers[MBR]);
                        descartarLinea();
                        loadInMemory(registers,mainMemory,listSegments);
                        registers[LAR]+=tam;
                    }
                break;
                case 8:
                    for (i=0;i<aux;i++){
                        printf("[%04x]:",getDir(registers[LAR],listSegments));
                        scanf(" %x",&registers[MBR]);
                        descartarLinea();
                        loadInMemory(registers,mainMemory,listSegments);
                        registers[LAR]+=tam;
                    }
                break;
                case 16:
                    for (i=0;i<aux;i++){
                        printf("[%04x]:",getDir(registers[LAR],listSegments));
                        scanf(" %32s",auxS);
                        descartarLinea();
                        registers[MBR]=(int32_t)stringToInt(auxS);
                        loadInMemory(registers,mainMemory,listSegments);
                        registers[LAR]+=tam;
                    }
                break;
            }

            break;
        }

        case 2:{
            int bytesCelda = (tam > 4) ? 4 : tam;
            for (i=0;i<aux;i++){
                readFromMemory(registers,mainMemory,listSegments); // lee UNA vez, deja el valor en MBR
                if (registers[IP] == -1)        // error de segmento: no se imprime basura
                    break;
                uint32_t sinSigno = (uint32_t)registers[MBR];
                if (bytesCelda < 4)             // binario, octal y hexa solo con los bytes de la celda
                    sinSigno &= (1u << (bytesCelda * 8)) - 1;

                printf("[%04x]:", getDir(registers[LAR],listSegments));
                if (registers[EAX] & 0x10){     // bit 4: binario
                    intToString(auxS, (int)sinSigno);
                    printf(" 0b%s", auxS);
                }
                if (registers[EAX] & 0x08)      // bit 3: hexadecimal
                    printf(" 0x%x", sinSigno);
                if (registers[EAX] & 0x04)      // bit 2: octal
                    printf(" 0o%o", sinSigno);
                if (registers[EAX] & 0x02){     // bit 1: un caracter por cada byte de la celda
                    printf(" ");
                    for (int b = bytesCelda - 1; b >= 0; b--){
                        unsigned char c = (registers[MBR] >> (8 * b)) & 0xFF;
                        putchar(isprint(c) ? c : '.');
                    }
                }
                if (registers[EAX] & 0x01)      // bit 0: decimal
                    printf(" %d", registers[MBR]);
                printf("\n");

                registers[LAR]+=tam; // avanza UNA sola vez por celda, no por formato
            }
            break;

            }
        
        case 3:{
                int32_t max = registers[ECX];   // -1: sin limite
                int c, cant = 0;
                registers[MAR] = (1 << 16);     // de a 1 byte
                printf("[%04x]:",getDir(registers[LAR],listSegments));
                c = getchar();
                while (c != '\n' && c!=EOF && (max == -1 || cant < max) && registers[IP] != -1){
                    registers[MBR] = c;
                    loadInMemory(registers,mainMemory,listSegments);
                    registers[LAR] += 1;
                    cant++;
                    c = getchar();
                }
                while (c != '\n' && c!=EOF)   // si se corto por ECX, descarta el resto de la linea
                    c = getchar();
                if (registers[IP] != -1){       // '\0' final
                    registers[MBR] = 0;
                    loadInMemory(registers,mainMemory,listSegments);
                }
            break;
        }

        case 4:{
            registers[MAR] = (1 << 16);     // de a 1 byte
            readFromMemory(registers,mainMemory,listSegments);
            while (registers[IP] != -1 && registers[MBR] != 0){
                putchar((char)registers[MBR]);
                registers[LAR] += 1;
                readFromMemory(registers,mainMemory,listSegments);
            }
            break;
        }
        case 15:{
            if(ctx->vmiPath!=NULL)
                ctx->pasoAPaso=1;
            return; //Vuelvo para no modificar registros
        }

        case 0:{
            fflush(stdout);
            #ifdef _WIN32
                system("cls");
            #else
                system("clear");
            #endif
            return; //Vuelvo para no modificar registros
        }
        default:{
            runtimeError(registers, "SYS: Operacion invalida");
            break;
        }
       
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
            if ((uint32_t)b >= 32){             // salen todos los bits
                C = (a != 0);
                V = (a != 0);
            }
            else{
                uint64_t shiftC = (uint64_t)(uint32_t)a << (uint64_t)(uint32_t)b;
                int64_t shiftV= (int64_t)a << (int64_t)b;

                C = (((shiftC >> 32) & L4Bt)!=0);
                V = ((shiftV) != ((int32_t)(shiftV)));
            }
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
void JMP (int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers, listSegments, ctx);

    if (((listSegments[(registers[CS] >> 16) & L2Bt] & L2Bt) > valor) &&  (valor>=0)){
        registers[IP] =(registers[IP] & H2Bt)+valor;
    }
    else{
        runtimeError(registers, "Salto fuera del segmento de codigo");
    }
}


//Funcion LOAD DATA LOW
void LDL (int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx) {
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers, listSegments, ctx);
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, ctx);
    valor = valor & 0xFFFF;
    a =(a& 0xFFFF0000) | valor;
    writeInOp(registers[OP1],a, mainMemory, registers, listSegments);
}


//Funcion LOAD DATA HIGH
void LDH (int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx) {
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers, listSegments, ctx);
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, ctx);
    a = (a & 0x0000FFFF) | ((valor & 0xFFFF) << 16);
    writeInOp(registers[OP1],a, mainMemory, registers, listSegments);
}


//Funcion MOV
void MOV(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, ctx);

    writeInOp(registers[OP1],valor, mainMemory, registers, listSegments);
    setCC(OP_MOV,0,0,valor,registers);
}


//FUNCIONES JUMP CONDICIONALES --------------------- !!
void JN(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    if (((registers[CC] & 0x80000000)>>31) & L2Bt){
        JMP(mainMemory,registers,listSegments, ctx);
}
}


void JP(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    if ((((registers[CC] & 0xC0000000)>>30) & L2Bt) == 0){
        JMP(mainMemory,registers,listSegments, ctx);
    }
}


void JZ(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    if (((registers[CC] & 0x40000000)>>30) & L2Bt){
        JMP(mainMemory,registers,listSegments, ctx);
    }
}


void JC(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    if (((registers[CC] & 0x20000000)>>29) & L2Bt){
        JMP(mainMemory,registers,listSegments, ctx);
    }
}


void JV(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    if (((registers[CC] & 0x10000000)>>28) & L2Bt){
        JMP(mainMemory,registers,listSegments, ctx);
    }
}


void JNP(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    if ((((registers[CC] & 0x80000000)>>31) & L2Bt) || (((registers[CC] & 0x40000000)>>30) & L2Bt)){
        JMP(mainMemory,registers,listSegments, ctx);
    }
}


void JNN(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    if ((((registers[CC] & 0x80000000)>>31) & L2Bt) == 0){
        JMP(mainMemory,registers,listSegments, ctx);
    }
}


void JNZ(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    if ((((registers[CC] & 0x40000000)>>30) & L2Bt) == 0){
        JMP(mainMemory,registers,listSegments, ctx);
    }
}

// FIN DE LAS OPERACIONES JMP ------------------------ !!


//Funcion NOT
void NOT(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers, listSegments, ctx);
    int32_t resultado = ~valor;
    writeInOp(registers[OP2],resultado, mainMemory, registers, listSegments);
    setCC(OP_NOT,valor,0,resultado,registers);
}

//Funciones de pila -------------------------------- !!
void PUSH(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx){
    if (!reservarPila(registers))           // pasos 1 y 2: SP -= 4 y overflow
        return;
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers, listSegments, ctx);  // paso 3, con el SP nuevo
    escribirTope(mainMemory, registers, listSegments, valor);
}

void POP(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx){
    int32_t valor;
    if (popValue(mainMemory, registers, listSegments, &valor)){
        writeInOp(registers[OP2], valor, mainMemory, registers, listSegments);   // paso 4
        registers[SP]+=4;                                                         // paso 5
    }
}

//Funcion CALL
void CALL(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx){

    int flag = pushValue(mainMemory, registers, listSegments, registers[IP]);
    
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers, listSegments, ctx);

    if (valor < 0 || valor >= getSegmentSize(registers[CS], listSegments)){
        runtimeError(registers, "Salto fuera del segmento de codigo");
        return;
    }
    if (flag)
        registers[IP] = (registers[IP] & H2Bt) | (valor & L2Bt);
}

//Funcion RET
void RET(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx){
    int32_t valor;
    if (popValue(mainMemory, registers, listSegments, &valor)){
        registers[IP] = valor;
        registers[SP]+=4;
    }
}




//Funcion ADD
void ADD(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, ctx);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, ctx);

    valor = a + b; //*a = *a +b

    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);

    setCC(OP_ADD,a,b,valor,registers);
}


//Funcion SUB
void SUB(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, ctx);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, ctx);

    valor = a + (~(uint32_t)b+1);

    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);

    setCC(OP_SUB,a,b,valor,registers);
}


//Funcion MUL
void MUL(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, ctx);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, ctx);

    valor = a * b;

    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);

    setCC(OP_MUL,a,b,valor,registers);
}


//Funcion DIV
void DIV(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    int32_t valor=0, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, ctx);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, ctx);
    if(b==0){
        runtimeError(registers, "ERROR: DIVISION POR 0 NO EVALUADA");
    }
    else{
        if (a == INT32_MIN && b == -1){     // el resultado no entra en 32 bits: queda igual que en complemento a 2
            valor = INT32_MIN;
            registers[AC] = 0;
        }
        else{
            valor = a / b;
            registers[AC]=a%b; //guardo resto de division entera en AC
        }
        writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);
        setCC(OP_DIV,a,b,valor,registers);
    
    }
}


//Funcion XOR
void XOR(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, ctx);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, ctx);

    valor = a ^ b;

    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);

    setCC(OP_XOR,a,b,valor,registers);
}


//Funcion SWAP
void SWAP(int8_t *mainMemory, int32_t *registers, int32_t listSegments[], VMContext *ctx){
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, ctx);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, ctx);
    writeInOp(registers[OP1],b, mainMemory, registers, listSegments);
    writeInOp(registers[OP2],a,mainMemory,registers,listSegments);
    setCC(OP_XOR, a ^ b, a, b, registers);
}


//---Shifters---


//Funcion SHL
void SHL(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    uint32_t valor;
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, ctx);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, ctx);
    valor = ((uint32_t)b >= 32) ? 0 : ((uint32_t)a) << b;   // 32 o mas: salen todos los bit
    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);
    setCC(OP_SHL,a,b,valor,registers);

}


//Funcion SHR
void SHR(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    uint32_t valor; //al ser unsigned, el shift agrega 0 por izquierda
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, ctx);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, ctx);
    int tam = tamOperando(registers[OP1]);
    uint32_t sinSigno = (uint32_t)a;
    if (tam < 4)                                 // AL, AH, AX, b[], w[]: sin el signo extendido, asi entran ceros
        sinSigno &= (1u << (tam * 8)) - 1;
    valor = ((uint32_t)b >= 32) ? 0 : sinSigno >> b;
    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);
    setCC(OP_SHR,a,b,valor,registers);

}


//Funcion SAR
void SAR(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, ctx);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, ctx);
    if ((uint32_t)b >= 32)
        valor = (a < 0) ? -1 : 0;               // 32 o mas: queda todo el signo
    else
        valor = a >> b; //como aca valor es signed, propaga el signo normalmente
    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);
    setCC(OP_SAR,a,b,valor,registers);

}

//Fin Shifters


//Funcion RND
void RND(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    int32_t valor;
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, ctx);
    if (b < 0) 
        b = 0;     //evita rand() % 0
    valor=rand() % (b+1);
    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);

}


//Funcion CMP
void CMP(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, ctx);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, ctx);
    valor=a-b;
    setCC(OP_CMP,a,b,valor,registers);
}


//Funcion AND
void AND(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, ctx);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, ctx);
    valor = a & b;

    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);

    setCC(OP_AND,a,b,valor,registers);

}


//Funcion OR
void OR(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments, ctx);
    int32_t b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, ctx);

    valor = a | b;

    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);

    setCC(OP_OR,a,b,valor,registers);

}


//Funcion STOP
void STOP(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    registers[IP] = -1;
}


//Funcion invalida, tira error.
void INVALID(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], VMContext *ctx){
    runtimeError(registers, "ERROR CODIGO DE OPERACION INVALIDA");
}