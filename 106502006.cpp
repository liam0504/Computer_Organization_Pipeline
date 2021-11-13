#include <string>
#include<fstream>
#include<iostream>
#include<bitset>
#include<istream>
#include<string>
using namespace std;

// Sign Extension
#define SIGNFLAG (1<<15)                        // 1000000000000000
#define DATABITS (SIGNFLAG-1)                   // 0111111111111111

// Stage
struct IF_ID{
    int PC;
    string Inst;            // 32 bits
} Reg_IF_ID = {0, "00000000000000000000000000000000"}, temp_IF_ID = {0, "00000000000000000000000000000000"};
struct ID_EX{
    int ReadData1;
    int ReadData2;
    int immediate;
    int Rs;
    int Rt;
    int Rd;
    string Control;
} Reg_ID_EX = {0, 0, 0, 0, 0 ,0, "000000000"}, temp_ID_EX = {0, 0, 0, 0, 0 ,0, "000000000"};
struct EX_MEM{
    int ALUout;
    int WriteData;
    int Rt_Rd;
    string Control;
} Reg_EX_MEM = {0, 0, 0, "00000"}, temp_EX_MEM = {0, 0, 0, "00000"};
struct MEM_WB{
    int ReadData;
    int ALUout;
    unsigned int Rt_Rd;
    string Control;
} Reg_MEM_WB = {0, 0, 0, "00"}, temp_MEM_WB = {0, 0, 0, "00"};

// Variable
ifstream fs;                // Input file's name
ofstream ofs;               // Output file's name
string INPUT_NAME;
string OUTPUT_NAME;
string instruction[10]={};  // Instruction
int Reg[10]={};             // Register
int Data[5]={};             // Data
int PC;                     // Program counter
int input_count=0;          // The number of instruction
int CC = 1;                 // Current clock cycle
int num=1;
bool Flush = false;          // BEQ flush IF
bool Forward_Rs = false;    // update Register value 
bool Forward_Rt = false;    // update Register value

// Function
void init();
void input();
void IF();
void ID();
void EX();
void MEM();
void WB();
bool empty();
bool empty_test();
void output();
void returnnext();
void changeinput();
int main(){
    for(int i = 0 ; i < 4 ; i++){
        switch (i) {
        case 0:
            INPUT_NAME = "General.txt";
            OUTPUT_NAME = "genResult.txt";
            break;
        case 1:
            INPUT_NAME = "Datahazard.txt";
            OUTPUT_NAME = "dataResult.txt";
            break;
        case 2:
            INPUT_NAME = "Lwhazard.txt";
            OUTPUT_NAME = "loadResult.txt";
            break;
        case 3:
            INPUT_NAME = "Branchhazard.txt";
            OUTPUT_NAME = "branchResult.txt";
            break;
        }
        init();
        input();

        do{
            IF();
            ID();
            EX();
            MEM();
            WB();
            Reg_IF_ID = temp_IF_ID;
            Reg_ID_EX = temp_ID_EX;
            Reg_EX_MEM = temp_EX_MEM;
            Reg_MEM_WB = temp_MEM_WB;
            output();

            if(Flush)
                Reg_IF_ID.Inst = "00000000000000000000000000000000";
            Forward_Rs = false;
            Forward_Rt = false;
            Flush = false;
            ++CC;
        }while(!empty());
        returnnext();
    }
    return 0;
}

void init(){            // Initialize the register
    Reg[0] = 0;
    Reg[1] = 9;
    Reg[2] = 5;
    Reg[3] = 7;
    Reg[4] = 1;
    Reg[5] = 2;
    Reg[6] = 3;
    Reg[7] = 4;
    Reg[8] = 5;
    Reg[9] = 6;
    Data[0] = 5;
    Data[1] = 9;
    Data[2] = 4;
    Data[3] = 8;
    Data[4] = 7;
    PC = 0;
}

