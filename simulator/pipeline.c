#include<stdio.h>
#include<stdlib.h>

#define Iimage "iimage.bin"
#define Dimage "dimage.bin"
#define ERROR "error_dump.rpt"
#define SNAPSHOT "snapshot.rpt"

#define op_func  0x00

#define op_addi  0x08
#define op_lw    0x23
#define op_lh    0x21
#define op_lhu   0x25
#define op_lb    0x20
#define op_lbu   0x24
#define op_sw    0x2B
#define op_sh    0x29
#define op_sb    0x28
#define op_lui   0x0F
#define op_andi  0x0C
#define op_ori   0x0D
#define op_nori  0x0E
#define op_slti  0x0A
#define op_beq   0x04
#define op_bne   0x05

#define op_j     0x02
#define op_jal   0x03

#define op_halt  0x3F

#define func_add  0x20
#define func_sub  0x22
#define func_and  0x24
#define func_or   0x25
#define func_xor  0x26
#define func_nor  0x27
#define func_nand 0x28
#define func_slt  0x2A
#define func_sll  0x00
#define func_srl  0x02
#define func_sra  0x03
#define func_jr   0x08

#define STALL 0x00000000;

typedef unsigned int u32;
typedef unsigned char u8;

struct S_IF_Branch{
    int PCSel;
    u32 branch_PC;
} IF_Branch;

struct S_IF_ID{
    u32 instr;
    u32 next_PC;
} IF_ID,tmp_IF_ID;

struct S_ID_EXE{
    
    u32 instr;
    
    u32 read_data_1;
    u32 read_data_2;
    u32 instr_offset;
    int instr_Rs;
    int instr_Rt;
    int instr_Rd;
    
    //control signal
    int ctrl_ExtOp;
    int ctrl_ALUSrc;
    int ctrl_ExtFunc; //func code
    int ctrl_RegDst;
    
    int ctrl_MemW;
    int ctrl_MemR;
    
    int ctrl_MemtoReg;
    int ctrl_RegWr;
}ID_EXE,tmp_ID_EXE;

struct S_EXE_MEM{
    
    u32 instr;
    
    u32 ALU_output;
    int write_reg;
    u32 write_data;
    //control signal
    int ctrl_MemW;
    int ctrl_MemR;
    
    int ctrl_MemtoReg;
    int ctrl_RegWr;
    
    
}EXE_MEM,tmp_EXE_MEM;

struct S_MEM_WB{
    
    u32 instr;
    
    u32 DMem_output;
    u32 ALU_output;
    int write_reg;
    
    //control signal
    int ctrl_MemtoReg;
    int ctrl_RegWr;
}MEM_WB,tmp_MEM_WB;

//global variable
FILE *snapShot,*error;

int n_cycle=0,halt=0;
int status[10];
int error_state[6];
u32 reg[32];
int instr_num=0,mem_num=0;
u32 PC,JAL_PC;
u8 D_Memory[1100],I_Memory[1100];


void initial();
void load_iimage();
void load_dimage();
int getSlice(u32,int,int);
void updateBuffer();
void printSnapShot();
void stall();
void flush();
void IF();
void ID();
void EXE();
void MEM();
void WB();
void LMemory(u32,int,int);
void SMemory(u32,int,u32);
u32 sign_extend(u32);
char* OPtoMIPS(u32);
char* printStatus(int);
void printStage();
void printError();
u32 ALU_add();
u32 ALU_and();
u32 ALU_andi();
u32 ALU_or();
u32 ALU_ori();
u32 ALU_sll();
u32 ALU_slt();
u32 ALU_sra();
u32 ALU_srl();
u32 ALU_xor();
u32 ALU_lui();

int main()
{
    int i;
    snapShot=fopen(SNAPSHOT,"w");
    error=fopen(ERROR,"w");
    initial();
    load_iimage();
    load_dimage();
    
    while (halt!=1)
    {
        //printf("[%d]\n",n_cycle);
        /*if (n_cycle==169)
         break;*/
        printSnapShot();
        if(n_cycle==251){
            printf("1");
        }
        IF();
        WB();
        ID();
        EXE();
        MEM();
        
        printStage();
        
        n_cycle++;
        printError();
        
        
        PC = (IF_Branch.PCSel==1) ? PC + 4 : IF_Branch.branch_PC;
        //printf("PC[%d] = 0x%08X\n",IF_Branch.PCSel,PC);
        updateBuffer();
        for (i=0;i<10;i++)
            status[i]=0;
        for (i=0;i<6;i++)
            error_state[i]=0;
    }
    
    return 0;
}

