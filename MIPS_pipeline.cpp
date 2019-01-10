//svk310 CSA LAB 1
// Five stage pipeline with RAW Hazards
//This code has been verified for all dependencies and holds true for Stalling and Branch type
//N13360675
#include<iostream>
#include<string>
#include<vector>
#include<bitset>
#include<fstream>
using namespace std;

#define MemSize 1000 // memory size, in reality, the memory size should be 2^32, but for this lab, for the space resaon, we keep it as this large number, but the memory is still 32-bit addressable.

struct IFStruct {
    bitset<32>  PC;
    bool        nop;
};

struct IDStruct {
    bitset<32>  Instr;
    bool        nop;
};

struct EXStruct {
    bitset<32>  Read_data1;
    bitset<32>  Read_data2;
    bitset<16>  Imm;
    bitset<5>   Rs;
    bitset<5>   Rt;
    bitset<5>   Wrt_reg_addr;
    bool        is_I_type;
    bool        rd_mem;
    bool        wrt_mem;
    bool        alu_op;     //1 for addu, lw, sw, 0 for subu
    bool        wrt_enable;
    bool        nop;
};

struct MEMStruct {
    bitset<32>  ALUresult;
    bitset<32>  Store_data;
    bitset<5>   Rs;
    bitset<5>   Rt;
    bitset<5>   Wrt_reg_addr;
    bool        rd_mem;
    bool        wrt_mem;
    bool        wrt_enable;
    bool        nop;
};

struct WBStruct {
    bitset<32>  Wrt_data;
    bitset<5>   Rs;
    bitset<5>   Rt;
    bitset<5>   Wrt_reg_addr;
    bool        wrt_enable;
    bool        nop;
};

struct stateStruct {
    IFStruct    IF;
    IDStruct    ID;
    EXStruct    EX;
    MEMStruct   MEM;
    WBStruct    WB;
};


class RF
{
    public:
        bitset<32> Reg_data;
     	RF()
    	{
			Registers.resize(32);
			Registers[0] = bitset<32> (0);
        }

        bitset<32> readRF(bitset<5> Reg_addr)
        {
            Reg_data = Registers[Reg_addr.to_ulong()];
            return Reg_data;
        }

        void writeRF(bitset<5> Reg_addr, bitset<32> Wrt_reg_data)
        {
            Registers[Reg_addr.to_ulong()] = Wrt_reg_data;
        }

		void outputRF()
		{
			ofstream rfout;
			rfout.open("RFresult.txt",std::ios_base::app);
			if (rfout.is_open())
			{
				rfout<<"State of RF:\t"<<endl;
				for (int j = 0; j<32; j++)
				{
					rfout << Registers[j]<<endl;
				}
			}
			else cout<<"Unable to open file";
			rfout.close();
		}

	private:
		vector<bitset<32> >Registers;
};

class INSMem
{
	public:
        bitset<32> Instruction;
        INSMem()
        {
			IMem.resize(MemSize);
            ifstream imem;
			string line;
			int i=0;
			imem.open("imem.txt");
			if (imem.is_open())
			{
				while (getline(imem,line))
				{
					IMem[i] = bitset<8>(line);
					i++;
				}
			}
            else cout<<"Unable to open file";
			imem.close();
		}

		bitset<32> readInstr(bitset<32> ReadAddress)
		{
			string insmem;
			insmem.append(IMem[ReadAddress.to_ulong()].to_string());
			insmem.append(IMem[ReadAddress.to_ulong()+1].to_string());
			insmem.append(IMem[ReadAddress.to_ulong()+2].to_string());
			insmem.append(IMem[ReadAddress.to_ulong()+3].to_string());
			Instruction = bitset<32>(insmem);		//read instruction memory
			return Instruction;
		}

    private:
        vector<bitset<8> > IMem;
};

