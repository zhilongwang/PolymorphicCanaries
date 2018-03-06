#include <iostream>
#include <cstdio>
#include <cstring>
#include <regex>

// DyninstAPI
#include "BPatch.h"
#include "BPatch_basicBlock.h"
#include "BPatch_flowGraph.h"
#include "BPatch_function.h"
#include "BPatch_object.h"
#include "BPatch_point.h"
#include "BPatch_snippet.h"

//PatchAPI
#include "PatchMgr.h"
#include "PatchModifier.h"
#include "Point.h"
#include "Snippet.h"
#include "Buffer.h"

using namespace std;
using namespace Dyninst;
using namespace Dyninst::InstructionAPI;
using namespace Dyninst::PatchAPI;

vector<Point *> pts;
BPatch *bpatch = new BPatch;
BPatch_image *appImage;
PatchMgrPtr mgr;
SnippetPtr snippet;


// LD_PRELOAD=libredundantguard.so

class StackChk : public Snippet
{
  public:
    virtual bool generate(Point *pt, Buffer &buf)
    {
        buf.copy((void *)code, sizeof(code)-1);
        return true;
    }

    char code[27] = "\x50"                   	//push   %rax
                    "\x52"                   	//push   %rdx
                    "\x64\x8b\x04\x25\x28\x00\x00\x00" 	//mov    %fs:0x28,%eax
                    "\x48\x89\xfa"             	        //mov    %rdi,%rdx
                    "\x48\xc1\xea\x20"          	    //shr    $0x20,%rdx
                    "\x31\xd7"                	//xor    %edx,%edi
                    "\x39\xc7"                	//cmp    %eax,%edi
                    "\x75\x03"                	//jne    4004f4 <next>
                    "\x5a"                   	//pop    %rdx
                    "\x58"                   	//pop    %rax
                    "\xc3";                   	//retq   
  
};

class ForkInst : public Snippet
{
public:
    virtual bool generate(Point *pt, Buffer &buf)
    {
        buf.copy((void *)code, sizeof(code)-1);
        return true;
    }

    char code[43] = "\x85\xc0"               //test %rax, %rax
                    "\x75\x26"              //jmp
                    "\x50"                   //push   %rax
                    "\x53"                   //push   %rbx
                    "\x64\x48\x8b\x04\x25\x28\x00\x00\x00" 	//mov    %fs:0x28,%rax
                    "\x48\x0f\xc7\xf3"          		    //rdrand %rbx
                    "\x48\x31\xd8"            		        //xor    %rax,%rbx
                    "\x64\x48\x89\x1c\x25\xa8\x02\x00\x00" 	//mov    %rbx,%fs:0x2a8
                    "\x64\x48\x89\x04\x25\xb0\x02\x00\x00"	//mov    %rax,%fs:0x2b0
                    "\x5b"                   		//pop    %rbx
                    "\x58";                   		//pop    %rax
};

class InitInst : public Snippet
{
public:
    virtual bool generate(Point *pt, Buffer &buf)
    {
        buf.copy((void *)code, sizeof(code)-1);
        return true;
    }

    char code[39] = "\x50"                   //push   %rax
                    "\x53"                   //push   %rbx
                    "\x64\x48\x8b\x04\x25\x28\x00\x00\x00" 	//mov    %fs:0x28,%rax
                    "\x48\x0f\xc7\xf3"          		    //rdrand %rbx
                    "\x48\x31\xd8"            		        //xor    %rax,%rbx
                    "\x64\x48\x89\x1c\x25\xa8\x02\x00\x00" 	//mov    %rbx,%fs:0x2a8
                    "\x64\x48\x89\x04\x25\xb0\x02\x00\x00"	//mov    %rax,%fs:0x2b0
                    "\x5b"                   		//pop    %rbx
                    "\x58";                   		//pop    %rax
};


void finishInstrumenting(BPatch_addressSpace *app, const char *newName)
{
    BPatch_binaryEdit *appBin = dynamic_cast<BPatch_binaryEdit *>(app);

    if (!appBin)
    {
        fprintf(stderr, "appBin not defined!\n");
        return;
    }

    // Write binary to file
    if (!appBin->writeFile(newName))
    {
        fprintf(stderr, "writeFile failed\n");
    }
}

void writeStackChk(PatchFunction *func)
{
    // const PatchFunction::Blockset &blks = func->blocks();
    // char bytes[1024];
    cout << func->name() << endl;
    mgr->findPoint(
        Location::Function(func),
        Point::FuncEntry,
        back_inserter(pts));
    
    snippet =  StackChk::create(new StackChk());
    InstancePtr instptr = pts.back()->pushBack(snippet);
}

void writeInit(PatchFunction *func)
{
    // const PatchFunction::Blockset &blks = func->blocks();
    // char bytes[1024];
    cout << func->name() << endl;
    mgr->findPoint(
        Location::Function(func),
        Point::FuncEntry,
        back_inserter(pts));
    
    snippet =  InitInst::create(new InitInst());
    InstancePtr instptr = pts.back()->pushBack(snippet);
}

void writeFork(PatchFunction *func)
{
    cout << func->name() << endl;
    const PatchFunction::Blockset &blks = func->blocks();
    char bytes[1024];

    for (auto it = blks.begin(); it != blks.end(); it++)
    {
        PatchBlock *block = *it;
        PatchBlock::Insns insns;
        block->getInsns(insns);

        for (auto j = insns.begin(); j != insns.end(); j++)
        {
            // get instruction bytes
            Address addr = (*j).first;
            Instruction::Ptr iptr = (*j).second;
            size_t nbytes = iptr->size();
            assert(nbytes < 1024);
            for (size_t i=0; i<nbytes; i++) {
                bytes[i] = iptr->rawByte(i);
            }

            // print_hex(bytes);
            if ((bytes[0]&0xff) == 0xc3)
            {
                cout << "find ret" << endl;
                mgr->findPoint(
                    Location::InstructionInstance(func, block, addr),
                    Point::PreInsn,
                    back_inserter(pts));

                snippet =  ForkInst::create(new ForkInst());
                InstancePtr instptr = pts.back()->pushBack(snippet);
            }
        }
    }
}

int main(int argc, char **argv)
{
    BPatch_addressSpace *app = bpatch->openBinary(argv[1]);
    if (app == NULL)
    {
        return 1;
    }
    mgr = convert(app);
    appImage = app->getImage();
    Patcher patcher(mgr);

    vector<BPatch_module *> *modules = appImage->getModules();
    vector<BPatch_function *> *functions;
    

    for (auto it = modules->begin(); it != modules->end(); ++it)
    {
        BPatch_module * module = *it;
        functions = module->getProcedures();
    }

    for (auto it = functions->begin(); it != functions->end(); ++it)
    {
        PatchFunction *func = convert(*it);
        // cout << func->name() << endl;
        if(func->name() == "main"){
            writeInit(func);
        }
        if(func->name() == "__stack_chk_fail"){
            writeStackChk(func);
        }
        if(func->name() == "fork"){
            writeFork(func);
        }
    }

    patcher.commit();
    cout << pts.size() << " inst points" << endl;

    string name = string(argv[1]);
    name += "_inst";
    finishInstrumenting(app, name.c_str());
    cout << "Instrumentation Success!" << endl;

    return 0;
}