void updateBuffer()
{
    EXE_MEM.ALU_output=tmp_EXE_MEM.ALU_output;
    EXE_MEM.ctrl_MemR=tmp_EXE_MEM.ctrl_MemR;
    EXE_MEM.ctrl_MemtoReg=tmp_EXE_MEM.ctrl_MemtoReg;
    EXE_MEM.ctrl_MemW=tmp_EXE_MEM.ctrl_MemW;
    EXE_MEM.ctrl_RegWr=tmp_EXE_MEM.ctrl_RegWr;
    EXE_MEM.instr=tmp_EXE_MEM.instr;
    EXE_MEM.write_data=tmp_EXE_MEM.write_data;
    EXE_MEM.write_reg=tmp_EXE_MEM.write_reg;
    
    ID_EXE.ctrl_ALUSrc=tmp_ID_EXE.ctrl_ALUSrc;
    ID_EXE.ctrl_ExtFunc=tmp_ID_EXE.ctrl_ExtFunc;
    ID_EXE.ctrl_ExtOp=tmp_ID_EXE.ctrl_ExtOp;
    ID_EXE.ctrl_MemR=tmp_ID_EXE.ctrl_MemR;
    ID_EXE.ctrl_MemtoReg=tmp_ID_EXE.ctrl_MemtoReg;
    ID_EXE.ctrl_MemW=tmp_ID_EXE.ctrl_MemW;
    ID_EXE.ctrl_RegDst=tmp_ID_EXE.ctrl_RegDst;
    ID_EXE.ctrl_RegWr=tmp_ID_EXE.ctrl_RegWr;
    ID_EXE.instr=tmp_ID_EXE.instr;
    ID_EXE.instr_offset=tmp_ID_EXE.instr_offset;
    ID_EXE.instr_Rd=tmp_ID_EXE.instr_Rd;
    ID_EXE.instr_Rs=tmp_ID_EXE.instr_Rs;
    ID_EXE.instr_Rt=tmp_ID_EXE.instr_Rt;
    ID_EXE.read_data_1=tmp_ID_EXE.read_data_1;
    ID_EXE.read_data_2=tmp_ID_EXE.read_data_2;
    
    IF_ID.instr=tmp_IF_ID.instr;
    IF_ID.next_PC=tmp_IF_ID.next_PC;
    
    MEM_WB.ALU_output=tmp_MEM_WB.ALU_output;
    MEM_WB.ctrl_MemtoReg=tmp_MEM_WB.ctrl_MemtoReg;
    MEM_WB.ctrl_RegWr=tmp_MEM_WB.ctrl_RegWr;
    MEM_WB.DMem_output=tmp_MEM_WB.DMem_output;
    MEM_WB.instr=tmp_MEM_WB.instr;
    MEM_WB.write_reg=tmp_MEM_WB.write_reg;
}

void printSnapShot()
{
    
    int i=0;
    fprintf(snapShot,"cycle %d\n",n_cycle);
    if (n_cycle==165)
        printf("[165] 0x%08X\n",reg[22]);
    for (i=0;i<32;i++)
    {
        fprintf(snapShot,"$%02d: 0x%08X\n",i,reg[i]);
    }
    fprintf(snapShot,"PC: 0x%08X\n",PC);
}

void printStage()
{
    int i;
    u32 instrction=0;
    for (i=0;i<=3;i++)
    {
        instrction=instrction<<8;
        instrction=instrction|I_Memory[PC+i];
    }
    
    fprintf(snapShot,"IF: 0x%08X%s%s\n",instrction,printStatus(0),printStatus(1));
    fprintf(snapShot,"ID: %s%s%s%s\n",OPtoMIPS(IF_ID.instr),printStatus(2),printStatus(3),printStatus(4));
    fprintf(snapShot,"EX: %s%s%s%s%s\n",OPtoMIPS(ID_EXE.instr),printStatus(5),printStatus(7),printStatus(6),printStatus(8));
    fprintf(snapShot,"DM: %s\n",OPtoMIPS(EXE_MEM.instr));
    fprintf(snapShot,"WB: %s\n",OPtoMIPS(MEM_WB.instr));
    fprintf(snapShot,"\n\n");
}

char * printStatus(int index)
{
    char* str=malloc(512);
    if (index==0 && status[0]!=0)
        return " to_be_stalled";
    if (index==1 && status[1]!=0)
        return " to_be_flushed";
    if (index==2 && status[2]!=0)
        return " to_be_stalled";
    if (index==3 && status[3]!=0)
    {
        sprintf(str," fwd_EX-DM_rs_$%d",status[3]);
        return str;
    }
    if (index==4 && status[4]!=0)
    {
        sprintf(str," fwd_EX-DM_rt_$%d",status[4]);
        return str;
    }
    if (index==5 && status[5]!=0)
    {
        sprintf(str," fwd_EX-DM_rs_$%d",status[5]);
        return str;
    }
    if (index==6 && status[6]!=0)
    {
        sprintf(str," fwd_EX-DM_rt_$%d",status[6]);
        return str;
    }
    if (index==7 && status[7]!=0)
    {
        sprintf(str," fwd_DM-WB_rs_$%d",status[7]);
        return str;
    }
    if (index==8 && status[8]!=0)
    {
        sprintf(str," fwd_DM-WB_rt_$%d",status[8]);
        return str;
    }
    return "\0";
}