void input(){           // Get input file's instructions
    fs.open(INPUT_NAME, fstream::in);
    string s1,s2;
    fs >> s1;
    for(input_count;input_count<=(s1.length()/32)-1;input_count++){
        instruction[input_count] = s2.assign(s1,input_count*32,32);
    }
    fs.close();
    ofs.open(OUTPUT_NAME, ofstream::out | ofstream::trunc);
    ofs.close();
}

void IF(){
    int PC4 = PC/4;
    if(PC4 >= input_count){
        temp_IF_ID.Inst = "00000000000000000000000000000000";
        PC += 4;
        temp_IF_ID.PC = PC;
        return;
    }
    temp_IF_ID.Inst = instruction[PC4];
    PC += 4;
    temp_IF_ID.PC = PC;
    return;
}

void ID(){
    string instruction_ID = Reg_IF_ID.Inst;
    int PC_ID = Reg_IF_ID.PC;
    int Rs_ID = 0, Rt_ID = 0, Rd_ID = 0;
    int func = 0;                           // R-type
    int data1_ID = 0, data2_ID = 0;         // Register
    int immediate_ID = 0;                   // I-type immediate
    int Op_ID;
    string Control_ID = "000000000";
    // Rs Rt Rd part
    Rs_ID = stoul(instruction_ID.substr(6,5), nullptr, 2);
    Rt_ID = stoul(instruction_ID.substr(11,5), nullptr, 2);
    Rd_ID = stoul(instruction_ID.substr(16,5), nullptr, 2);
    
    // data1 and data2 part
    data1_ID = Reg[Rs_ID];
    data2_ID = Reg[Rt_ID];

    // Sign extension part (2's complement)
    immediate_ID = stoi(instruction_ID.substr(16,16), nullptr, 2);
    if((immediate_ID & SIGNFLAG) != 0){
        immediate_ID = (~immediate_ID & DATABITS) + 1;
        immediate_ID = -immediate_ID;
    }
    //Control signals part
    Op_ID = stoul(instruction_ID.substr(0,6), nullptr, 2);
    //Instruction Execution
    switch (Op_ID) {
        case 0:
            if(stoul(instruction_ID, nullptr, 2) != 0)
                Control_ID = "110000010";
            else
                Control_ID = "000000000";
            break;
        case 35:
            Control_ID = "000101011";
            break;
        case 43:
            Control_ID = "000100100";
            break;
        case 4:
            Control_ID = "001010000";
            break;
        default:
            if(instruction_ID.substr(0,6) == "001000")          // ADDi
                Control_ID = "000100010";
            else if(instruction_ID.substr(0,6) == "001100")     // ANDi
                Control_ID = "011100010";
            else
                Control_ID = "error";                           // error
    }

    // Banch equal part
    if(Control_ID[4] == '1'){
        if(data1_ID == data2_ID){
            Flush = true;
            PC += immediate_ID * 4 - 4;
        }
    }

    // Detect lw hazard part
    string Control_EX = Reg_ID_EX.Control;
    int Rt_EX = Reg_ID_EX.Rt;
    if(Control_EX[5] == '1' && (Rt_EX == Rt_ID || Rt_EX == Rs_ID)){
        PC -= 4;
        temp_IF_ID.PC -= 4;
        temp_IF_ID.Inst = instruction[(PC-4)/4];
        temp_ID_EX.Control = "000000000";
        temp_ID_EX.ReadData1 = data1_ID;
        temp_ID_EX.ReadData2 = data2_ID;
        temp_ID_EX.immediate = immediate_ID;
        temp_ID_EX.Rs = Rs_ID;
        temp_ID_EX.Rt = Rt_ID;
        temp_ID_EX.Rd = Rd_ID;
        return;
    }
    temp_ID_EX.ReadData1 = data1_ID;
    temp_ID_EX.ReadData2 = data2_ID;
    temp_ID_EX.immediate = immediate_ID;
    temp_ID_EX.Rs = Rs_ID;
    temp_ID_EX.Rt = Rt_ID;
    temp_ID_EX.Rd = Rd_ID;
    temp_ID_EX.Control = Control_ID;
    return;
}