class DataMem
{
    public:
        bitset<32> ReadData;
        DataMem()
        {
            DMem.resize(MemSize);
            ifstream dmem;
            string line;
            int i=0;
            dmem.open("dmem.txt");
            if (dmem.is_open())
            {
                while (getline(dmem,line))
                {
                    DMem[i] = bitset<8>(line);
                    i++;
                }
            }
            else cout<<"Unable to open file";
                dmem.close();
        }

        bitset<32> readDataMem(bitset<32> Address)
        {
			string datamem;
            datamem.append(DMem[Address.to_ulong()].to_string());
            datamem.append(DMem[Address.to_ulong()+1].to_string());
            datamem.append(DMem[Address.to_ulong()+2].to_string());
            datamem.append(DMem[Address.to_ulong()+3].to_string());
            ReadData = bitset<32>(datamem);		//read data memory
            return ReadData;
		}

        void writeDataMem(bitset<32> Address, bitset<32> WriteData)
        {
            DMem[Address.to_ulong()] = bitset<8>(WriteData.to_string().substr(0,8));
            DMem[Address.to_ulong()+1] = bitset<8>(WriteData.to_string().substr(8,8));
            DMem[Address.to_ulong()+2] = bitset<8>(WriteData.to_string().substr(16,8));
            DMem[Address.to_ulong()+3] = bitset<8>(WriteData.to_string().substr(24,8));
        }

        void outputDataMem()
        {
            ofstream dmemout;
            dmemout.open("dmemresult.txt");
            if (dmemout.is_open())
            {
                for (int j = 0; j< 1000; j++)
                {
                    dmemout << DMem[j]<<endl;
                }

            }
            else cout<<"Unable to open file";
            dmemout.close();
        }

    private:
		vector<bitset<8> > DMem;
};

void printState(stateStruct state, int cycle)
{
    ofstream printstate;
    printstate.open("stateresult.txt", std::ios_base::app);
    if (printstate.is_open())
    {
        printstate<<"State after executing cycle:\t"<<cycle<<endl;
        printstate<<"IF.PC:\t"<<state.IF.PC.to_ulong()<<endl;
        printstate<<"IF.nop:\t"<<state.IF.nop<<endl;
        printstate<<"ID.Instr:\t"<<state.ID.Instr<<endl;
        printstate<<"ID.nop:\t"<<state.ID.nop<<endl;
        printstate<<"EX.Read_data1:\t"<<state.EX.Read_data1<<endl;
        printstate<<"EX.Read_data2:\t"<<state.EX.Read_data2<<endl;
        printstate<<"EX.Imm:\t"<<state.EX.Imm<<endl;
        printstate<<"EX.Rs:\t"<<state.EX.Rs<<endl;
        printstate<<"EX.Rt:\t"<<state.EX.Rt<<endl;
        printstate<<"EX.Wrt_reg_addr:\t"<<state.EX.Wrt_reg_addr<<endl;
        printstate<<"EX.is_I_type:\t"<<state.EX.is_I_type<<endl;
        printstate<<"EX.rd_mem:\t"<<state.EX.rd_mem<<endl;
        printstate<<"EX.wrt_mem:\t"<<state.EX.wrt_mem<<endl;
        printstate<<"EX.alu_op:\t"<<state.EX.alu_op<<endl;
        printstate<<"EX.wrt_enable:\t"<<state.EX.wrt_enable<<endl;
        printstate<<"EX.nop:\t"<<state.EX.nop<<endl;
        printstate<<"MEM.ALUresult:\t"<<state.MEM.ALUresult<<endl;
        printstate<<"MEM.Store_data:\t"<<state.MEM.Store_data<<endl;
        printstate<<"MEM.Rs:\t"<<state.MEM.Rs<<endl;
        printstate<<"MEM.Rt:\t"<<state.MEM.Rt<<endl;
        printstate<<"MEM.Wrt_reg_addr:\t"<<state.MEM.Wrt_reg_addr<<endl;
        printstate<<"MEM.rd_mem:\t"<<state.MEM.rd_mem<<endl;
        printstate<<"MEM.wrt_mem:\t"<<state.MEM.wrt_mem<<endl;
        printstate<<"MEM.wrt_enable:\t"<<state.MEM.wrt_enable<<endl;
        printstate<<"MEM.nop:\t"<<state.MEM.nop<<endl;
        printstate<<"WB.Wrt_data:\t"<<state.WB.Wrt_data<<endl;
        printstate<<"WB.Rs:\t"<<state.WB.Rs<<endl;
        printstate<<"WB.Rt:\t"<<state.WB.Rt<<endl;
        printstate<<"WB.Wrt_reg_addr:\t"<<state.WB.Wrt_reg_addr<<endl;
        printstate<<"WB.wrt_enable:\t"<<state.WB.wrt_enable<<endl;
        printstate<<"WB.nop:\t"<<state.WB.nop<<endl;
    }
    else cout<<"Unable to open file";
    printstate.close();
}