char* OPtoMIPS(u32 instr)
{
    int opcode=getSlice(instr,31,26);
    int func = getSlice(instr,5,0);
    if (instr==0x00000000)
        return "NOP";
    if (opcode==op_func)
    {
        if (func==func_add)
            return "ADD";
        if (func==func_and)
            return "AND";
        if (func==func_jr)
            return "JR";
        if (func==func_nand)
            return "NAND";
        if (func==func_nor)
            return "NOR";
        if (func==func_or)
            return "OR";
        if (func==func_sll)
            return "SLL";
        if (func==func_slt)
            return "SLT";
        if (func==func_sra)
            return "SRA";
        if (func==func_srl)
            return "SRL";
        if (func==func_sub)
            return "SUB";
        if (func==func_xor)
            return "XOR";
    }
    if (opcode==op_addi)
        return "ADDI";
    if (opcode==op_andi)
        return "ANDI";
    if (opcode==op_beq)
        return "BEQ";
    if (opcode==op_bne)
        return "BNE";
    if (opcode==op_halt)
        return "HALT";
    if (opcode==op_j)
        return "J";
    if (opcode==op_jal)
        return "JAL";
    if (opcode==op_lb)
        return "LB";
    if (opcode==op_lbu)
        return "LBU";
    if (opcode==op_lh)
        return "LH";
    if (opcode==op_lhu)
        return "LHU";
    if (opcode==op_lui)
        return "LUI";
    if (opcode==op_lw)
        return "LW";
    if (opcode==op_nori)
        return "NORI";
    if (opcode==op_ori)
        return "ORI";
    if (opcode==op_sb)
        return "SB";
    if (opcode==op_sh)
        return "SH";
    if (opcode==op_slti)
        return "SLTI";
    if (opcode==op_sw)
        return "SW";
    
    return "ERROR";
}

void printError()
{
    if (error_state[0]==1)
        fprintf(error,"In cycle %d: Write $0 Error\n",n_cycle);
    if (error_state[1]==1)
        fprintf(error,"In cycle %d: Address Overflow\n",n_cycle);
    if (error_state[2]==1)
        fprintf(error,"In cycle %d: Misalignment Error\n",n_cycle);
    if (error_state[3]==1)
        fprintf(error,"In cycle %d: Number Overflow\n",n_cycle);
    if (error_state[4]==1&&error_state[1]!=1)
        fprintf(error,"In cycle %d: Address Overflow\n",n_cycle);
    if (error_state[5]==1&&error_state[2]!=1)
        fprintf(error,"In cycle %d: Misalignment Error\n",n_cycle);
}

void initial()
{
    int i=0;
    PC=0;
    for (i=0;i<32;i++)
        reg[i]=0;
    for (i=0;i<1100;i++)
    {
        I_Memory[i]=0;
        D_Memory[i]=0;
    }
}

void control(u32 instruction)
{
    int opcode=getSlice(instruction,31,26);
    int func=getSlice(instruction,5,0);
    tmp_ID_EXE.ctrl_ExtOp    = opcode;
    tmp_ID_EXE.ctrl_ExtFunc  = func;
    if (instruction==0x00000000)
    {
        tmp_ID_EXE.ctrl_ALUSrc   = 0;
        tmp_ID_EXE.ctrl_RegDst   = 0;
        tmp_ID_EXE.ctrl_MemW     = 0;
        tmp_ID_EXE.ctrl_MemR     = 0;
        tmp_ID_EXE.ctrl_MemtoReg = 0;
        tmp_ID_EXE.ctrl_RegWr    = 0;
    }
    else if(opcode==0x00&&func!=func_jr)
    {
        tmp_ID_EXE.ctrl_ALUSrc   = 0;
        tmp_ID_EXE.ctrl_RegDst   = 1;
        tmp_ID_EXE.ctrl_MemW     = 0;
        tmp_ID_EXE.ctrl_MemR     = 0;
        tmp_ID_EXE.ctrl_MemtoReg = 1;//
        tmp_ID_EXE.ctrl_RegWr    = 1;
    }
    else if (opcode==op_lui||opcode==op_addi||opcode==op_andi||opcode==op_ori||opcode==op_nori||opcode==op_slti)
    {
        tmp_ID_EXE.ctrl_ALUSrc   = 1;
        tmp_ID_EXE.ctrl_RegDst   = 0;
        
        tmp_ID_EXE.ctrl_MemW     = 0;
        tmp_ID_EXE.ctrl_MemR     = 0;
        
        tmp_ID_EXE.ctrl_MemtoReg = 1;//
        tmp_ID_EXE.ctrl_RegWr    = 1;
    }
    else if(opcode==op_lw || opcode==op_lhu || opcode==op_lh || opcode==op_lbu || opcode==op_lb)
    {
        tmp_ID_EXE.ctrl_ALUSrc   = 1;
        tmp_ID_EXE.ctrl_RegDst   = 0;
        
        tmp_ID_EXE.ctrl_MemW     = 0;
        tmp_ID_EXE.ctrl_MemR     = 1;
        
        tmp_ID_EXE.ctrl_MemtoReg = 0;//
        tmp_ID_EXE.ctrl_RegWr    = 1;
    }
    else if(opcode==op_sw || opcode==op_sh || opcode==op_sb)
    {
        tmp_ID_EXE.ctrl_ALUSrc   = 1;
        //tmp_ID_EXE.ctrl_RegDst   = ;
        
        tmp_ID_EXE.ctrl_MemW     = 1;
        tmp_ID_EXE.ctrl_MemR     = 0;
        
        //tmp_ID_EXE.ctrl_MemtoReg = ;
        tmp_ID_EXE.ctrl_RegWr    = 0;
    }
    else if(opcode==op_beq || opcode==op_bne||opcode==op_j||(opcode==0x00&&func==func_jr))
    {
        tmp_ID_EXE.ctrl_ALUSrc   = 0;// don't care
        //tmp_ID_EXE.ctrl_RegDst   = ;
        
        tmp_ID_EXE.ctrl_MemW     = 0;
        tmp_ID_EXE.ctrl_MemR     = 0;
        
        //tmp_ID_EXE.ctrl_MemtoReg = ;
        tmp_ID_EXE.ctrl_RegWr    = 0;
    }
    else if(opcode==op_jal)
    {
        tmp_ID_EXE.ctrl_ALUSrc   = 1; //offset
        tmp_ID_EXE.ctrl_RegDst   = 1; //rd
        tmp_ID_EXE.ctrl_MemW     = 0;
        tmp_ID_EXE.ctrl_MemR     = 0;
        tmp_ID_EXE.ctrl_MemtoReg = 1;
        tmp_ID_EXE.ctrl_RegWr    = 1;
    }
}

