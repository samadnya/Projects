//svk310
//Cache simulator for Lab2
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <stdlib.h>
#include <cmath>
#include <bitset>

using namespace std;
//access state:
#define NA 0 // no action
#define RH 1 // read hit
#define RM 2 // read miss
#define WH 3 // Write hit
#define WM 4 // write miss

struct config{
       int L1blocksize;
       int L1setsize;
       int L1size;
       int L2blocksize;
       int L2setsize;
       int L2size;
       };
//Define a cache class
class cache {
public:
        
    unsigned long  b, s, t, rows; //To calculate the offset bits, set index and tag bits for L1 and L2
    vector<vector<unsigned long> > cache_table; //to allot space for the cache
    //vector <int> nsize;
    vector<vector<unsigned long> > count;
    int setsize;

    cache()
    {
    }
    cache(int blocksize, int setsize, int size) 
        {
            this->setsize = setsize;

        
            b = ( unsigned long )(log2(blocksize));  // gives the number of offset bits
            s = ( unsigned long ) log2((size) * (1024) / (blocksize * setsize)); //gives the number of index bits
            rows = ( unsigned long ) pow(2, s); //gives the number of rows needed to be indexed
            t= 32-s-b;  //gives the number of tag bits
            cache_table.resize(rows);  //vector::resize() can be used to automatically resize the vector for each different case
            count.resize(rows);
            for (int i = 0; i < rows; i++) 
                {
		            cache_table[i].resize(setsize);
                    count[i].resize(setsize);
                    for(int j=0; j<setsize; j++){
                        cache_table[i][j]=-1;  //to avoid false hits
                        count[i][j]= setsize-j-1;
                    }
                       
	            }
            
            
        }
    void calculate_param(bitset<32> address,  int& tag, int& index)  
        {
            
            index = (address.to_ulong()>>b) % rows;  //the modulo operator is used to identify the index
            tag = (address.to_ulong()>> s+b); //shift right by offset bits + index bits

            cout<<tag<<"\t"<<index<<endl;

        }  
    void read_fun(bitset <32> address, int & state)
        {  
           // cout<<"code is in read_fun"<<endl;
            int tag, index, offset, hit=0;
            calculate_param(address, tag, index);
            for(int i=0; i < setsize ; i++){   
                    //cout<<"code is in read_fun for loop"<<endl;
                    //cout<<index<<endl;
                    //cout<<cache_table[index][i]<<endl;
                    if (cache_table[index][i] == tag)
                    {
                        updateLRU(index, i);       // updates the LRU status for future reference
                        hit=1;  //set hit to 1 to indicate a read hit

                        cout<<"Hit index: "<<i<<endl;  
                        break;           
                    }
            }
            
            if (hit)
            {
                cout<<"read hit is called"<<endl;
                state=1;
            }
            else
            {
              //hit is set to 0 by default 
              int i = findLRU(index, tag); //to identify the LRU 
              cout<<"Replace index: "<<i<<endl; 
              cache_table[index][i]= tag;   
              cout<<"read miss"<<endl;
              state=0;
            }     
            
        }

void write_fun(bitset <32> address, int & state)
        {  
           // cout<<"code is in write_fun"<<endl;
            int tag, index, offset, hit=0;
            calculate_param(address, tag, index);
            for(int i=0; i < setsize ; i++){   
                    //cout<<index<<endl;
                    //cout<<cache_table[index][i]<<endl;
                    if (cache_table[index][i] == tag)
                    {
                        updateLRU(index,i);        
                        hit=1;  

                        cout<<"Hit index: "<<i<<endl;
                        break;           
                    }
            }
            
            if (hit)
            {
                cout<<"write hit "<<endl;
                state=1;
            }
            else
            {
                state=0;    
        
            }     
            
        } 


   void updateLRU(int index,int i)
   {
    int temp = count[index][i];
    for(int j=0;j<setsize;j++)
    {
        if(count[index][j]<temp)
            count[index][j]+=1;  // increment the count for everything else other than the most recently used 
    }
    count[index][i]=0;  //set the most recent current count to 0
   }

