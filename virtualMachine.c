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

#define NUM_SEGMENTOS 2

#define H2Bt 0xFFFF0000//Constantes para tomar los 2 bytes más significativos y los 2 menos significativos.
#define L2Bt 0x0000FFFF 


//funcion que verifica si el archivo es valido, devuelve true si lo es y false si no lo es.
// Ademas cierra el archivo en caso de que no sea valido.
int verifyFile(FILE *file) {
    char id[6]={0};
    int8_t version;
    int16_t tamaño;

    if (file == null){
        printf("ARCHIVO INEXISTENTE.");
        return 1
    }
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


//Funcion para obtener la ultima posicion de memoria de un segmento determinado por un registro.
int getSegmentSize(int32_t register, int32_t *listSegment){
    int baseCS = listaSegments[(register >>16 & L2Bt)]>>16 & L2Bt;
    int tamCS = listaSegments[(register >>16 & L2Bt)] & L2Bt;
    return baseCS+tamCS;
}

//Funcion para obtener la direccion fisica a partir de la logica.
int32_t getDir(int32_t logicDir, int32_t listSegments[]){
    int lowByte=logicDir & L2Bt;
    int highByte=(logicDir>>16) & L2Bt;
    if(highByte>=NUM_SEGMENTOS || listSegments[highByte]==-1 ){
        printf("ERROR: INDICE DE SEGMENTO NO VALIDO");
        return -1;
        //error: segmento invalido
    }
    else{
        int base=listSegments[highByte] & H2Bt;
        return (lowByte+base);
    }
}


//Funcion que se encarga de obtener los datos de los operandos dentro del codeSegment.
int32_t getOp(int tipo, int8_t *mainMemory,int rindex){
    int32_t opnd=0;
    for(int i=rindex;i<rindex+tipo;i++){
        opnd=opnd<<8;
        opnd+=mainMemory[i];
    }
    return opnd;
}


//Funcion que obtiene el valor de un operando y lo devuelve
int32_t getValorOpnd(int32_t operando, int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int8_t tipo = (operando >> 24) & 0x000000FF;
    int32_t valor=0;

    switch (tipo) {
        case '1': //operando de registro
            int16_t codReg = registers[OP2] & 0b11111; //obtengo el codigo
            valor = registers[codReg];
            break;
        case '2': //operando inmediato
            valor = registers[OP2] & 0xFFFF;
            break;
        case '3': //operando de memoria;
            int16_t offset = (registers[OP2] >> 8) & L2Bt;
            int8_t codReg = registers[OP2] & 0b11111;
            registers[LAR] = registers[codReg] + offset;
            registers[MAR] = (4 << 16); 
            readFromMemory(registers, mainMemory,listSegments);
            valor = (registers[MBR]);
    }

    return valor;
}


//Funcion que guarda en el operando 1 un valor proveniente de una operacion.
void writeInOp1(int32_t valor, int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int32_t tipo = (registers[OP1] >> 24) & 0X0000000F;
    switch (tipo) {
        case '1': //Operando de registro
            int8_t codReg = registers[OP1] & 0b11111;
            registers[codReg] = valor;
            break;
        case '3': //operando de memoria
            int16_t offset = (registers[OP2] >>8) & L2Bt;
            int8_t codReg = registers[OP2] & 0b11111;
            registers[LAR]=registers[codReg] + offset
            registers[MAR] = (4 << 16); 
            registers[MBR] = valor;
            loadInMemory(registers,mainMemory,listSegments);
    }

}


//Funcion para leer la siguiente instruccion
void readNextInst(int8_t *mainMemory, int32_t *registers, int32_t listSegments[]){
    if registers[IP]!=-1{
        int dir = getDir(registers[IP],listSegments);
        if ((dir != -1) && dir < getSegmentSize(registers[CS], listSegments)){
            int8_t operacion=mainMemory[dir]; //Traigo la operacion del IP
            registers[OPC]= operacion & 0b11111; //Obtengo codigo de operacion
            int32_t tipoa= (operacion >> 4) & 0b11; //tipo operando A
            int32_t tipob= (operacion >> 6) & 0b11;//tipo operando B
            registers[OP1]=(tipoa<<24);
            registers[OP2]=(tipob<<24);
            registers[OP2]+=getOp(tipob,mainMemory,registers[IP]+1);
            registers[OP1]+=getOp(tipoa,mainMemory,registers[IP]+1+tipob);
            registers[IP]+=1+tipoa+tipob;//Sumo el tamaño de la operacion
        }
        else{
            STOP(mainMemory, registers, listSegments);
            break;
        }
    }
    else{
        break;
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
void loadInMemory(int32_t *registers, int8_t mainMemory,int32_t listSegments[]){
    int i;
    fDir = getDir(registers[LAR],listSegments);
    if(fDir != -1){
        int 
        if ((fDir + (registers[MAR]>>16)&L2Bt) < getSegmentSize(registers[LAR], listSegments)){
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
            STOP(mainMemory, registers, listSegments);
            break;
        }
    }
    else{
        break;
    }
}


//Funcion que lee una variable de la memoria y la carga en el MBR dependiendo del MAR. 
void readFromMemory(int32_t *registers, int8_t mainMemory,int32_t listSegments[]){
    int i;
    fDir = getDir(registers[LAR],listSegments);
    if (fDir !=-1){
        if ((fDir + (registers[MAR]>>16)&L2Bt) < getSegmentSize(registers[LAR], listSegments)){
            registers[MAR]+= fDir;
            int opSize = (registers[MAR] >>16) &L2Bt;
            int dir = registers[MAR] & L2Bt;

            for (i=0;i<opSize;i++){
                registers[MBR] = registers[MBR]<<8;
                registers[MBR] += mainMemory[dir+i];
            }
        }
        else{
            STOP(mainMemory, registers, listSegments);
            break;
        }
    }
    else{
        break;
    }
}


//Funcion SYS
void SYS (int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int op=registers[OP2] & 0x00FFFFFF;
    int i;
    char *auxS;
    int32_t aux=registers[ECX]&L2Bt, tam=(registers[ECX]>>16)&L2Bt;
    registers[LAR]=registers[EDX];
    registers[MAR]=(registers[ECX]&H2Bt);

    if(op==1){
        switch (registers[EAX])
        {
        case 1:
            for (i=0;i<aux;i++){
                scanf(" %d",&registers[MBR]);
                loadInMemory(registers,mainMemory,listSegments);
                registers[MAR]+=i*tam;
            }    
        break;
        case 2:
            for (i=0;i<aux;i++){
                    scanf(" %c",&registers[MBR]);
                    loadInMemory(registers,mainMemory,listSegments);
                    registers[MAR]+=i*tam;
            }
        break;
        case 4:
            for (i=0;i<aux;i++){
                scanf(" %o",&registers[MBR]);
                loadInMemory(registers,mainMemory,listSegments);
                registers[MAR]+=i*tam;
            }
        break;
        case 8:
            for (i=0;i<aux;i++){
                scanf(" %x",&registers[MBR]);
                loadInMemory(registers,mainMemory,listSegments);
                registers[MAR]+=i*tam;
            }
        break;
        case 16:
            for (i=0;i<aux;i++){
                scanf(" %s",auxS);
                registers[MBR]=(int32_t)stringToInt(auxS);
                loadInMemory(registers,mainMemory,listSegments);
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
                readFromMemory(registers,mainMemory,listSegments);
                printf(" %d",registers[MBR]);
                registers[MAR]+=i*tam;
            }    
        break;
        case 2:
            for (i=0;i<aux;i++){
                readFromMemory(registers,mainMemory,listSegments);
                    printf(" %c",registers[MBR]);
                    registers[MAR]+=i*tam;
            }
        break;
        case 4:
            for (i=0;i<aux;i++){
                readFromMemory(registers,mainMemory,listSegments);
                printf(" %o",registers[MBR]);
                registers[MAR]+=i*tam;
            }
        break;
        case 8:
            for (i=0;i<aux;i++){
                readFromMemory(registers,mainMemory,listSegments);
                printf(" %x",registers[MBR]);
                registers[MAR]+=i*tam;
            }
        break;
        case 16:
            for (i=0;i<aux;i++){
                readFromMemory(registers,mainMemory,listSegments);
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
void JMP (int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers, listSegments);
    int salto=registers[IP] & 2HBt + valor;

    if (getSegmentSize(registers[IP]) > (salto) && (listSegments[registers[IP]>>16 & L2Bt]>>16 & L2Bt) < salto){
        registers[IP] &= H2Bt + valor;
    }
    else{
        printf("SALTO DEL PAPU!");
        STOP(mainMemory, registers, listSegments);
        break;
    }
}


//Funcion LOAD DATA LOW
void LDL (int8_t *mainMemory, int32_t *registers,int32_t listSegments[]) {
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers,listSegments);
    valor = valor & 0xFFFF;
    
    int8_t tipo = (registers[OP1] >> 24) & 0x000000FF;

    switch (tipo) {
        case '1': 
            int8_t codReg = registers[OP1] & 0b11111;
            registers[codReg] &= valor; //sobrescribe los 2 bits menos significativos
            break;
        case '3': //operando de memoria
            int16_t offset = (registers[OP1] >>8)& L2Bt;
            codReg = registers[OP1] & 0b11111;
            registers[MBR] &= valor;
            registers[LAR] = registers[codReg] + offset;
            registers[MAR] = (4 << 16); 
            loadInMemory(registers,mainMemory,listSegments);
    }
}


//Funcion LOAD DATA HIGH
void LDH (int8_t *mainMemory, int32_t *registers,int32_t listSegments[]) {
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers,listSegments);
    valor = valor & 0xFFFF;
    
    int32_t tipo = (registers[OP1] >> 24)&0x000000FF;

    switch (tipo) {
        case '1': 
            int8_t codReg = registers[OP1] & 0b11111;
            registers[codReg] &= (valor<<16) + 0xFFFF; //sobrescribe los 2 bits menos significativos
            break;
        case '3': //operando de memoria
            int16_t offset = (registers[OP1] >> 8)&L2Bt;
            codReg = registers[OP1] & 0b11111;
            registers[MBR] &= (valor<<16) + 0xFFFF;
            registers[LAR] = registers[codReg] + offset;
            registers[MAR] = (4 << 16); 
            loadInMemory(registers,mainMemory,listSegments);
    }

}


//Funcion MOV
void MOV(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers,listSegments);

    int32_t tipo = (registers[OP1] >> 24)&0x000000FF;
    writeInOp1(valor, mainMemory, registers,listSegments);
    setCC(OP_MOV,0,0,valor,registers);
}

//FUNCIONES JUMP CONDICIONALES --------------------- !!
void JN(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    if (!((registers[CC] & 0x80000000)>>31)){
        JMP(mainMemory,registers,listSegments);
}
}


void JP(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    if (((registers[CC] & 0xC0000000)>>30) == 0){
        JMP(mainMemory,registers,listSegments);
    }
}


void JZ(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    if (!((registers[CC] & 0x40000000)>>30)){
        JMP(mainMemory,registers,listSegments);
    }
}


void JC(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    if (!((registers[CC] & 0x20000000)>>29)){
        JMP(mainMemory,registers,listSegments);
    }
}


void JV(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    if (!((registers[CC] & 0x10000000)>>28)){
        JMP(mainMemory,registers,listSegments);
    }
}


void JNP(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    if ((!((registers[CC] & 0x80000000)>>31)) || (!((registers[CC] & 0x40000000)>>30))){
        JMP(mainMemory,registers,listSegments);
    }
}


void JNN(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    if ((registers[CC] & 0x80000000)>>31){
        JMP(mainMemory,registers,listSegments);
    }
}


void JNZ(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    if ((registers[CC] & 0x40000000)>>30){
        JMP(mainMemory,registers,listSegments);
    }
}

// FIN DE LAS OPERACIONES JMP ------------------------ !!

//Funcion NOT
void NOT(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int32_t valor = getValorOpnd(registers[OP2], mainMemory, registers, listSegments);
    valor = ~valor;
    writeInOp1(valor, mainMemory, registers, listSegments);
    setCC(OP_NOT,0,0,valor,registers);
}


//Funcion ADD
void ADD(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments), b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments);

    valor = a + b; //*a = *a +b

    writeInOp1(valor, mainMemory, registers,listSegments);

    setCC(OP_ADD,a,b,valor,registers);
}

//Funcion SUB
void SUB(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments), b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments);

    valor = a - b;

    writeInOp1(valor, mainMemory, registers,listSegments);

    setCC(OP_SUB,a,b,valor,registers);
}