void IF()
{
    //printf("IF > ");
    int i=0;
    u32 instrction=0;
    
    if (PC%4!=0)
    {
        error_state[5]=1;
        halt=1;
        return;
    }
    
        if ((signed)(PC+i)<0||(signed)(PC+i)>1023)
        {
            printf("nc=%d",n_cycle);
            printf("ERROR => %d\n",PC+i);
            error_state[4]=1;
            halt=1;
            return;
        }
        else
            instrction=(I_Memory[PC]<<24)+((I_Memory[PC+1]<<16) & 0xff0000)+((I_Memory[PC+2]<<8) & 0xff00)+(I_Memory[PC+3] & 0xff);
    
    //printf("[%d] IF = 0x%08X...\n",n_cycle,instrction);
    tmp_IF_ID.instr = instrction;
    tmp_IF_ID.next_PC = PC + 4;
}

void ID()
{
    //printf("ID > ");
    int opcode = getSlice(IF_ID.instr,31,26);
    int func=getSlice(IF_ID.instr,5,0);
    int _RS=getSlice(IF_ID.instr, 25, 21);
    int _RT=getSlice(IF_ID.instr, 20, 16);
    int _RD=getSlice(IF_ID.instr, 15, 11);
    
    if (opcode==op_func&&func==func_jr)
    {
        _RT=0;
        _RD=0;
    }
    else if (opcode==op_j)
    {
        _RS=0;
        _RT=0;
        _RD=0;
    }
    else if (opcode==op_jal)
    {
        _RS=0;
        _RT=0;
        _RD=31;
    }
    
    if (opcode==op_lui||(opcode==op_func&&func==func_sll)||(opcode==op_func&&func==func_srl)||(opcode==op_func&&func==func_sra))
    {
        _RS=0;
    }
    
    //printf("[%d] ID = 0x%08X...\n",n_cycle,IF_ID.instr);
    control(IF_ID.instr);
    tmp_ID_EXE.instr=IF_ID.instr;
    tmp_ID_EXE.read_data_1 = reg[_RS];
    tmp_ID_EXE.read_data_2 = reg[_RT];
    tmp_ID_EXE.instr_offset = sign_extend(getSlice(IF_ID.instr, 15, 0));
    tmp_ID_EXE.instr_Rs = _RS;
    tmp_ID_EXE.instr_Rt = _RT;
    tmp_ID_EXE.instr_Rd = _RD;
    
    if (IF_ID.instr==0x00000000)
    {
        IF_Branch.PCSel=1;
        tmp_ID_EXE.instr=IF_ID.instr;
        return;
    }
    if (IF_ID.instr==0xFFFFFFFF)
    {
        IF_Branch.PCSel=1;
        tmp_ID_EXE.instr=IF_ID.instr;
        return;
    }
    
    if (ID_EXE.ctrl_MemR==1 &&ID_EXE.ctrl_RegDst==0&& ID_EXE.instr_Rt!=0 && ID_EXE.instr_Rt == _RS)
    {
        stall();
        return;
    }
    else if (ID_EXE.ctrl_MemR==1 &&ID_EXE.ctrl_RegDst==0&& ID_EXE.instr_Rt!=0 && ID_EXE.instr_Rt == _RT&&tmp_ID_EXE.ctrl_ALUSrc==0)
    {
        stall();
        return;
    }
    // sw forwarding
    else if (ID_EXE.ctrl_MemR==1 &&ID_EXE.ctrl_RegDst==0&& ID_EXE.instr_Rt!=0 && ID_EXE.instr_Rt == _RT&&tmp_ID_EXE.ctrl_MemW==1)
    {
        stall();
        return;
    }
    else if (opcode==op_beq||opcode==op_bne)
    {
        int write_reg=(ID_EXE.ctrl_RegDst==0)?ID_EXE.instr_Rt:ID_EXE.instr_Rd;
        if (ID_EXE.ctrl_RegWr == 1 && write_reg!=0 && (write_reg==_RS||write_reg==_RT))
        {
            stall();
            return;
        }
        else if (EXE_MEM.ctrl_MemR==1 && EXE_MEM.ctrl_RegWr==1 && EXE_MEM.write_reg!=0&&(EXE_MEM.write_reg==_RS||EXE_MEM.write_reg==_RT))
        {
            stall();
            return;
        }
    }
    else if (opcode==op_func&&func==func_jr)
    {
        int write_reg=(ID_EXE.ctrl_RegDst==0)?ID_EXE.instr_Rt:ID_EXE.instr_Rd;
        if (ID_EXE.ctrl_RegWr == 1 && write_reg!=0 && write_reg==_RS)
        {
            stall();
            return;
        }
        else if (EXE_MEM.ctrl_MemR==1 && EXE_MEM.ctrl_RegWr==1 && EXE_MEM.write_reg!=0&&EXE_MEM.write_reg==_RS)
        {
            stall();
            return;
        }
    }
    
    if ((opcode==op_beq||opcode==op_bne)&&EXE_MEM.ctrl_MemR==0 && EXE_MEM.ctrl_RegWr==1 && EXE_MEM.write_reg != 0 && EXE_MEM.write_reg == _RS)
    {
        status[3]=_RS;
        /*reg[_RS]=EXE_MEM.ALU_output;
         tmp_ID_EXE.read_data_1=reg[_RS];*/
        //can't modify reg
        tmp_ID_EXE.read_data_1=EXE_MEM.ALU_output;
    }
    if ((opcode==op_beq||opcode==op_bne)&&EXE_MEM.ctrl_MemR==0 && EXE_MEM.ctrl_RegWr==1 && EXE_MEM.write_reg != 0 && EXE_MEM.write_reg == _RT)
    {
        status[4]=_RT;
        /*reg[_RT]=EXE_MEM.ALU_output;
         tmp_ID_EXE.read_data_2=reg[_RT];*/
        tmp_ID_EXE.read_data_2=EXE_MEM.ALU_output;
    }
    
    if ((opcode==op_func&&func==func_jr)&&EXE_MEM.ctrl_MemR==0 && EXE_MEM.ctrl_RegWr==1 && EXE_MEM.write_reg != 0 && EXE_MEM.write_reg == _RS)
    {
        status[3]=_RS;
        tmp_ID_EXE.read_data_1=EXE_MEM.ALU_output;
    }
    
    if (opcode==op_beq)
    {
        IF_Branch.branch_PC = ALU_add(IF_ID.next_PC,(sign_extend(getSlice(IF_ID.instr, 15, 0))<<2));
        if (tmp_ID_EXE.read_data_1==tmp_ID_EXE.read_data_2)
        {
            flush();
        }
        else
            IF_Branch.PCSel=1;
    }
    else if (opcode==op_bne)
    {
        IF_Branch.branch_PC = ALU_add(IF_ID.next_PC,(sign_extend(getSlice(IF_ID.instr, 15, 0))<<2));
        if (tmp_ID_EXE.read_data_1!=tmp_ID_EXE.read_data_2)
        {
            flush();
        }
        else
            IF_Branch.PCSel=1;
    }
    else if (opcode==op_func&&getSlice(IF_ID.instr,5,0)==func_jr)
    {
        //printf("JR flush");
        IF_Branch.branch_PC = tmp_ID_EXE.read_data_1;
        flush();
    }
    else if (opcode==op_j||opcode==op_jal)
    {
        //printf("(J)");
        IF_Branch.branch_PC = (IF_ID.next_PC>>28<<28)|(getSlice(IF_ID.instr, 25, 0)<<2);
        flush();
        if (opcode==op_jal)
        {
            JAL_PC=IF_ID.next_PC;
            //printf("[JAL] PC+4= 0x%08X\n",JAL_PC);
        }
    }
    else
        IF_Branch.PCSel=1;
}

