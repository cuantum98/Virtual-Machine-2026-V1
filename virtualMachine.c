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
#define H4Bt 0xFFFFFFFF00000000
#define L4Bt 0x00000000FFFFFFFF

int verifyFile(FILE *file);

void loadCodeSize(FILE *file, int16_t *cSize);

void loadCode(FILE *file, int16_t cSize, int8_t *mainMemory);

void setSegments(int32_t listSegments[], int16_t cSize);

void setRegisters(int32_t registers[32]);

int getSegmentSize(int32_t reg, int32_t *listSegments);

int16_t getDir(int32_t logicDir, int32_t listSegments[]);

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

void INVALID(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag);

void DISASSEMBLER(int8_t *mainMemory, int32_t *registers, int32_t listSegments[]);

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

        int32_t Lultinst,Fultinst;
        int8_t ultinst;
        if (argc == 3) {
            flagD = argv[2];
        } else {
            flagD = "a";
}
        
        //Se cargan las variables del archivo y se setean los arrays.
        loadCodeSize(arch, &cSize);
        loadCode(arch, cSize, mainMemory);
        setRegisters(registers);
        setSegments(listSegments, cSize);
        flagaux = (strcmp(flagD,"-d")==0)? 1:0;

        if (flagaux){
            DISASSEMBLER(mainMemory, registers, listSegments);
            registers[IP]=registers[CS];
        }
        
        
        while (registers[IP] != -1){
            Lultinst=registers[IP];
            readNextInst(mainMemory, registers, listSegments);

            if (registers[IP] == -1){
                break;
            }
            
            if (registers[OPC]<=31 && registers[OPC]>=0){
                op[registers[OPC]](mainMemory, registers, listSegments, flagaux);
            }
            else{
                printf("ERROR OPERACION INVALIDA");
                registers[IP]=-1;
            }
        }
        fclose(arch);
        printf("Fin de proceso.");
    }
    return 0;
}