void EX(){
    int data1_EX = Reg_ID_EX.ReadData1;
    int data2_EX = Reg_ID_EX.ReadData2;
    int immediate_EX = Reg_ID_EX.immediate;         //......
    int Rs_EX = Reg_ID_EX.Rs;
    int Rt_EX = Reg_ID_EX.Rt;
    int Rd_EX = Reg_ID_EX.Rd;
    string Control_EX = Reg_ID_EX.Control.substr(0,4);
    int ALUout_EX = 0;
    int WriteData_EX = data2_EX;
    int Rt_Rd_EX = 0;
    string Control_MEM_WB = Reg_ID_EX.Control.substr(4,5);

    // Rt/Rd part
    if(Control_EX[0] == '0')            // R-Type keeps value in Rd
        Rt_Rd_EX = Rt_EX;
    else
        Rt_Rd_EX = Rd_EX;

    // Forwarding part
    int src1 = 0;
    int src2 = 0;
    int ALUout_MEM = Reg_EX_MEM.ALUout;
    int ALUout_WB = Reg_MEM_WB.ALUout;
    int Rt_Rd_MEM = Reg_EX_MEM.Rt_Rd;
    int Rt_Rd_WB = Reg_MEM_WB.Rt_Rd;
    string Control_MEM = Reg_EX_MEM.Control;
    string Control_WB = Reg_MEM_WB.Control;
    int Data_WB;

    if(Control_WB[1] == '0')
        Data_WB = ALUout_WB;
    else
        Data_WB = Reg_MEM_WB.ReadData;
    if(Control_MEM[3] == '1' && Rt_Rd_MEM != 0){
        if(Rt_Rd_MEM == Rs_EX)
            src1 = 1;
        if(Rt_Rd_MEM == Rt_EX)
            src2 = 1;
    }
    if(Control_WB[0] == '1' && Rt_Rd_WB != 0 && ( Control_MEM[3] != '1' || (Rt_Rd_MEM != Rs_EX && Rt_Rd_MEM != Rt_EX))){
        if(Rt_Rd_WB == Rs_EX){
            src1 = 2;
            Forward_Rs = true;
        }
        if(Rt_Rd_WB == Rt_EX){
            src2 = 2;
            Forward_Rt = true;
        }
    }

    // ALUout part
    int ALUsrc1_EX;
    int ALUsrc2_EX;
    switch (src1){
        case 0:
            ALUsrc1_EX = data1_EX;
            break;
        case 1:
            ALUsrc1_EX = ALUout_MEM;
            break;
        case 2:
            ALUsrc1_EX = Data_WB;
            break;
    }
    switch (src2){
        case 0:
            ALUsrc2_EX = (Control_EX[3] == '1') ? immediate_EX : data2_EX;
            break;
        case 1:
            ALUsrc2_EX = ALUout_MEM;
            WriteData_EX = ALUsrc2_EX;
            break;
        case 2:
            ALUsrc2_EX = Data_WB;
            WriteData_EX = ALUsrc2_EX;
            break;
    }

    string Function_EX = bitset<6>(immediate_EX).to_string();       // Keep last 6 bits into Function_EX
    string temp = Control_EX.substr(1,2);

    if(temp == "00"){
        ALUout_EX = ALUsrc1_EX + ALUsrc2_EX;
    }
    else if(temp == "01"){
        ALUout_EX = ALUsrc1_EX - ALUsrc2_EX;
    }
    else if(temp == "10"){                      // Execute the R-Type instruction
        if(Function_EX == "100000")             // ADD
            ALUout_EX = ALUsrc1_EX + ALUsrc2_EX;
        else if(Function_EX == "100010")        // SUB
            ALUout_EX = ALUsrc1_EX - ALUsrc2_EX;
        else if(Function_EX == "100100")        // AND
            ALUout_EX = ALUsrc1_EX & ALUsrc2_EX;
        else if(Function_EX == "100101")        // OR
            ALUout_EX = ALUsrc1_EX | ALUsrc2_EX;
        else if(Function_EX == "101010")        // SLT
            ALUout_EX = (ALUsrc1_EX <= ALUsrc2_EX) ? 1 : 0;
    }
    else if(temp == "11"){
        ALUout_EX = ALUsrc1_EX & ALUsrc2_EX;
    }
    temp_EX_MEM.ALUout = ALUout_EX;
    temp_EX_MEM.WriteData = WriteData_EX;
    temp_EX_MEM.Rt_Rd = Rt_Rd_EX;
    temp_EX_MEM.Control = Control_MEM_WB;
}