void flush()
{
    //printf("flush...\n");
    status[1]=1;
    tmp_IF_ID.instr=0x00000000;
    tmp_IF_ID.instr=0x00000000;
    tmp_IF_ID.next_PC=0;
    IF_Branch.PCSel=0;
    // control set to 0?
}

void stall()
{
    //printf("stall...\n");
    status[0]=1;
    status[2]=1;
    tmp_ID_EXE.instr= 0x00000000;
    tmp_ID_EXE.ctrl_ALUSrc   = 0;
    tmp_ID_EXE.ctrl_RegDst   = 0;
    tmp_ID_EXE.ctrl_MemW     = 0;
    tmp_ID_EXE.ctrl_MemR     = 0;
    tmp_ID_EXE.ctrl_MemtoReg = 0;
    tmp_ID_EXE.ctrl_RegWr    = 0;
    tmp_IF_ID.instr=IF_ID.instr;
    tmp_IF_ID.next_PC=IF_ID.next_PC;
    IF_Branch.branch_PC=IF_ID.next_PC;
    //printf("PC will go to = 0x%08X\n",IF_Branch.branch_PC);
    IF_Branch.PCSel=0;
}

u32 sign_extend(u32 instr_offset)
{
    if(instr_offset>>15 == 1)
        instr_offset = instr_offset | 0xffff0000;
    return instr_offset;
}

