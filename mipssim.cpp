#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <iomanip>
#include <bitset>
#include <unordered_map>
#include<string>
#include<fstream>
using namespace std;

int main(int argc, char *argv[])
{
  char buffer[4];
  int i;
  char * iPtr;
  iPtr = (char*)(void*) &i;

  int FD = open(argv[2], O_RDONLY);
  if (FD<0){cerr<<"no file"<<endl; exit(0);}
  string disfileName = string(argv[4])+"_dis.txt";
  string simfileName = string(argv[4])+"_sim.txt";

  ofstream disout(disfileName);
  ofstream simout(simfileName);

  struct decodedInstruction {
    int op, rs, rt, rd, imm,I, shift, func;
    unsigned int UI;
    int addr;
    string inst, binNoSpace, binFormatted;
  };

  int amt = 4;
  int addr = 96;
  unordered_map< int, decodedInstruction> MEM;
  bool breakEncountered = false;

  while( amt != 0 )
  {
    decodedInstruction instr;
    amt = read(FD, buffer, 4);
    if( amt == 4)
    {
      iPtr[0] = buffer[3];
      iPtr[1] = buffer[2];
      iPtr[2] = buffer[1];
      iPtr[3] = buffer[0];
      instr.addr = addr;
      instr.UI = static_cast<unsigned int> (i);
      instr.op = instr.UI >> 26;
      instr.rs = (instr.UI >> 21) & 0x1F;
      instr.rt = (instr.UI >> 16) & 0x1F;
      instr.rd = (instr.UI >> 11) & 0x1F;
      instr.shift = (instr.UI >> 6) & 0x1F;
      instr.func = instr.UI & 0x3F;
      instr.imm = static_cast<int16_t>(instr.UI & 0xFFFF);
      string bin = bitset<32>(i).to_string();
      instr.binFormatted = bin.substr(0, 6) + " " + bin.substr(6, 5) + " " + bin.substr(11, 5) + " " + bin.substr(16, 5)+ " " + bin.substr(21, 5) + " " + bin.substr(26, 6);
      instr.inst += instr.binFormatted + "\t" + to_string(instr.addr) + "\t";
      if(breakEncountered){
          disout<< bin << "\t"<< instr.addr <<"\t"<<static_cast<int>(instr.UI) <<endl;
      }else{

        if (instr.op == 8) { // ADDI
            instr.inst += "ADDI\tR" + to_string(instr.rt) + ", R" + to_string(instr.rs) + ", #" + to_string(instr.imm);
        } else if (instr.op == 43) { // SW
            instr.inst += "SW\tR" + to_string(instr.rt) + ", " + to_string(instr.imm) + "(R" + to_string(instr.rs) + ")";
        } else if (instr.op == 35) { // LW
            instr.inst += "LW\tR" + to_string(instr.rt) + ", " + to_string(instr.imm) + "(R" + to_string(instr.rs) + ")";
        } else if (instr.op == 2) { // J
            instr.inst += "J\t#" + to_string((instr.UI & 0x3FFFFFF) << 2);
        } else if (instr.op == 0 && instr.func == 32) { // ADD
            instr.inst += "ADD\tR" + to_string(instr.rd) + ", R" + to_string(instr.rs) + ", R" + to_string(instr.rt);
        } else if (instr.op == 0 && instr.func == 34) { // SUB
            instr.inst += "SUB\tR" + to_string(instr.rd) + ", R" + to_string(instr.rs) + ", R" + to_string(instr.rt);
        } else if (instr.op == 4) { // BEQ
            instr.inst += "BEQ\tR" + to_string(instr.rs) + ", R" + to_string(instr.rt) + ", #" + to_string(instr.imm);
        } else if (instr.op == 1 && instr.rt == 0) { //BLTZ
            instr.inst += "BLTZ\tR" + to_string(instr.rs) + ", #" + to_string(instr.imm * 4 + addr + 4);
        } else if (instr.op == 0 && instr.func == 0) { //SLL
            instr.inst += "SLL\tR" + to_string(instr.rd) + ", R" + to_string(instr.rt) + ", #" + to_string(instr.shift);
        } else if (instr.op == 5) { // BNE
            instr.inst += to_string(instr.addr) + "\tBNE\tR" + to_string(instr.rs) + ", R" + to_string(instr.rt) + ", #" + to_string(instr.imm);
  } else if (instr.op == 0 && instr.func == 8) { // JR
             instr.inst += "JR\tR" + to_string(instr.rs);
        } else if (instr.op == 0 && instr.func == 2) { // SRL
             instr.inst += "SRL\tR" + to_string(instr.rd) + ", R" + to_string(instr.rt) + ", #" + to_string(instr.shift);
        } else if (instr.op == 28 && instr.func == 2) { // MUL
             instr.inst += "MUL\tR" + to_string(instr.rd) + ", R" + to_string(instr.rs) + ", R" + to_string(instr.rt);
        } else if (instr.op == 0 && instr.func == 10) { // MOVZ
             instr.inst += "MOVZ\tR" + to_string(instr.rd) + ", R" + to_string(instr.rs) + ", R" + to_string(instr.rt);
        } else if (instr.op == 0 && instr.func == 0 && instr.UI == 0) { // NOP
             instr.inst += "NOP";
        } else if (instr.op == 0 && instr.func == 13) { // BREAK
      instr.inst += "BREAK";
      breakEncountered = true;
        }	

        MEM[addr] = instr;
        disout << instr.inst << endl;
      }
       addr+=4;
     }
  }
  // simulation
  //
  int PC = 96;
  int R[32] = {0};
  int cycle = 1;
  string str;
  decodedInstruction I;
  breakEncountered = false;

  while(!breakEncountered) {
    I = MEM[PC];
    
    // Check if the current instruction is BREAK
    if (I.op == 0 && I.func == 13) {
      breakEncountered = true;
    }

    int nextPC = PC + 4; //next PC

    // Execute the instruction
    if (I.op == 8) { // ADDI
      R[I.rt] = R[I.rs] + I.imm;
    } else if (I.op == 43) { // SW
      MEM[R[I.rs] + I.imm].UI = R[I.rt];
    } else if (I.op == 35) { // LW
      R[I.rt] = MEM[R[I.rs] + I.imm].UI;
    } else if (I.op == 2) { // J
      nextPC = (I.UI & 0x3FFFFFF) << 2;
    } else if (I.op == 0 && I.func == 32) { // ADD
      R[I.rd] = R[I.rs] + R[I.rt];
    } else if (I.op == 0 && I.func == 34) { // SUB
      R[I.rd] = R[I.rs] - R[I.rt];
    } else if (I.op == 4) { // BEQ
      if (R[I.rs] == R[I.rt]) {
        nextPC = PC + 4 + (I.imm * 4);
      }
    } else if (I.op == 1 && I.rt == 0) { // BLTZ
      if (R[I.rs] < 0) {
        nextPC = PC + 4 + (I.imm * 4);
      }
    } else if (I.op == 0 && I.func == 0 && I.UI == 0) { // NOP
    } else if (I.op == 0 && I.func == 0) { // SLL
      R[I.rd] = R[I.rt] << I.shift;
    } else if (I.op == 5) { // BNE
      if (R[I.rs] != R[I.rt]) {
        nextPC = PC + 4 + (I.imm * 4);
      }
    } else if (I.op == 0 && I.func == 8) { // JR
      nextPC = R[I.rs];
    } else if (I.op == 0 && I.func == 2) { // SRL
      R[I.rd] = (unsigned int)R[I.rt] >> I.shift;
    } else if (I.op == 28 && I.func == 2) { // MUL
      R[I.rd] = R[I.rs] * R[I.rt];
    } else if (I.op == 0 && I.func == 10) { // MOVZ
      if (R[I.rt] == 0) {
        R[I.rd] = R[I.rs];
      }
    }

    str = "====================\ncycle:" + to_string(cycle) + "\t" + to_string(PC) + "\t\t" + I.inst.substr(I.inst.find("\t") + 1).substr(I.inst.substr(I.inst.find("\t") + 1).find("\t") + 1) + "\n\nregisters:";
    for (int i = 0; i < 32; i++) {
      if (i % 8 == 0)
        str += "\nr" + to_string(i < 10 ? i : i/10) + (i < 10 ? " " : to_string(i%10)) + ":\t";
      str += to_string(R[i]) + "\t";
    }

    // Display data memory in chunks
    str += "\n\ndata:";
    // Display from address 164 to 256+ in chunks of 8
    for (int addr = 164; addr < 260; addr += 32) {
      str += "\n" + to_string(addr) + ":\t";
      for (int i = 0; i < 32; i += 4) {
        str += to_string(static_cast<int>(MEM[addr + i].UI)) + ((i % 16 == 12) ? "\t" : "\t");
      }
    }

    simout << str << endl;
    cout << str << endl;

    PC = nextPC;
    cycle++;
  }

  close(FD);
  disout.close();
  simout.close();

  return 0;
}