    int findLRU(int index, int tag)
    {
        int temp;
        for(int i=0;i<setsize;i++)
        {
            if(count[index][i]!=setsize-1)
                count[index][i]+=1;
            else 
            {
                temp =i;
                count[index][i]=0;
            }
        }
        return temp;
    }
        
    };
      



int main(int argc, char* argv[]){
    
    
    
    config cacheconfig;
    ifstream cache_params;
    string dummyLine;
    cache_params.open("cacheconfig.txt");
    while(!cache_params.eof())  // read config file
    {
      cache_params>>dummyLine;
      cache_params>>cacheconfig.L1blocksize;
      cache_params>>cacheconfig.L1setsize;              
      cache_params>>cacheconfig.L1size;
      cache_params>>dummyLine;              
      cache_params>>cacheconfig.L2blocksize;           
      cache_params>>cacheconfig.L2setsize;        
      cache_params>>cacheconfig.L2size;
      }



    cache obL1(cacheconfig.L1blocksize, cacheconfig.L1setsize, cacheconfig.L1size); // to instantiate class cache use object- obL1
    cache obL2(cacheconfig.L2blocksize, cacheconfig.L2setsize, cacheconfig.L2size); // to instantiate class cache use object- obL1
   
  int L1AcceState =0; // L1 access state variable, can be one of NA, RH, RM, WH, WM;
  int L2AcceState =0; // L2 access state variable, can be one of NA, RH, RM, WH, WM;

   
   
    ifstream traces;
    ofstream tracesout;
    string outname;
    outname = "trace_bigsvk.txt.out";
    
    traces.open("trace.txt");
    tracesout.open(outname.c_str());
    
    string line;
    string accesstype;  // the Read/Write access type from the memory trace;
    string xaddr;       // the address from the memory trace store in hex;
    unsigned int addr;  // the address from the memory trace store in unsigned int;        
    bitset<32> accessaddr; // the address from the memory trace store in the bitset;
    if(traces.is_open()&&tracesout.is_open()){    
        while (getline (traces,line)){   // read mem access file and access Cache
            
            istringstream iss(line); 
            if (!(iss >> accesstype >> xaddr)) {break;}
            stringstream saddr(xaddr);
            saddr >> std::hex >> addr;
            accessaddr = bitset<32> (addr);
           
           
           // access the L1 and L2 Cache according to the trace;
              if (accesstype.compare("R")==0)
              
             {    
                int hit;
                //Look up the L1 cache first   
                obL1.read_fun(accessaddr.to_ulong(), hit);

                if(hit==1)
                {
                    L1AcceState = 1;
                    L2AcceState = 0;
                }
                
                
                else
                {
                   
                   obL2.read_fun(accessaddr.to_ulong(), hit);
                   if(hit == 1)
                   {
                        L1AcceState = 2;
                        L2AcceState = 1;
                   }
                   else 
                   {
                       L1AcceState = 2;
                       L2AcceState = 2;
                   } 
                }
                 
                 
                }
             else 
             {    
            
                int hit;  
                obL1.write_fun(accessaddr.to_ulong(),hit);

                if(hit==1)
                {
                    L1AcceState = 3;
                }
                else
                {
                    L1AcceState = 4;
                } 
                  L2AcceState = 0;
                   obL2.write_fun(accessaddr.to_ulong(),hit);
                   if(hit ==1)
                   {
                        L2AcceState = 3;
                   }
                   else 
                   {
                        L1AcceState = 4;
                        L2AcceState = 4;
       
                   }
                  //cout<<"HELLO"<<endl;
                  
                  }
              
              
             
            tracesout<< L1AcceState << " " << L2AcceState << endl;  // Output hit/miss results for L1 and L2 to the output file;
             
             
        }
        traces.close();
        tracesout.close(); 
    }
    else cout<< "Unable to open trace or traceout file ";
   
    return 0;
}