void EXE()
{
    //printf("EX > ");
    u32 data1 = ID_EXE.read_data_1;
    u32 data2 = (ID_EXE.ctrl_ALUSrc==0)?ID_EXE.read_data_2:ID_EXE.instr_offset;
    int not_branch=1,opcode=getSlice(ID_EXE.instr,31,26);
    int shamt=(ID_EXE.instr_offset&0x000007FF)>>6;
    // sll srl sra
    
    
    tmp_EXE_MEM.ctrl_MemR=ID_EXE.ctrl_MemR;
    tmp_EXE_MEM.ctrl_MemtoReg=ID_EXE.ctrl_MemtoReg;
    tmp_EXE_MEM.ctrl_MemW=ID_EXE.ctrl_MemW;
    tmp_EXE_MEM.ctrl_RegWr=ID_EXE.ctrl_RegWr;
    tmp_EXE_MEM.write_reg=(ID_EXE.ctrl_RegDst==0)?ID_EXE.instr_Rt:ID_EXE.instr_Rd;
    tmp_EXE_MEM.write_data=ID_EXE.read_data_2;
    tmp_EXE_MEM.instr=ID_EXE.instr;
    if (ID_EXE.instr==0x00000000||ID_EXE.instr==0xFFFFFFFF)
    {
        tmp_EXE_MEM.instr=ID_EXE.instr;
        return;
    }
    
    if (opcode==op_beq||opcode==op_bne||(ID_EXE.ctrl_ExtOp==op_func&&ID_EXE.ctrl_ExtFunc==func_jr))
        not_branch=0;
    else
        not_branch=1;
    
    if (not_branch==1 && EXE_MEM.ctrl_MemR==0 &&EXE_MEM.ctrl_RegWr==1 && EXE_MEM.write_reg!=0 && EXE_MEM.write_reg==ID_EXE.instr_Rs)
    {
        //printf("5_EXE_MEM.ctrl_RegWr = %d\n",EXE_MEM.ctrl_RegWr);
        status[5]=ID_EXE.instr_Rs;
        data1=EXE_MEM.ALU_output;
    }
    else if (not_branch==1 && MEM_WB.ctrl_RegWr==1 && MEM_WB.write_reg!=0 && MEM_WB.write_reg==ID_EXE.instr_Rs)
    {
        //printf("7_MEM_WB.ctrl_RegWr = %d\n",MEM_WB.ctrl_RegWr);
        status[7]=ID_EXE.instr_Rs;
        data1=MEM_WB.ctrl_MemtoReg ? MEM_WB.ALU_output : MEM_WB.DMem_output;
    }
    
    if (not_branch==1 && EXE_MEM.ctrl_MemR==0 && EXE_MEM.ctrl_RegWr==1 && EXE_MEM.write_reg!=0 && EXE_MEM.write_reg==ID_EXE.instr_Rt && (ID_EXE.ctrl_ALUSrc==0||ID_EXE.ctrl_MemW==1))
    {
        //printf("6_EXE_MEM.ctrl_RegWr = %d\n",EXE_MEM.ctrl_RegWr);
        status[6]=ID_EXE.instr_Rt;
        if (ID_EXE.ctrl_ALUSrc==0)
            data2=EXE_MEM.ALU_output;
        else if (ID_EXE.ctrl_MemW==1)
            tmp_EXE_MEM.write_data=EXE_MEM.ALU_output;
        
    }
    else if (not_branch==1 && MEM_WB.ctrl_RegWr==1 && MEM_WB.write_reg!=0 && MEM_WB.write_reg==ID_EXE.instr_Rt && (ID_EXE.ctrl_ALUSrc==0||ID_EXE.ctrl_MemW==1))
    {
        status[8]=ID_EXE.instr_Rt;
        if (ID_EXE.ctrl_ALUSrc==0)
            data2=MEM_WB.ctrl_MemtoReg ? MEM_WB.ALU_output : MEM_WB.DMem_output;
        else if (ID_EXE.ctrl_MemW==1)
            tmp_EXE_MEM.write_data=MEM_WB.ctrl_MemtoReg ? MEM_WB.ALU_output : MEM_WB.DMem_output;
    }
    
    if (ID_EXE.ctrl_MemR==1||ID_EXE.ctrl_MemW==1)// address
    {
        tmp_EXE_MEM.ALU_output=ALU_add(data1,data2);
    }
    else if (ID_EXE.ctrl_ExtOp==op_addi)
        tmp_EXE_MEM.ALU_output=ALU_add(data1,data2);
    else if (ID_EXE.ctrl_ExtOp==op_func&&ID_EXE.ctrl_ExtFunc==func_add)
        tmp_EXE_MEM.ALU_output=ALU_add(data1,data2);
    else if (ID_EXE.ctrl_ExtOp==op_func&&ID_EXE.ctrl_ExtFunc==func_sub)
    {
//        if (data2==0x80000000)
//            error_state[3]=1;
//        tmp_EXE_MEM.ALU_output=ALU_add(data1,~data2+1);
        tmp_EXE_MEM.ALU_output=ALU_add(data1,-data2);
    }
    else if (ID_EXE.ctrl_ExtOp==op_andi)
        tmp_EXE_MEM.ALU_output=ALU_andi(data1,data2);
    else if (ID_EXE.ctrl_ExtOp==op_func&&ID_EXE.ctrl_ExtFunc==func_and)
        tmp_EXE_MEM.ALU_output=ALU_and(data1,data2);
    else if (ID_EXE.ctrl_ExtOp==op_ori)
        tmp_EXE_MEM.ALU_output=ALU_ori(data1,data2);
    else if (ID_EXE.ctrl_ExtOp==op_func&&ID_EXE.ctrl_ExtFunc==func_or)
        tmp_EXE_MEM.ALU_output=ALU_or(data1,data2);
    else if (ID_EXE.ctrl_ExtOp==op_func&&ID_EXE.ctrl_ExtFunc==func_xor)
        tmp_EXE_MEM.ALU_output=ALU_xor(data1,data2);
    else if (ID_EXE.ctrl_ExtOp==op_nori)
        tmp_EXE_MEM.ALU_output=~ALU_ori(data1,data2);
    else if (ID_EXE.ctrl_ExtOp==op_func&&ID_EXE.ctrl_ExtFunc==func_nor)
        tmp_EXE_MEM.ALU_output=~ALU_or(data1,data2);
    else if (ID_EXE.ctrl_ExtOp==op_func&&ID_EXE.ctrl_ExtFunc==func_nand)
        tmp_EXE_MEM.ALU_output=~ALU_and(data1,data2);
    else if (ID_EXE.ctrl_ExtOp==op_slti||(ID_EXE.ctrl_ExtOp==op_func&&ID_EXE.ctrl_ExtFunc==func_slt))
        tmp_EXE_MEM.ALU_output=ALU_slt(data1,data2);
    else if (ID_EXE.ctrl_ExtOp==op_func&&ID_EXE.ctrl_ExtFunc==func_sll)
        tmp_EXE_MEM.ALU_output=ALU_sll(data2,shamt);
    else if (ID_EXE.ctrl_ExtOp==op_func&&ID_EXE.ctrl_ExtFunc==func_srl)
        tmp_EXE_MEM.ALU_output=ALU_srl(data2,shamt);
    else if (ID_EXE.ctrl_ExtOp==op_func&&ID_EXE.ctrl_ExtFunc==func_sra)
        tmp_EXE_MEM.ALU_output=ALU_sra(data2,shamt);
    else if (ID_EXE.ctrl_ExtOp==op_lui)
        tmp_EXE_MEM.ALU_output=ALU_lui(data1,data2);
    else if (ID_EXE.ctrl_ExtOp==op_jal)
        tmp_EXE_MEM.ALU_output=JAL_PC;
}