//Funcion MUL
void MUL(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments), b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments);

    valor = a * b;

    writeInOp1(valor, mainMemory, registers,listSegments);

    setCC(OP_MUL,a,b,valor,registers);
}

//Funcion DIV
void DIV(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments), b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments);
    if(b==0){
        printf("ERROR: DIVISION POR 0 NO EVALUADA");
        STOP(mainMemory, registers, listSegments);
        break;
    }
    else{

        valor = a / b;

        registers[AC]=a%b; //guardo resto de division entera en AC

        writeInOp1(valor, mainMemory, registers,listSegments);
        

        setCC(OP_DIV,a,b,valor,registers);
    }
}


//Funcion XOR
void XOR(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments), b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments);

    valor = a ^ b;

    writeInOp1(valor, mainMemory, registers,listSegments);

    setCC(OP_XOR,a,b,valor,registers);
}


//Funcion SWAP
void SWAP(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments), b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments);

}

//---Shifters---


//Funcion SHL
void SHL(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    uint32_t valor;
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments), b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments);
    valor=a<<b;
    writeInOp1(valor, mainMemory, registers,listSegments);
    setCC(OP_SHL,a,b,valor,registers);

}


//Funcion SHR
void SHR(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    uint32_t valor; //al ser unsigned, el shift agrega 0 por izquierda
    int32_t a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments), b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments);
    valor=(uint32_t)a>>b;
    writeInOp1(valor, mainMemory, registers,listSegments);
    setCC(OP_SHR,a,b,valor,registers);

}