void MEM(){
    int WriteData_MEM = Reg_EX_MEM.WriteData;
    string Control_MEM = Reg_EX_MEM.Control.substr(0,3);
    string Control_WB = Reg_EX_MEM.Control.substr(3,2);
    int ALUout_MEM = Reg_EX_MEM.ALUout;
    int ReadData_MEM = 0;
    int Rt_Rd_MEM = Reg_EX_MEM.Rt_Rd;

    // Read memory part
    if(Control_MEM[1] == '1')
        ReadData_MEM = Data[ALUout_MEM/4];
    
    // Write memory
    if(Control_MEM[2] == '1')
        Data[ALUout_MEM/4] = WriteData_MEM;

    temp_MEM_WB.ALUout = ALUout_MEM;
    temp_MEM_WB.ReadData = ReadData_MEM;
    temp_MEM_WB.Rt_Rd = Rt_Rd_MEM;
    temp_MEM_WB.Control = Control_WB;
}

void WB(){
    int ReadData_WB = Reg_MEM_WB.ReadData;
    int ALUout_WB = Reg_MEM_WB.ALUout;
    int Rt_Rd_WB = Reg_MEM_WB.Rt_Rd;
    string Control_WB = Reg_MEM_WB.Control;

    if(Control_WB[0] == '1' && Rt_Rd_WB != 0){
        if(Control_WB[1] == '1')
            Reg[Rt_Rd_WB] = ReadData_WB;
        else
            Reg[Rt_Rd_WB] = ALUout_WB;
    }
    if(Rt_Rd_WB == temp_ID_EX.Rs && Rt_Rd_WB != 0)
        temp_ID_EX.ReadData1 = Reg[Rt_Rd_WB];
    if(Rt_Rd_WB == temp_ID_EX.Rt && Rt_Rd_WB != 0)
        temp_ID_EX.ReadData2 = Reg[Rt_Rd_WB];
    return;
}

bool empty(){
    if(PC/4 > input_count){         // All the register return to initial state
        if(empty_test())
            return true;
        else
            return false;
    }
    else
        return false;
}

bool empty_test(){
    if(!(Reg_IF_ID.Inst == "00000000000000000000000000000000"))
        return false;
    if(!(Reg_ID_EX.ReadData1 == 0 && Reg_ID_EX.ReadData2 == 0 && Reg_ID_EX.immediate == 0 && Reg_ID_EX.Rs == 0 && Reg_ID_EX.Rt == 0 && Reg_ID_EX.Rd == 0 && Reg_ID_EX.Control == "000000000"))
        return false;
    if(!(Reg_EX_MEM.ALUout == 0 && Reg_EX_MEM.WriteData == 0 && Reg_EX_MEM.Rt_Rd == 0 && Reg_EX_MEM.Control == "00000"))
        return false;
    if(!(Reg_MEM_WB.ReadData == 0 && Reg_MEM_WB.ALUout == 0 && Reg_MEM_WB.Rt_Rd == 0 && Reg_MEM_WB.Control == "00"))
        return false;
    else
        return true;
}