u32 ALU_lui(u32 data1,u32 data2)
{
    return data2<<16;
}

u32 ALU_add(u32 data1,u32 data2)
{
    int MSB1=data1>>31,MSB2=data2>>31,MSB3=(data1+data2)>>31;
    
    if (MSB1==MSB2 && MSB1!=MSB3)
    {
        //printf("0x%08X\n",data1);
        //printf("0x%08X\n",data2);
        //printf("0x%08X\n",data1+data2);
        error_state[3]=1;
    }
    
    return data1+data2;
}

u32 ALU_and(u32 data1,u32 data2)
{
    return data1&data2;
}

u32 ALU_andi(u32 data1,u32 data2)
{
    return data1&(data2&0x0000FFFF);
}

u32 ALU_or(u32 data1,u32 data2)
{
    return data1|data2;
}

u32 ALU_ori(u32 data1,u32 data2)
{
    return data1|(data2&0x0000FFFF);
}

u32 ALU_xor(u32 data1,u32 data2)
{
    return data1^data2;
}

u32 ALU_slt(u32 data1,u32 data2)
{
    return ((signed)data1<(signed)data2)?1:0;
}

u32 ALU_sll(u32 data1,u32 shamt)
{
    if (shamt==32)
        return 0x00000000;
    else if (shamt==0)
        return data1;
    else
        return data1<<shamt;
}

u32 ALU_srl(u32 data1,u32 shamt)
{
    if(shamt==32)
        return 0x00000000;
    else if (shamt==0)
        return data1;
    else
        return data1>>shamt;
}

u32 ALU_sra(u32 data1,u32 shamt)
{
    int sign=data1>>31;
    
    if (sign==1)
    {
        if(shamt==32)
            return 0xffffffff;
        else if (shamt==0)
            return data1;
        else
            return (data1>>shamt)|(0xffffffff<<(32-shamt));
    }
    else
        return ALU_srl(data1,shamt);
}

void MEM()
{
    //printf("DM\n");
    if (n_cycle==169)
        printf("DM = 0x%08X\n",EXE_MEM.instr);
    //printf("[%d] DM = 0x%08X...\n\n\n",n_cycle,EXE_MEM.instr);
    int opcode=getSlice(EXE_MEM.instr,31,26);
    int byteNum=0,sign=0;
    tmp_MEM_WB.ALU_output=EXE_MEM.ALU_output;
    tmp_MEM_WB.write_reg=EXE_MEM.write_reg;
    tmp_MEM_WB.ctrl_RegWr=EXE_MEM.ctrl_RegWr;
    tmp_MEM_WB.ctrl_MemtoReg=EXE_MEM.ctrl_MemtoReg;
    tmp_MEM_WB.instr=EXE_MEM.instr;
    if(EXE_MEM.instr==0x00000000||EXE_MEM.instr==0xFFFFFFFF)
    {
        tmp_MEM_WB.instr=EXE_MEM.instr;
        return;
    }
    
    if (EXE_MEM.ctrl_MemW==1)
    {
        if (opcode==op_sw)
            byteNum=4;
        else if (opcode==op_sh)
            byteNum=2;
        else if (opcode==op_sb)
            byteNum=1;
        SMemory(EXE_MEM.ALU_output,byteNum,EXE_MEM.write_data);
    }
    if (EXE_MEM.ctrl_MemR==1)
    {
        if (n_cycle==169)
            printf("MEMR = 0x%02X,0x%08X,%d\n",opcode,EXE_MEM.ALU_output,(signed)EXE_MEM.ALU_output);
        if (opcode==op_lw)
            byteNum=4;
        else if (opcode==op_lh||opcode==op_lhu)
            byteNum=2;
        else if (opcode==op_lb||opcode==op_lbu)
            byteNum=1;
        if (opcode==op_lw||opcode==op_lh||opcode==op_lb)
            sign=1;
        LMemory(EXE_MEM.ALU_output,byteNum,sign);
    }
    
}