//Funcion SAR
void SAR(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments), b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments);
    valor=a>>b; //como aca valor es signed, propaga el signo normalmente
    writeInOp1(valor, mainMemory, registers,listSegments);
    setCC(OP_SAR,a,b,valor,registers);

}

//Fin Shifters

//Funcion RND
void RND(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments), b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments);
    valor=rand() % (b+1);
    writeInOp1(valor, mainMemory, registers,listSegments);

}

//Funcion CMP
void CMP(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments), b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments);
    valor=a-b;
    setCC(OP_CMP,a,b,valor,registers);


}

//Funcion AND
void AND(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments), b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments);

    valor = a & b;

    writeInOp1(valor, mainMemory, registers,listSegments);

    setCC(OP_AND,a,b,valor,registers);

}

//Funcion OR
void OR(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    int32_t valor, a = getValorOpnd(registers[OP1], mainMemory, registers,listSegments), b = getValorOpnd(registers[OP2], mainMemory, registers,listSegments);

    valor = a | b;

    writeInOp1(valor, mainMemory, registers,listSegments);

    setCC(OP_OR,a,b,valor,registers);

}

//Funcion STOP
void STOP(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    registers[IP] = -1;
}


//Funcion invalida, tira error.
void INVALID(int8_t *mainMemory, int32_t *registers,int32_t listSegments[]){
    printf("ERROR OPERACION INVALIDA");
    STOP(mainMemory, registers, listSegments);
}

