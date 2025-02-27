#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>
#include <unordered_map>
#include <iomanip>
#include <unistd.h>

using namespace std;

struct Instruction {
    int addr, opcode, rs, rt, rd, shamt, funct, imm, target;
    string bin, asm_code;
};

vector<Instruction> instructions;
unordered_map<int, int> memory;
int registers[32] = {0};

void disassemble(const string &inputFile, const string &outputFile) {
    ifstream in(inputFile, ios::binary);
    ofstream out(outputFile + "_dis.txt");
    int addr = 96; // Removed 'data_start'
    unsigned int word;
    
    while (in.read(reinterpret_cast<char*>(&word), sizeof(word))) {
        word = __builtin_bswap32(word); // Convert endianness
        Instruction instr;
        instr.addr = addr;
        instr.bin = bitset<32>(word).to_string();
        instr.opcode = (word >> 26) & 0x3F;

        if (instr.opcode == 0) { // R-Type
            instr.rs = (word >> 21) & 0x1F;
            instr.rt = (word >> 16) & 0x1F;
            instr.rd = (word >> 11) & 0x1F;
            instr.shamt = (word >> 6) & 0x1F;
            instr.funct = word & 0x3F;
            
            switch (instr.funct) {
                case 32: instr.asm_code = "ADD R" + to_string(instr.rd) + ", R" + to_string(instr.rs) + ", R" + to_string(instr.rt); break;
                case 34: instr.asm_code = "SUB R" + to_string(instr.rd) + ", R" + to_string(instr.rs) + ", R" + to_string(instr.rt); break;
                case 0: instr.asm_code = "SLL R" + to_string(instr.rd) + ", R" + to_string(instr.rt) + ", #" + to_string(instr.shamt); break;
                case 2: instr.asm_code = "SRL R" + to_string(instr.rd) + ", R" + to_string(instr.rt) + ", #" + to_string(instr.shamt); break;
                case 24: instr.asm_code = "MUL R" + to_string(instr.rd) + ", R" + to_string(instr.rs) + ", R" + to_string(instr.rt); break;
                case 36: instr.asm_code = "AND R" + to_string(instr.rd) + ", R" + to_string(instr.rs) + ", R" + to_string(instr.rt); break;
                case 37: instr.asm_code = "OR R" + to_string(instr.rd) + ", R" + to_string(instr.rs) + ", R" + to_string(instr.rt); break;
                case 10: instr.asm_code = "MOVZ R" + to_string(instr.rd) + ", R" + to_string(instr.rs) + ", R" + to_string(instr.rt); break;
                case 8: instr.asm_code = "JR R" + to_string(instr.rs); break;
                case 13: instr.asm_code = "BREAK"; break;
                default: instr.asm_code = "NOP";
            }
        } else { // I/J-Type
            instr.rs = (word >> 21) & 0x1F;
            instr.rt = (word >> 16) & 0x1F;
            instr.imm = (int16_t)(word & 0xFFFF);
            instr.target = word & 0x3FFFFFF;
            
            switch (instr.opcode) {
                case 8: instr.asm_code = "ADDI R" + to_string(instr.rt) + ", R" + to_string(instr.rs) + ", #" + to_string(instr.imm); break;
                case 43: instr.asm_code = "SW R" + to_string(instr.rt) + ", " + to_string(instr.imm) + "(R" + to_string(instr.rs) + ")"; break;
                case 35: instr.asm_code = "LW R" + to_string(instr.rt) + ", " + to_string(instr.imm) + "(R" + to_string(instr.rs) + ")"; break;
                case 1: instr.asm_code = "BLTZ R" + to_string(instr.rs) + ", #" + to_string(addr + 4 + (instr.imm << 2)); break;
                case 2: instr.asm_code = "J #" + to_string(instr.target << 2); break;
                case 4: instr.asm_code = "BEQ R" + to_string(instr.rs) + ", R" + to_string(instr.rt) + ", #" + to_string(addr + 4 + (instr.imm << 2)); break;
                default: instr.asm_code = "NOP";
            }
        }
        
        out << instr.bin << "\t" << instr.addr << "\t" << instr.asm_code << endl;
        instructions.push_back(instr);
        addr += 4;
    }
    in.close();
    out.close();
}

void simulate(const string &outputFile) {
    ofstream out(outputFile + "_sim.txt");
    int pc = 96, cycle = 1;
    
    while (true) {
        if (pc >= static_cast<int>(instructions.size()) * 4 + 96) break;
        Instruction &instr = instructions[(pc - 96) / 4];
        
        out << "====================\n";
        out << "cycle: " << cycle << "\t" << pc << "\t" << instr.asm_code << "\n\n";
        
        if (instr.asm_code == "BREAK") break;
        
        switch (instr.opcode) {
            case 8: registers[instr.rt] = registers[instr.rs] + instr.imm; break;
            case 43: memory[registers[instr.rs] + instr.imm] = registers[instr.rt]; break;
            case 35: registers[instr.rt] = memory[registers[instr.rs] + instr.imm]; break;
            case 1: if (registers[instr.rs] < 0) pc = instr.imm; break;
            case 2: pc = instr.target << 2; continue;
        }
        
        pc += 4;
        cycle++;
    }
    out.close();
}

int main(int argc, char* argv[]) {
    if (argc != 5 || string(argv[1]) != "-i" || string(argv[3]) != "-o") {
        cerr << "Usage: mipssim -i INPUTFILE -o OUTPUTFILE" << endl;
        return 1;
    }
    string inputFile = argv[2], outputFile = argv[4];
    disassemble(inputFile, outputFile);
    simulate(outputFile);
    return 0;
}