void LMemory(u32 memoryLocation,int byteNum,int sign)
{
    int temp=0,i;
    int MSB;
    if (byteNum>1&&memoryLocation%byteNum!=0)
    {
        error_state[2]=1;
        halt=1;
        //return;
    }
    if ((signed)(memoryLocation+byteNum-1)<0||(signed)(memoryLocation+byteNum-1)>1023)
    {
        error_state[1]=1;
        halt=1;
        return;
    }
    else
        MSB=D_Memory[memoryLocation]>>7;
    
    for (i=0;i<=byteNum-1;i++)
    {
        
        if ((signed)(memoryLocation+i)<0||(signed)(memoryLocation+i)>1023)
        {
            error_state[1]=1;
            halt=1;
            return;
        }
        else
        {
            temp=(temp<<8);
            temp=temp|D_Memory[memoryLocation+i];
        }
    }
    
    if (sign==1&&MSB==1&&byteNum<4)
    {
        temp=temp|(0xffffffff<<(8*byteNum));
    }
    
    tmp_MEM_WB.DMem_output=temp;
}

void SMemory(u32 memoryLocation,int byteNum,u32 data)
{
    int i;
    if (byteNum>1&&memoryLocation%byteNum!=0)
    {
        error_state[2]=1;
        halt=1;
        //return;
    }
    for(i=byteNum-1;i>=0;i--)
    {
        if ((signed)(memoryLocation+i)<0||(signed)(memoryLocation+i)>1023)
        {
            error_state[1]=1;
            halt=1;
            return;
        }
        else
        {
            D_Memory[memoryLocation+i]=(u8)(data<<24>>24);
            data=data>>8;
        }
    }
}

void WB()
{
    //printf("WB >");
    //printf("[%d] WB = 0x%08X...\n",n_cycle,MEM_WB.instr);
    if (MEM_WB.instr==0x00000000)
        return;
    if (MEM_WB.instr==0xFFFFFFFF)
    {
        halt=1;
        return;
    }
    
    if (getSlice(MEM_WB.instr,31,26)==op_jal)
    {
        reg[31]=JAL_PC;
    }
    else if(MEM_WB.ctrl_RegWr)
    {
        if (MEM_WB.write_reg==0)
        {
            error_state[0]=1;
            return;
        }
        reg[MEM_WB.write_reg] = MEM_WB.ctrl_MemtoReg ? MEM_WB.ALU_output : MEM_WB.DMem_output;
    }
    
}


int getSlice(u32 instrction,int from,int to) //unsigned slice
{
    return (instrction<<(31-from))>>(32-(from-to+1));
}

void load_iimage()
{
//    FILE *fp;
//    int n=0;
//    u32 sum;
//    int n_instr=0;
//    int i;
//    int pc_done=0;
//    u8 buff[4];
//    
//    fp = fopen(Iimage, "rb");
//    if (!fp)
//    {
//        halt=1;
//        printf(Iimage" not found.\n");
//        return;
//    }
//    while(fread(buff, sizeof(char),4, fp))
//    {
//        sum=0;
//        for (i=0; i<=3;i++)
//        {
//            sum=sum<<8;
//            sum=sum|buff[i];
//        }
//        //printf("%02d = 0x%02x%02x%02x%02x [%010u] \n",n++,buff[3], buff[2],buff[1], buff[0],sum);
//        if (pc_done==0)
//        {
//            PC=sum;
//            printf("sum=%d",sum);
//            pc_done=1;
//        }
//        else if (instr_num==0)
//        {
//            instr_num=sum;
//        }
//        else
//        {
//            
//            for (i=0; i<4;i++)
//            {
//                I_Memory[PC+n_instr++]=buff[i];
//            }
//            instr_num--;
//            if (instr_num==0)
//                break;
//            
//            
//        }
//    }
    int i;
    unsigned char buf[4];
    FILE *fp;
    fp=fopen("iimage.bin", "r");
    for(i=0;i<1024;I_Memory[i++]=0);
    for(i=0;i<32;reg[i++]=0);
    fread((void *)buf, (size_t)1, (size_t)4, fp);
    PC = (buf[0] << 24) |   (buf[1] << 16) |  (buf[2] << 8) | buf[3];
    fread(&buf, (size_t)1, (size_t)4, fp);
    instr_num = (buf[0] << 24) |   (buf[1] << 16) |  (buf[2] << 8) | buf[3];
    fread((void *)(I_Memory+PC), (size_t)1, (size_t)instr_num*4, fp);
    fclose(fp);
}

void load_dimage()
{
    FILE *fp;
    u32 sum;
    int n_mem=0;
    int i,n=0;
    int sp_done=0;
    u8 buff[4];
    
    fp = fopen(Dimage, "rb");
    if (!fp)
    {
        halt=1;
        printf(Dimage" not found.\n");
        return;
    }
    while(fread(buff, sizeof(char),4, fp))
    {
        sum=0;
        for (i=0; i<=3;i++)
        {
            sum=sum<<8;
            sum=sum|buff[i];
        }
        printf("%02d = 0x%02x%02x%02x%02x [%010u] \n",n++,buff[3], buff[2],buff[1], buff[0],sum);
        if (sp_done==0)
        {
            reg[29]=sum;
            sp_done=1;
        }
        else if (mem_num==0)
        {
            mem_num=sum;
        }
        else
        {
            //printf("%d...\n",mem_num);
            for (i=0; i<4;i++)
            {
                D_Memory[n_mem++]=buff[i];
            }
            mem_num--;
            if (mem_num==0)
                break;
            
        }
    }
}