//----------------------------DISASSEMBLER-------------------------------
void DISASSEMBLER(int8_t *mainMemory, int32_t *registers, int32_t listSegments[]) {
    int32_t Lultinst, ultinst, Fultinst;

    char *opndNames[32] = {
        "SYS","JMP","JP","JN","JZ","JC","JV","JNP",
        "JNN","JNZ","NOT"," "," "," "," ","STOP",
        "MOV","ADD","SUB","MUL","DIV","CMP","AND","OR",
        "XOR","SWAP","SHL","SHR","SAR","LDL","LDH","RND"
    };

    char *registersNames[32] = {
        "IP","OPC","OP1","OP2","LAR","MAR","MBR"," "," "," ",
        "EAX","EBX","ECX","EDX","EEX","EFX","AC","CC"," "," ",
        " "," "," "," "," "," ","CS","DS"," "," "," "," "
    };

    int aux = getSegmentSize(registers[CS], listSegments);

    while (registers[IP] < aux) {
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
    uint8_t b[2];
    if (fread(b, 1, 2, file) != 2) {
        *cSize = 0;
        printf("Cabecera invalida");
        return;
    }
    *cSize = (int16_t)((b[0] << 8) | b[1]);
    printf("Tamaño del código: %d bytes\n", *cSize);
}


//funcion que carga el codigo en la memoria principal.
void loadCode(FILE *file, int16_t cSize, int8_t *mainMemory) {
    int i;
    for (i=0; i<cSize; i++){
        fread(&(mainMemory[i]), sizeof(int8_t), 1, file);
    }
}


//Funcion que inicializa la lista de segmentos.
void setSegments(int32_t listSegments[], int16_t cSize){
    int i=2;
    // Segmento de código: base=0, tamaño=cSize
    listSegments[0] = (0 << 16) | (cSize & 0xFFFF);
    
    // Segmento de datos: base=cSize, tamaño=(16KiB - cSize)
    listSegments[1] = (cSize << 16) | ((MAXMEMORY - cSize) & 0xFFFF);
    
    while (i < 8){
        listSegments[i]=0xFFFFFFFF;
        i++;
    }
}


//Funcion para inicializar los registros más importantes.
void setRegisters(int32_t registers[32]){
    registers[CS] = 0x00000000;  // Apunta a segmento 0
    registers[DS] = 0x00010000;  // Apunta a segmento 1
    registers[IP] = registers[CS];
    registers[AC] = 0;
}


//Funcion para obtener la ultima posicion de memoria de un segmento determinado por un registro.
int getSegmentSize(int32_t reg, int32_t *listSegments){
    int base , tam ;
    int indice = (int16_t)((reg >>16) & L2Bt);

    base = (listSegments[indice] >> 16) & L2Bt;
    tam = listSegments[indice] & L2Bt;
    return base + tam;
}


//Funcion para obtener la direccion fisica a partir de la logica.
int16_t getDir(int32_t logicDir, int32_t listSegments[]){
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


//Funcion para leer la siguiente instruccion
void readNextInst(int8_t *mainMemory, int32_t *registers, int32_t listSegments[]){
    if (registers[IP]!=-1){
        int dir = getDir(registers[IP],listSegments);

        if (dir == -1) {
            printf("Error de segmento al leer siguiente instruccion\n");
            STOP(mainMemory, registers, listSegments, 0);
            return;
        }  

        int16_t tamSegmento = getSegmentSize(registers[CS], listSegments);

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
            printf("Error de segmento al leer siguiente instruccion\n");
            STOP(mainMemory, registers, listSegments, 0);
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
    int16_t fDir = getDir(registers[LAR], listSegments);
    
    if(fDir != -1){

        // Extraer información de MAR
        int16_t opSize = (registers[MAR] >> 16) & 0xFFFF;  // Tamaño en bits altos
        registers[MAR] = (registers[MAR] & 0xFFFF0000) | ((uint16_t)fDir);
        // Validar que el acceso esté dentro del segmento
        int limSegmento = getSegmentSize(registers[LAR], listSegments);
        
        // fDir es la dirección física de inicio
        // Necesitamos verificar que [fDir ... fDir + opSize - 1] esté dentro del segmento
        if ((fDir + opSize) > limSegmento) {
            printf("Error de segmento al cargar dato en memoria\n");
            printf("Intento de acceso: [%d, %d), Límite del segmento: %d\n", fDir, fDir + opSize, limSegmento);
            STOP(mainMemory, registers, listSegments, 0);
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
        printf("Error de segmento al obtener direccion fisica del registro LAR\n");
        STOP(mainMemory, registers, listSegments, 0);
    }
}


//Funcion que lee una variable de la memoria y la carga en el MBR dependiendo del MAR. 
void readFromMemory(int32_t *registers, int8_t *mainMemory, int32_t listSegments[]){
    int16_t fDir = getDir(registers[LAR], listSegments);
    
    if (fDir != -1){
        // Extraer información de MAR
        int16_t opSize = (registers[MAR] >> 16) & 0xFFFF;  // Tamaño en bits altos
        registers[MAR] = (registers[MAR] & 0xFFFF0000) | ((uint16_t)fDir);
        // Validar que el acceso esté dentro del segmento
        int limSegmento = getSegmentSize(registers[LAR], listSegments);
        
        if ((fDir + opSize) > limSegmento) {
            printf("Error de segmento al leer dato de la memoria\n");
            printf("Intento de acceso: [%d, %d), Límite del segmento: %d\n", fDir, fDir + opSize, limSegmento);
            STOP(mainMemory, registers, listSegments, 0);
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
        printf("Error de segmento al obtener direccion fisica del registro LAR\n");
        STOP(mainMemory, registers, listSegments, 0);
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
    }
    else{
        if(op == 2){
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
        printf("Salto fuera del segmento de codigo");
        STOP(mainMemory, registers, listSegments, flag);
    }
}


//Funcion LOAD DATA LOW
void LDL (int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag) {
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers, listSegments, flag);
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
    valor = valor & 0xFFFF;
    a =(a& 0xFFFF0000) | valor;
    writeInOp(registers[OP1],a, mainMemory, registers, listSegments);
}


//Funcion LOAD DATA HIGH
void LDH (int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag) {
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers, listSegments, flag);
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);
    a = (a & 0x0000FFFF) | ((valor & 0xFFFF) << 16);
    writeInOp(registers[OP1],a, mainMemory, registers, listSegments);
}


//Funcion MOV
void MOV(int8_t *mainMemory, int32_t *registers,int32_t listSegments[], int flag){
    int32_t a=getValorOpnd(registers[OP1],mainMemory,registers,listSegments,flag);
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers,listSegments, flag);

    writeInOp(registers[OP1],valor, mainMemory, registers,listSegments);
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
    writeInOp(registers[OP2],resultado, mainMemory, registers, listSegments);
    setCC(OP_NOT,valor,0,resultado,registers);
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
            printf("ERROR: DIVISION POR 0 NO EVALUADA");
            STOP(mainMemory, registers, listSegments, flag);
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
    printf("ERROR CODIGO DE OPERACION INVALIDA");
    STOP(mainMemory, registers, listSegments, flag);
}