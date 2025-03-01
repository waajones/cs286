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
    int addr = 96;
    string inst, binNoSpace;
  };
  int amt = 4;
  int addr = 96;
  unordered_map< int, decodedInstruction> MEM;

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
      instr.imm = instr.UI & 0xFFFF;
      instr.binNoSpace = bitset<32>(i).to_string();
      instr.inst += instr.binNoSpace + "\t";

      if (instr.op == 8) { // ADDI
        instr.inst += to_string(instr.addr) + "\tADDI\tR" + to_string(instr.rt) + ", R" + to_string(instr.rs) + ", #" + to_string(instr.imm);
      } else if (instr.op == 43) { // SW
        instr.inst += to_string(instr.addr) + "\tSW\tR" + to_string(instr.rt) + ", " + to_string(instr.imm) + "(R" + to_string(instr.rs) + ")";
      } else if (instr.op == 35) { // LW
        instr.inst += to_string(instr.addr) + "\tLW\tR" + to_string(instr.rt) + ", " + to_string(instr.imm) + "(R" + to_string(instr.rs) + ")";
      } else if (instr.op == 2) { // J
        instr.inst += to_string(instr.addr) + "\tJ\t#" + to_string((instr.UI & 0x3FFFFFF) << 2);
      } else if (instr.op == 0 && instr.func == 32) { // ADD
        instr.inst += to_string(instr.addr) + "\tADD\tR" + to_string(instr.rd) + ", R" + to_string(instr.rs) + ", R" + to_string(instr.rt);
      } else if (instr.op == 0 && instr.func == 34) { // SUB
        instr.inst += to_string(instr.addr) + "\tSUB\tR" + to_string(instr.rd) + ", R" + to_string(instr.rs) + ", R" + to_string(instr.rt);
      } else if (instr.op == 4) { // BEQ
        instr.inst += to_string(instr.addr) + "\tBEQ\tR" + to_string(instr.rs) + ", R" + to_string(instr.rt) + ", #" + to_string(instr.imm);
      } else if (instr.op == 5) { // BNE
        instr.inst += to_string(instr.addr) + "\tBNE\tR" + to_string(instr.rs) + ", R" + to_string(instr.rt) + ", #" + to_string(instr.imm);
      } else if (instr.op == 0 && instr.func == 13) { // BREAK
	instr.inst += to_string(instr.addr) + "\tBREAK";
    }
      MEM[addr] = instr;
      disout << instr.inst << endl;
      addr += 4;
    }
  }

  // simulation
  //
  int PC = 96;
  int R[32] = {0};
  int cycle = 1;
  string str;
  decodedInstruction I;
  while( true ){
    if( cycle > 144 ) exit(0);
    I = MEM[PC];

    if( I.op == 8 ) {  // ADDI
      R[I.rt] = R[I.rs] + I.imm;
    } else if( I.op ==43 ) {  //SW
      MEM[R[I.rs] + I.imm ].I = R[I.rt];
    } else if (I.op == 35) { // LW
      R[I.rt] = MEM[R[I.rs] + I.imm].UI;
    } else if (I.op == 2) { // J
      PC = (I.UI & 0x3FFFFFF) << 2;
      continue;
    }
    str = "====================\ncycle:" + to_string( cycle ) + " " + to_string(PC)
      + "\n\nregisters:";
    for( int i = 0; i < 32; i++ ){
      if( i % 8 == 0 )
        str += "\nr" + to_string(i) + ":";
      str += "\t" + to_string( R[i] );
    }
    str += "\n\ndata: " + to_string( MEM[256].I );
    cout << str << endl;
    PC+=4;
    cycle++;
  }


  close(FD);
  disout.close();
  simout.close();

  return 0;

}