void output(){
    ofs.open(OUTPUT_NAME, ofstream::app);         //！！！！！！！！！！！ 輸出檔
    ofs << "CC " << CC << ":" << endl;
    ofs << endl;

    ofs << "Registers:" << endl;
    for(int i = 0; i < 10; i++)
        ofs << "$" << i << ": " << Reg[i] << endl;

    ofs << endl << "Data memory:";
    ofs << endl << "0x00: " << Data[0];
    ofs << endl << "0x04: " << Data[1];
    ofs << endl << "0x08: " << Data[2];
    ofs << endl << "0x0C: " << Data[3];
    ofs << endl << "0x10: " << Data[4];
    ofs << endl;

    ofs << endl << "IF/ID :";
    ofs << endl << "PC              " << Reg_IF_ID.PC;
    ofs << endl << "Instruction     " << Reg_IF_ID.Inst;
    ofs << endl;

    ofs << endl << "ID/EX :";
    ofs << endl << "ReadData1       " << Reg_ID_EX.ReadData1;
    ofs << endl << "ReadData2       " << Reg_ID_EX.ReadData2;
    ofs << endl << "sign_ext        " << Reg_ID_EX.immediate;
    ofs << endl << "Rs              " << Reg_ID_EX.Rs;
    ofs << endl << "Rt              " << Reg_ID_EX.Rt;
    ofs << endl << "Rd              " << Reg_ID_EX.Rd;
    ofs << endl << "Control signals " << Reg_ID_EX.Control;
    ofs << endl;

    ofs << endl << "EX/MEM :";
    ofs << endl << "ALUout          " << Reg_EX_MEM.ALUout;
    ofs << endl << "WriteData       " << Reg_EX_MEM.WriteData;
    ofs << endl << "Rt/Rd           " << Reg_EX_MEM.Rt_Rd;
    ofs << endl << "Control signals " << Reg_EX_MEM.Control;
    ofs << endl;

    ofs << endl << "MEM/WB :";
    ofs << endl << "ReadData        " << Reg_MEM_WB.ReadData;
    ofs << endl << "ALUout          " << Reg_MEM_WB.ALUout;
    ofs << endl << "Rt/Rd           " << Reg_MEM_WB.Rt_Rd ;
    ofs << endl << "Control signals " << Reg_MEM_WB.Control;
    ofs << endl;
    ofs << "=================================================================";
    ofs << endl;
    ofs.close();
    return;
}
void returnnext(){
    // IF_ID
        Reg_IF_ID.PC = 0;
        temp_IF_ID.Inst = "00000000000000000000000000000000";
        Reg_IF_ID.PC = 0;
        temp_IF_ID.Inst = "00000000000000000000000000000000";
        // ID_EX
        Reg_ID_EX.ReadData1 = 0;
        temp_ID_EX.ReadData1 = 0;
        Reg_ID_EX.ReadData2 = 0;
        temp_ID_EX.ReadData2 = 0;
        Reg_ID_EX.immediate = 0;
        temp_ID_EX.immediate = 0;
        Reg_ID_EX.Rs = 0;
        temp_ID_EX.Rs = 0;
        Reg_ID_EX.Rt = 0;
        temp_ID_EX.Rt = 0;
        Reg_ID_EX.Rd = 0;
        temp_ID_EX.Rd = 0;
        Reg_ID_EX.Control = "000000000";
        temp_ID_EX.Control = "000000000";
        // EX_MEM
        Reg_EX_MEM.ALUout = 0;
        temp_EX_MEM.ALUout = 0;
        Reg_EX_MEM.WriteData = 0;
        temp_EX_MEM.WriteData = 0;
        Reg_EX_MEM.Rt_Rd = 0;
        temp_EX_MEM.Rt_Rd = 0;
        Reg_EX_MEM.Control = "00000";
        temp_EX_MEM.Control = "00000";
        // MEM_WB
        Reg_MEM_WB.ReadData = 0;
        temp_MEM_WB.ReadData = 0;
        Reg_MEM_WB.ALUout = 0;
        temp_MEM_WB.ALUout = 0;
        Reg_MEM_WB.Rt_Rd = 0;
        temp_MEM_WB.Rt_Rd = 0;
        Reg_MEM_WB.Control =  "00";
        temp_MEM_WB.Control = "00";
        // Variable
        string instruction[10]={};
        PC = 0;
        input_count=0;
        CC = 1;
        Flush = false;
        Forward_Rs = false; 
        Forward_Rt = false;
}