void main(int argc,char* argv[]){
    srand(time(NULL)); //inicializa semilla al iniciar programa
    int16_t cSize = 0;
    int32_t registers[32]={0}; //serian los 32 registros de 4 bytes que se piden (aunque solo se usen 17 por ahora)
    int32_t listSegments[8]={0}; //serian los 8 segmentos de 4 bytes que se piden
    int8_t mainMemory[16384]={0}; //seria la memoria principal de 16kib

    //VECTORES A FUNCIONES 0, 1 y 2 operandos.
    void (*op[32])(int8_t *mainMemory, int32_t *registers,int32_t listSegments[])={SYS, JMP, JP, JN, JZ, JC, JV, JNP,JNN, JNZ, NOT, INVALID, INVALID, INVALID, INVALID, STOP, MOV, ADD, SUB, MUL, DIV, CMP, AND, OR, XOR, SWAP, SHL, SHR, SAR, LDL, LDH, RND};

    FILE * arch = fopen(argv[0], "rb");

    if (!verifyFile(arch)){
        printf("ARCHIVO INVALIDO");
    }
    else{
        //Se cargan las variables del archivo y se setean los arrays.
        loadCodeSize(arch, &cSize);
        loadCode(arch, cSize, mainMemory);
        setRegisters(registers,cSize);
        setSegments(listSegments, cSize);
        
        while (registers[IP] != -1){
            readNextInst(mainMemory, registers, listSegments);
            if (registers[OPC]<=31 && registers[OPC]>0){
                op[registers[OPC]](mainMemory, registers, listSegments);
            }
            else{
                printf("ERROR OPERACION INVALIDA");
                registers[IP]=-1;
            }
        }
        printf("Fin de proceso.");
        fclose(arch);
    }
}