bitset<32> signextend (bitset<16> imm)
{
    string sestring;
    if (imm[15]==0){
        sestring = "0000000000000000"+imm.to_string<char,std::string::traits_type,std::string::allocator_type>();
    }
    else{
        sestring = "1111111111111111"+imm.to_string<char,std::string::traits_type,std::string::allocator_type>();
    }
    return (bitset<32> (sestring));

}

int main()
{
    
    RF myRF;
    INSMem myInsMem;
    DataMem myDataMem;
    stateStruct state, newState;
    bitset<32> PC =0;
    bitset<1> IsEq;
    bitset<1>  IsBranch = 0;
    bitset<1>  Branch_taken = 0;
    bitset<32> Braddr = 0;
    bool stall ;
 //set the nop bits of last 4 stages to fetch the first instruction
    state.IF.nop=0;
    state.ID.nop=1;
    state.EX.nop=1;
    state.MEM.nop=1;
    state.WB.nop=1;
//initialize the flip flop states to zero in order to avoid garbage values
    newState.EX.Rs = 0;
    newState.EX.Rt = 0;
    newState.EX.Wrt_reg_addr = 0;
    newState.EX.Read_data1 = 0;
    newState.EX.Read_data2 = 0;
    newState.EX.is_I_type = 0;
    newState.EX.rd_mem = 0;
    newState.EX.wrt_mem = 0;
    newState.EX.alu_op = 1;
    newState.EX.wrt_enable = 0;
    newState.EX.Imm = 0;
    newState.MEM.ALUresult = 0;
    newState.MEM.Rs = 0;
    newState.MEM.Rt = 0;
    newState.MEM.Wrt_reg_addr = 0;
    newState.MEM.wrt_enable = 0;
    newState.MEM.Store_data= 0;
    newState.MEM.rd_mem = 0;
    newState.MEM.wrt_mem = 0;
    newState.WB.Wrt_data = 0;
    newState.WB.Rs = 0;
    newState.WB.Rt = 0;
    newState.WB.Wrt_reg_addr = 0;
    newState.WB.wrt_enable = 0;
    //start at cycle = 0
    int cycle = 0;
    bool branch_taken;
    //int i;
   while(1)
  
    {

        /* --------------------- WB stage --------------------- */
         if(state.WB.nop)
        {
            newState.WB.nop = state.WB.nop;
        }
        
        else
        {
            if (state.WB.wrt_enable)
            {
                myRF.writeRF(state.WB.Wrt_reg_addr, state.WB.Wrt_data);
            }
            
        }


        /* --------------------- MEM stage --------------------- */
        if(state.MEM.nop)
        {
                newState.WB.nop=1;
        }
        else
        {
            if(state.MEM.rd_mem)
            {
                newState.WB.Wrt_data = myDataMem.readDataMem(state.MEM.ALUresult);
                newState.WB.Rs = state.MEM.Rs;
                newState.WB.Wrt_reg_addr = state.MEM.Wrt_reg_addr;
                newState.WB.Rt = state.MEM.Rt;
                newState.WB.wrt_enable = state.MEM.wrt_enable;
                newState.WB.nop = state.MEM.nop;
            }
            else if (state.MEM.wrt_mem)
            {
                
                myDataMem.writeDataMem(state.MEM.ALUresult, state.MEM.Store_data);
                //newState.WB.Wrt_data = state.MEM.ALUresult;
                newState.WB.wrt_enable = state.MEM.wrt_enable;
                newState.WB.Rs = state.MEM.Rs;
                newState.WB.Wrt_reg_addr = state.MEM.Wrt_reg_addr;
                newState.WB.Rt = state.MEM.Rt;
                newState.WB.nop = newState.MEM.nop; // verify this step later
                newState.WB.Wrt_data = state.WB.Wrt_data;
            }
            else
            {
                newState.WB.Wrt_data = state.MEM.ALUresult;
                newState.WB.Rs = state.MEM.Rs;
                newState.WB.Rt = state.MEM.Rt;
                newState.WB.Wrt_reg_addr = state.MEM.Wrt_reg_addr;
                newState.WB.wrt_enable = state.MEM.wrt_enable;
                newState.WB.nop = state.MEM.nop;
            }

        }


        /* --------------------- EX stage --------------------- */
        if(state.EX.nop)
        {
            newState.MEM.nop=1;
        }
        else
        {   
            newState.MEM.wrt_enable = state.EX.wrt_enable;
            newState.MEM.rd_mem = state.EX.rd_mem;
            newState.MEM.wrt_mem = state.EX.wrt_mem ;
            newState.MEM.Rs = state.EX.Rs;
            newState.MEM.Rt = state.EX.Rt;
            newState.MEM.Wrt_reg_addr = state.EX.Wrt_reg_addr;
            newState.MEM.Store_data = state.EX.Read_data2;
            if(state.EX.alu_op)
            {
                if (state.EX.rd_mem)   
                {
                    newState.MEM.ALUresult = state.EX.Read_data1.to_ulong() + signextend(state.EX.Imm).to_ulong();
                }
                else if (state.EX.wrt_mem)
                {
                    newState.MEM.ALUresult = state.EX.Read_data1.to_ulong() + signextend(state.EX.Imm).to_ulong();
                }
                else
                {
                    newState.MEM.ALUresult= state.EX.Read_data1.to_ulong() + state.EX.Read_data2.to_ulong();
                }
            }
            else if (!state.EX.alu_op)  // alu_op is 0 for subu instructions
            {
               newState.MEM.ALUresult = state.EX.Read_data1.to_ulong() - state.EX.Read_data2.to_ulong();
            }
            //ADD-STORE dependency
            if((newState.MEM.wrt_enable==0 && newState.MEM.rd_mem==0 && newState.MEM.wrt_mem==1)&& (state.MEM.wrt_enable && state.MEM.rd_mem==0 && state.MEM.wrt_mem==0))
            {
                if(state.MEM.Wrt_reg_addr==newState.MEM.Rt )  //send the data from the parent to child instruction
                {
                   newState.MEM.Store_data = newState.WB.Wrt_data; 
                  //newState.MEM.ALUresult = state.MEM.ALUresult;  //MEM to MEM forwarding
                }
            }
            //LOAD-STORE dependency
            if((newState.MEM.wrt_enable==0 && newState.MEM.rd_mem==0 && newState.MEM.wrt_mem==1)&& (state.MEM.wrt_enable && state.MEM.rd_mem==1 && state.MEM.wrt_mem==0))
            {
                if(state.MEM.Wrt_reg_addr==newState.MEM.Rt )  // send the data from the parent to child instruction
                {
                  newState.MEM.Store_data = newState.WB.Wrt_data;  //MEM to MEM forwarding
                }
            }
            newState.MEM.nop = state.EX.nop; 
        }
        /* --------------------- ID stage --------------------- */
        if(state.ID.nop)
        {
            newState.EX.nop = 1;
        }
        else
        {
            
            if(state.ID.Instr.to_string<char,std::string::traits_type,std::string::allocator_type>() == "11111111111111111111111111111111")
            {

            }
            else
            {
            if (((state.ID.Instr.to_ulong()>>26)&0x3F)==000000)
              {
                 newState.EX.Rs = ((state.ID.Instr.to_ulong()>>21)&0x1F);
                 newState.EX.Rt = ((state.ID.Instr.to_ulong()>>16)&0x1F);
                 newState.EX.Wrt_reg_addr = ((state.ID.Instr.to_ulong()>>11)&0x1F);
                 newState.EX.Read_data1 = myRF.readRF(newState.EX.Rs);
                 newState.EX.Read_data2 = myRF.readRF(newState.EX.Rt);
                 newState.EX.Imm = ((state.ID.Instr.to_ulong()>>0)&0xFFFF);
                 newState.EX.is_I_type = 0;
                 newState.EX.wrt_enable = 1;
                 newState.EX.rd_mem = 0;
                 newState.EX.wrt_mem = 0;                 
                 if (((state.ID.Instr.to_ulong()>>0)&0x3F)==0x21)   //check funct for addu
                 {
                    newState.EX.alu_op = 1;
                    
                    newState.EX.nop = state.ID.nop;
                 }
                 else if (((state.ID.Instr.to_ulong()>>0)&0x3F)==0x23) //check funct for subu
                 {
                     newState.EX.alu_op = 0;
                     
                     newState.EX.nop = state.ID.nop;
                 }
              }
            else
               {
                 newState.EX.is_I_type = 1;
                 newState.EX.Rs = ((state.ID.Instr.to_ulong()>>21)&0x1F);
                 newState.EX.Rt = ((state.ID.Instr.to_ulong()>>16)&0x1F);
                 newState.EX.Wrt_reg_addr = ((state.ID.Instr.to_ulong()>>16)&0x1F);
                 newState.EX.Read_data1 = myRF.readRF(newState.EX.Rs);
                 newState.EX.Read_data2 = myRF.readRF(newState.EX.Wrt_reg_addr); 
                 newState.EX.Imm = ((state.ID.Instr.to_ulong()>>0)&0xFFFF);
                 
                    IsEq = (newState.EX.Read_data1.to_ulong() != newState.EX.Read_data2.to_ulong())?1:0;

                    IsBranch = (((state.ID.Instr.to_ulong()>>26)&0x3F) == 0x04)?1:0;
                   // if(IsBranch.to_ulong() && IsEq.to_ulong())    //check branch condition
                    if (IsBranch==1)
                    {
                    if(newState.EX.Rs != newState.EX.Rt )    //check branch condition
                   
                       { cout << "Read Data_1 in cycle"<< cycle << endl<<newState.EX.Read_data1<<endl;
                       	 cout << "Read Data_2 in cycle"<< cycle << endl<<newState.EX.Read_data1<<endl;

                       	   if(newState.EX.Read_data1 != newState.EX.Read_data2)
                            {
                       		Branch_taken=1;
                            //Braddr = bitset<32>(signextend(newState.EX.Imm).to_ulong());// & 0xFFFFFFFC);  //compute the addr to be added to PC + 4
                            Braddr = signextend(newState.EX.Imm);
                            //cout<<Braddr<<endl;
                            Braddr <<= 2; //compute the addr to be added to PC + 4
                            newState.ID.nop=1;
                            newState.EX.wrt_enable = 0;
                            newState.EX.rd_mem = 0;
                            newState.EX.wrt_mem = 0;  
                            newState.EX.alu_op = 1;
                            cout<< "branch taken in cycle"<<cycle<<endl <<Braddr<< endl;
                            cout<< "state.IF.PC"<< state.IF.PC<<endl;
                            newState.EX.is_I_type = 0;
                            }
                       	   else {
                       		newState.EX.wrt_enable = 0;
                       		newState.EX.rd_mem = 0;
                       		newState.EX.wrt_mem = 0;
                       		newState.EX.alu_op = 1;
                       		newState.EX.is_I_type = 0;
                       	   }
                       }
                    }
                 else if (((state.ID.Instr.to_ulong()>>26)&0x3F) == 0x23)  // check opcode for lw
                   {
                     newState.EX.wrt_enable = 1;
                     newState.EX.rd_mem = 1;
                     newState.EX.wrt_mem = 0;
                     newState.EX.alu_op = 1;
                     newState.EX.nop = state.ID.nop;
                   }
                 else if (((state.ID.Instr.to_ulong()>>26)&0x3F) == 0x2B)  //check opcode for sw
                   {
                     newState.EX.wrt_enable = 0;
                     newState.EX.rd_mem = 0;
                     newState.EX.wrt_mem = 1;
                     newState.EX.alu_op = 1;
                     newState.EX.nop = state.ID.nop;
                   }
                }
            }  
            //Check the code for RAW dependencies
            //ADD-ADD dependency independent instruction in between
            if((newState.EX.wrt_enable && newState.EX.rd_mem==0 && newState.EX.wrt_mem==0)&& (state.MEM.wrt_enable && state.MEM.rd_mem==0 && state.MEM.wrt_mem==0))
             {

                if(state.MEM.Wrt_reg_addr==newState.EX.Rs )  //if rs_ex == wb_rd then send the data from the parent to child instruction
                {
                  newState.EX.Read_data1 = state.MEM.ALUresult;  //MEM to Ex forwarding
                }
                if(state.MEM.Wrt_reg_addr==newState.EX.Rt )  //if rs_ex == wb_rd then send the data from the parent to child instruction
                {
                  newState.EX.Read_data2 = state.MEM.ALUresult;  //MEM to Ex forwarding
                }
            }
            //ADD-ADD dependency consecutive instruction
            if((newState.EX.wrt_enable && newState.EX.rd_mem==0 && newState.EX.wrt_mem==0) && (state.EX.wrt_enable && state.EX.rd_mem==0 && state.EX.wrt_mem==0))
            {
                if(state.EX.Wrt_reg_addr==newState.EX.Rs)
                {
                  newState.EX.Read_data1 = newState.MEM.ALUresult; // EX to EX forwarding
                }
                if(state.EX.Wrt_reg_addr==newState.EX.Rt)
                {
                  newState.EX.Read_data2 = newState.MEM.ALUresult; // EX to EX forwarding
                }
            }
            //ADD-LOAD dependency independent instruction in bewtween
            if((newState.EX.wrt_enable && newState.EX.rd_mem==1 && newState.EX.wrt_mem==0)&& (state.MEM.wrt_enable && state.MEM.rd_mem==0 && state.MEM.wrt_mem==0))
             {

                if(state.MEM.Wrt_reg_addr==newState.EX.Rs )  //if rs_ex == wb_rd then send the data from the parent to child instruction
                {
                  newState.EX.Read_data1 =newState.WB.Wrt_data;  //MEM to Ex forwarding
                }
                if(state.MEM.Wrt_reg_addr==newState.EX.Rt )  //if rs_ex == wb_rd then send the data from the parent to child instruction
                {
                  newState.EX.Read_data2 =newState.WB.Wrt_data;  //MEM to Ex forwarding
                }
            }
            //ADD-LOAD dependency consecutive instruction
            if((newState.EX.wrt_enable && newState.EX.rd_mem==1 && newState.EX.wrt_mem==0) && (state.EX.wrt_enable && state.EX.rd_mem==0 && state.EX.wrt_mem==0))
            {
                if(state.EX.Wrt_reg_addr==newState.EX.Rs)
                {
                  newState.EX.Read_data1 = newState.MEM.ALUresult; // EX to EX forwarding
                }
                if(state.EX.Wrt_reg_addr==newState.EX.Rt)
                {
                  newState.EX.Read_data2 = newState.MEM.ALUresult; // EX to EX forwarding
                }
            }
            //LOAD-ADD dependency independent instructionn in between
            if((newState.EX.wrt_enable && newState.EX.rd_mem==0 && newState.EX.wrt_mem==0)&& (state.MEM.wrt_enable && state.MEM.rd_mem==1 && state.MEM.wrt_mem==0))
            {

                if(state.MEM.Wrt_reg_addr==newState.EX.Rs )  //if rs_ex == wb_rd then send the data from the parent to child instruction
                {
                  newState.EX.Read_data1 = newState.WB.Wrt_data;//edited from state.MEM.ALUresult;  //MEM to Ex forwarding
                }
                if(state.MEM.Wrt_reg_addr==newState.EX.Rt )  //if rs_ex == wb_rd then send the data from the parent to child instruction
                {
                  newState.EX.Read_data2 = newState.WB.Wrt_data;  //MEM to Ex forwarding
                }
            }


            //Stalling operation for LOAD ADD dependency consecutive instructions
            if((newState.EX.wrt_enable && newState.EX.rd_mem==0 && newState.EX.wrt_mem==0) && (state.EX.wrt_enable && state.EX.rd_mem==1 && state.EX.wrt_mem==0))
            {
               if(state.EX.Wrt_reg_addr==newState.EX.Rs)
                {
                    stall=1; // EX to EX forwarding
                    newState.EX.nop=1;   
        
                }
               if(state.EX.Wrt_reg_addr==newState.EX.Rt)
                {
                    stall=1; // EX to EX forwarding
                    newState.EX.nop=1;

                }
              
            }
        }

        /* --------------------- IF stage --------------------- */
        if(state.IF.nop)
        {
        goto mylabel;
        }

        if (state.IF.nop==0 && !stall && Branch_taken==0)
        {
            newState.ID.Instr = myInsMem.readInstr(state.IF.PC);  
            newState.IF.PC = (state.IF.PC.to_ulong() + 4);  //to fetch the next instruction
            newState.IF.nop = state.IF.nop; 
            newState.ID.nop = state.IF.nop;   //to propagate nop
        }

        // else if(state.IF.PC == 32)
        // { 
        //     newState.ID.Instr = myInsMem.readInstr(state.IF.PC);
        //     newState.IF.PC = (state.IF.PC.to_ulong()+0); 
        //     newState.IF.nop = 1; 
        //     newState.ID.nop = 1; 
        // }

        if(Branch_taken== 1)
        {
            
            newState.IF.PC = (state.IF.PC.to_ulong() + Braddr.to_ulong());
            Branch_taken=0;
        }
        if (stall)
        {   
            newState.IF = state.IF;
            newState.ID = state.ID;
            stall=0;
        }
    	if (newState.ID.Instr.to_ulong() == 4294967295)  // checking for halt condition 
    	{
    	newState.ID.nop = 1;
    	newState.IF.nop = 1;
    	newState.IF.PC=state.IF.PC;
    	goto mylabel;
   		}
       
        mylabel:
        if (state.IF.nop && state.ID.nop && state.EX.nop && state.MEM.nop && state.WB.nop)
                    break;
        printState(newState, cycle); //print states after executing cycle 0, cycle 1, cycle 2 ...
        cycle ++;
        state = newState; /*The end of the cycle and updates the current state with the values calculated in this cycle */

    }

    myRF.outputRF(); // dump RF;
	myDataMem.outputDataMem(); // dump data mem

	return 0;
}
