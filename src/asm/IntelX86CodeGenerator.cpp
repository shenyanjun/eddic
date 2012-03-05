//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <iostream>

#include "AssemblyFileWriter.hpp"
#include "FunctionContext.hpp"
#include "FunctionTable.hpp"
#include "Labels.hpp"
#include "VisitorUtils.hpp"

#include "asm/IntelStatementCompiler.hpp"
#include "asm/IntelX86CodeGenerator.hpp"
#include "asm/IntelAssemblyUtils.hpp"

using namespace eddic;

as::IntelX86CodeGenerator::IntelX86CodeGenerator(AssemblyFileWriter& w) : IntelCodeGenerator(w) {}

namespace x86 {

enum class Register : unsigned int {
    EAX,
    EBX,
    ECX,
    EDX,

    ESP, //Extended stack pointer
    EBP, //Extended base pointer

    ESI, //Extended source index
    EDI, //Extended destination index
    
    REGISTER_COUNT  
};

enum class FloatRegister : unsigned int {
    XMM0,
    XMM1,
    XMM2,
    XMM3,
    XMM4,
    XMM5,
    XMM6,
    XMM7,

    REGISTER_COUNT
};

std::string regToString(Register reg){
    static std::string registers[(int) Register::REGISTER_COUNT] = {"eax", "ebx", "ecx", "edx", "esp", "ebp", "esi", "edi"};

    return registers[(int) reg];
}

std::string regToString(FloatRegister reg){
    static std::string registers[(int) FloatRegister::REGISTER_COUNT] = {"xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"};

    return registers[(int) reg];
}

void enterFunction(AssemblyFileWriter& writer){
    writer.stream() << "push ebp" << std::endl;
    writer.stream() << "mov ebp, esp" << std::endl;
}

void defineFunction(AssemblyFileWriter& writer, const std::string& function){
    writer.stream() << std::endl << function << ":" << std::endl;
    
    enterFunction(writer);
}

void leaveFunction(AssemblyFileWriter& writer){
    writer.stream() << "leave" << std::endl;
    writer.stream() << "ret" << std::endl;
}

} //end of x86 namespace

using namespace x86;

namespace eddic { namespace as {

struct IntelX86StatementCompiler : public IntelStatementCompiler<Register, FloatRegister>, public boost::static_visitor<> {
    IntelX86StatementCompiler(AssemblyFileWriter& w, std::shared_ptr<tac::Function> f) : IntelStatementCompiler(w, 
        {Register::EDI, Register::ESI, Register::ECX, Register::EDX, Register::EBX, Register::EAX}, 
        {FloatRegister::XMM0, FloatRegister::XMM1, FloatRegister::XMM2, FloatRegister::XMM3, FloatRegister::XMM4, FloatRegister::XMM5, FloatRegister::XMM6, FloatRegister::XMM7}, f) {}
    
    std::string getMnemonicSize(){
        return "dword";
    }

    Register getReturnRegister1(){
        return Register::EAX;
    }

    Register getReturnRegister2(){
        return Register::EBX;
    }

    Register getBasePointerRegister(){
        return Register::EBP;
    }

    Register getStackPointerRegister(){
        return Register::ESP;
    }
  
    //Div eax by arg2 
    void divEax(std::shared_ptr<tac::Quadruple> quadruple){
        writer.stream() << "mov edx, eax" << std::endl;
        writer.stream() << "sar edx, 31" << std::endl;

        if(isInt(*quadruple->arg2)){
            auto reg = getReg();
            move(*quadruple->arg2, reg);

            writer.stream() << "idiv " << reg << std::endl;

            if(registers.reserved(reg)){
                registers.release(reg);
            }
        } else {
            writer.stream() << "idiv " << arg(*quadruple->arg2) << std::endl;
        }
    }
    
    void div(std::shared_ptr<tac::Quadruple> quadruple){
        spills(Register::EDX);
        registers.reserve(Register::EDX);

        //Form x = x / y
        if(*quadruple->arg1 == quadruple->result){
            safeMove(quadruple->result, Register::EAX);

            divEax(quadruple);
            //Form x = y / z (y: variable)
        } else if(isVariable(*quadruple->arg1)){
            spills(Register::EAX);
            registers.reserve(Register::EAX);

            copy(boost::get<std::shared_ptr<Variable>>(*quadruple->arg1), Register::EAX);

            divEax(quadruple);

            registers.release(Register::EAX);
            registers.setLocation(quadruple->result, Register::EAX);
        } else {
            spills(Register::EAX);
            registers.reserve(Register::EAX);

            copy(*quadruple->arg1, Register::EAX);

            divEax(quadruple);

            registers.release(Register::EAX);
            registers.setLocation(quadruple->result, Register::EAX);
        }

        registers.release(Register::EDX);
    }
    
    void mod(std::shared_ptr<tac::Quadruple> quadruple){
        spills(Register::EAX);
        spills(Register::EDX);

        registers.reserve(Register::EAX);
        registers.reserve(Register::EDX);

        copy(*quadruple->arg1, Register::EAX);

        divEax(quadruple);

        //result is in edx (no need to move it now)
        registers.setLocation(quadruple->result, Register::EDX);

        registers.release(Register::EAX);
    }
    
    void operator()(std::shared_ptr<tac::Quadruple>& quadruple){
        compile(quadruple);
    }
    
    void operator()(std::shared_ptr<tac::IfFalse>& ifFalse){
        compile(ifFalse);
    }

    void operator()(std::shared_ptr<tac::If>& if_){
        compile(if_);
    }

    void operator()(std::shared_ptr<tac::Goto>& goto_){
        compile(goto_);
    }

    void operator()(std::shared_ptr<tac::Call>& call){
        compile(call);
    }

    void operator()(tac::NoOp&){
        //It's a no-op
    }

    void operator()(std::string&){
        assert(false); //There is no more label after the basic blocks have been extracted
    }
};

}} //end of eddic::as

namespace { //anonymous namespace

void compile(AssemblyFileWriter& writer, std::shared_ptr<tac::BasicBlock> block, as::IntelX86StatementCompiler& compiler){
    compiler.reset();

    if(compiler.blockUsage.find(block) != compiler.blockUsage.end()){
        writer.stream() << block->label << ":" << std::endl;
    }

    for(unsigned int i = 0; i < block->statements.size(); ++i){
        auto& statement = block->statements[i];

        if(i == block->statements.size() - 1){
            compiler.setLast(true);
        } else {
            compiler.setNext(block->statements[i+1]);
        }
        
        visit(compiler, statement);
    }

    //If the basic block has not been ended
    if(!compiler.ended){
        compiler.endBasicBlock();
    }
}
 
} //end of anonymous space

namespace eddic { namespace as {

void IntelX86CodeGenerator::compile(std::shared_ptr<tac::Function> function){
    defineFunction(writer, function->getName());

    auto size = function->context->size();
    //Only if necessary, allocates size on the stack for the local variables
    if(size > 0){
        writer.stream() << "sub esp, " << size << std::endl;
    }
    
    auto iter = function->context->begin();
    auto end = function->context->end();

    for(; iter != end; iter++){
        auto var = iter->second;
        if(var->type().isArray() && var->position().isStack()){
            int position = -var->position().offset();

            writer.stream() << "mov dword [ebp + " << position << "], " << var->type().size() << std::endl;

            if(var->type().base() == BaseType::INT){
                writer.stream() << "mov ecx, " << var->type().size() << std::endl;
            } else if(var->type().base() == BaseType::STRING){
                writer.stream() << "mov ecx, " << (var->type().size() * 2) << std::endl;
            }
            
            writer.stream() << "mov eax, 0" << std::endl;
            writer.stream() << "lea edi, [ebp + " << position << " - 4]" << std::endl;
            writer.stream() << "std" << std::endl;
            writer.stream() << "rep stosd" << std::endl;
            writer.stream() << "cld" << std::endl;
        }
    }

    IntelX86StatementCompiler compiler(writer, function);

    tac::computeBlockUsage(function, compiler.blockUsage);

    //First we computes a label for each basic block
    for(auto& block : function->getBasicBlocks()){
        block->label = newLabel();
    }

    //Then we compile each of them
    for(auto& block : function->getBasicBlocks()){
        ::compile(writer, block, compiler);
    }
 
    if(function->getBasicBlocks().size() > 0){
        auto& lastBasicBlock = function->getBasicBlocks().back();
        
        if(lastBasicBlock->statements.size() > 0){
            auto lastStatement = lastBasicBlock->statements.back();
            
            if(auto* ptr = boost::get<std::shared_ptr<tac::Quadruple>>(&lastStatement)){
                if((*ptr)->op != tac::Operator::RETURN){
                    //Only if necessary, deallocates size on the stack for the local variables
                    if(size > 0){
                        writer.stream() << "add esp, " << size << std::endl;
                    }

                    leaveFunction(writer);

                    return;
                }
            }
        }
    }
                    
    //Only if necessary, deallocates size on the stack for the local variables
    if(size > 0){
        writer.stream() << "add esp, " << size << std::endl;
    }

    leaveFunction(writer);
}

void IntelX86CodeGenerator::writeRuntimeSupport(FunctionTable& table){
    writer.stream() << "section .text" << std::endl << std::endl;

    writer.stream() << "global _start" << std::endl << std::endl;

    writer.stream() << "_start:" << std::endl;

    //If the user wants the args, we add support for them
    if(table.getFunction("main")->parameters.size() == 1){
        writer.stream() << "pop ebx" << std::endl;                          //ebx = number of args
        writer.stream() << "lea ecx, [4 + ebx * 8]" << std::endl;           //ecx = size of the array
        writer.stream() << "push ecx" << std::endl;
        writer.stream() << "call eddi_alloc" << std::endl;                  //eax = start address of the array
        writer.stream() << "add esp, 4" << std::endl;

        writer.stream() << "lea esi, [eax + ecx - 4]" << std::endl;         //esi = last address of the array
        writer.stream() << "mov edx, esi" << std::endl;                     //edx = last address of the array
        
        writer.stream() << "mov [esi], ebx" << std::endl;                   //Set the length of the array
        writer.stream() << "sub esi, 8" << std::endl;                       //Move to the destination address of the first arg

        writer.stream() << ".copy_args:" << std::endl;
        writer.stream() << "pop edi" << std::endl;                          //edi = address of current args
        writer.stream() << "mov [esi+4], edi" << std::endl;                 //set the address of the string

        /* Calculate the length of the string  */
        writer.stream() << "xor eax, eax" << std::endl;
        writer.stream() << "xor ecx, ecx" << std::endl;
        writer.stream() << "not ecx" << std::endl;
        writer.stream() << "repne scasb" << std::endl;
        writer.stream() << "not ecx" << std::endl;
        writer.stream() << "dec ecx" << std::endl;
        /* End of the calculation */

        writer.stream() << "mov dword [esi], ecx" << std::endl;               //set the length of the string
        writer.stream() << "sub esi, 8" << std::endl;
        writer.stream() << "dec ebx" << std::endl;
        writer.stream() << "jnz .copy_args" << std::endl;

        writer.stream() << "push edx" << std::endl;
    }

    writer.stream() << "call main" << std::endl;
    writer.stream() << "mov eax, 1" << std::endl;
    writer.stream() << "xor ebx, ebx" << std::endl;
    writer.stream() << "int 80h" << std::endl;
}

void IntelX86CodeGenerator::defineDataSection(){
    writer.stream() << std::endl << "section .data" << std::endl;
}

void IntelX86CodeGenerator::declareIntArray(const std::string& name, unsigned int size){
    writer.stream() << "V" << name << ":" <<std::endl;
    writer.stream() << "%rep " << size << std::endl;
    writer.stream() << "dd 0" << std::endl;
    writer.stream() << "%endrep" << std::endl;
    writer.stream() << "dd " << size << std::endl;
}

void IntelX86CodeGenerator::declareFloatArray(const std::string& name, unsigned int size){
    writer.stream() << "V" << name << ":" <<std::endl;
    writer.stream() << "%rep " << size << std::endl;
    writer.stream() << "dd __float32__(0.0)" << std::endl;
    writer.stream() << "%endrep" << std::endl;
    writer.stream() << "dd " << size << std::endl;
}

void IntelX86CodeGenerator::declareStringArray(const std::string& name, unsigned int size){
    writer.stream() << "V" << name << ":" <<std::endl;
    writer.stream() << "%rep " << size << std::endl;
    writer.stream() << "dd S3" << std::endl;
    writer.stream() << "dd 0" << std::endl;
    writer.stream() << "%endrep" << std::endl;
    writer.stream() << "dd " << size << std::endl;
}

void IntelX86CodeGenerator::declareIntVariable(const std::string& name, int value){
    writer.stream() << "V" << name << " dd " << value << std::endl;
}

void IntelX86CodeGenerator::declareStringVariable(const std::string& name, const std::string& label, int size){
    writer.stream() << "V" << name << " dd " << label << ", " << size << std::endl;
}

void IntelX86CodeGenerator::declareString(const std::string& label, const std::string& value){
    writer.stream() << label << " dd " << value << std::endl;
}

}} //end of eddic::as

namespace { //anonymous namespace

void saveFloat32(AssemblyFileWriter& writer, const std::vector<std::string>& registers){
    for(auto& reg : registers){
        writer.stream() << "sub rsp, 4" << std::endl;
        writer.stream() << "movd [esp], " << reg << std::endl;
    }
}

void restoreFloat32(AssemblyFileWriter& writer, const std::vector<std::string>& registers){
    auto it = registers.rbegin();
    auto end = registers.rend();

    while(it != end){
        writer.stream() << "movd " << *it << ", [esp]" << std::endl;
        writer.stream() << "add rsp, 4" << std::endl;
        ++it;
    }
}

void addPrintIntegerBody(AssemblyFileWriter& writer){
    writer.stream() << "mov eax, [ebp+8]" << std::endl;
    writer.stream() << "xor esi, esi" << std::endl;

    //If the number is negative, we print the - and then the number
    writer.stream() << "cmp eax, 0" << std::endl;
    writer.stream() << "jge .loop" << std::endl;

    writer.stream() << "neg eax" << std::endl;
    writer.stream() << "push eax" << std::endl; //We push eax to not loose it from print_string

    //Print "-" 
    writer.stream() << "push S2" << std::endl;
    writer.stream() << "push 1" << std::endl;
    writer.stream() << "call _F5printS" << std::endl;
    writer.stream() << "add esp, 8" << std::endl;

    //Get the the valueof eax again
    writer.stream() << "pop eax" << std::endl;

    writer.stream() << ".loop" << ":" << std::endl;
    writer.stream() << "mov edx, 0" << std::endl;
    writer.stream() << "mov ebx, 10" << std::endl;
    writer.stream() << "div ebx" << std::endl;
    writer.stream() << "add edx, 48" << std::endl;
    writer.stream() << "push edx" << std::endl;
    writer.stream() << "inc esi" << std::endl;
    writer.stream() << "cmp eax, 0" << std::endl;
    writer.stream() << "jz .next" << std::endl;
    writer.stream() << "jmp .loop" << std::endl;

    writer.stream() << ".next" << ":" << std::endl;
    writer.stream() << "cmp esi, 0" << std::endl;
    writer.stream() << "jz .exit" << std::endl;
    writer.stream() << "dec esi" << std::endl;

    writer.stream() << "mov eax, 4" << std::endl;
    writer.stream() << "mov ecx, esp" << std::endl;
    writer.stream() << "mov ebx, 1" << std::endl;
    writer.stream() << "mov edx, 1" << std::endl;
    writer.stream() << "int 80h" << std::endl;

    writer.stream() << "add esp, 4" << std::endl;

    writer.stream() << "jmp .next" << std::endl;

    writer.stream() << ".exit" << ":" << std::endl;
}

void addPrintIntegerFunction(AssemblyFileWriter& writer){
    defineFunction(writer, "_F5printI");

    as::save(writer, {"eax", "ebx", "ecx", "edx", "esi"});

    addPrintIntegerBody(writer);

    as::restore(writer, {"eax", "ebx", "ecx", "edx", "esi"});

    leaveFunction(writer);
   
    /* println version */
    
    defineFunction(writer, "_F7printlnI");

    as::save(writer, {"eax", "ebx", "ecx", "edx", "esi"});

    addPrintIntegerBody(writer);

    writer.stream() << "call _F7println" << std::endl;

    as::restore(writer, {"eax", "ebx", "ecx", "edx", "esi"});

    leaveFunction(writer);
}

//TODO Convert to 32 bits
void addPrintFloatBody(AssemblyFileWriter& writer){
    writer.stream() << "movq xmm0, [rbp+16]" << std::endl;  //Get the floating point to display
    
    writer.stream() << "cvttsd2si rbx, xmm0" << std::endl;   //Get the integer part into rbx
    writer.stream() << "cvtsi2sd xmm1, rbx" << std::endl;   //Move the integer part into xmm1

    //Print the integer part
    writer.stream() << "push rbx" << std::endl;
    writer.stream() << "call _F5printI" << std::endl;
    writer.stream() << "add rsp, 8" << std::endl;

    //Print the dot char
    writer.stream() << "push S4" << std::endl;
    writer.stream() << "push 1" << std::endl;
    writer.stream() << "call _F5printS" << std::endl;
    writer.stream() << "add rsp, 16" << std::endl;
   
    //Remove the integer part from the floating point 
    writer.stream() << "subsd xmm0, xmm1" << std::endl;
    
    writer.stream() << "mov rcx, __float64__(10000.0)" << std::endl;
    writer.stream() << "movq xmm2, rcx" << std::endl;
    
    writer.stream() << "mulsd xmm0, xmm2" << std::endl;
    writer.stream() << "cvttsd2si rbx, xmm0" << std::endl;
    writer.stream() << "mov rax, rbx" << std::endl;

    //Handle numbers with 0 at the beginning of the decimal part
    writer.stream() << "or rax, rax" << std::endl;
    writer.stream() << "je .end" << std::endl;
    writer.stream() << ".start:" << std::endl;
    writer.stream() << "cmp rax, 1000" << std::endl;
    writer.stream() << "jge .end" << std::endl;
    writer.stream() << "push 0" << std::endl;
    writer.stream() << "call _F5printI" << std::endl;
    writer.stream() << "add rsp, 8" << std::endl;
    writer.stream() << "imul rax, 10" << std::endl;
    writer.stream() << "jmp .start" << std::endl;
    
    writer.stream() << ".end:" << std::endl;
    writer.stream() << "push rbx" << std::endl;
    writer.stream() << "call _F5printI" << std::endl;
    writer.stream() << "add rsp, 8" << std::endl;
}

void addPrintFloatFunction(AssemblyFileWriter& writer){
    defineFunction(writer, "_F5printF");

    as::save(writer, {"rax", "rbx"});
    saveFloat32(writer, {"xmm0", "xmm1", "xmm2"});

    addPrintFloatBody(writer);

    restoreFloat32(writer, {"xmm0", "xmm1", "xmm2"});
    as::restore(writer, {"rax", "rbx"});

    leaveFunction(writer);
   
    /* println version */
    
    defineFunction(writer, "_F7printlnF");

    as::save(writer, {"rax", "rbx"});
    saveFloat32(writer, {"xmm0", "xmm1", "xmm2"});

    addPrintFloatBody(writer);

    writer.stream() << "call _F7println" << std::endl;

    restoreFloat32(writer, {"xmm0", "xmm1", "xmm2"});
    as::restore(writer, {"rax", "rbx"});

    leaveFunction(writer);
}

void addPrintBoolBody(AssemblyFileWriter& writer){
    writer.stream() << "mov eax, [ebp-4] " << std::endl;
    writer.stream() << "or eax, eax" << std::endl;
    writer.stream() << "jne .true_print" << std::endl;
    writer.stream() << "push 0" << std::endl;
    writer.stream() << "call _F5printI" << std::endl;
    writer.stream() << "jmp .end" << std::endl;
    writer.stream() << ".true_print:" << std::endl;
    writer.stream() << "push 1" << std::endl;
    writer.stream() << "call _F5printI" << std::endl;
    writer.stream() << "sub esp, 4" << std::endl;
    writer.stream() << ".end:" << std::endl;
}

void addPrintBoolFunction(AssemblyFileWriter& writer){
    defineFunction(writer, "_F5printB");

    as::save(writer, {"eax", "ebx", "ecx", "edx", "esi"});

    addPrintBoolBody(writer);

    as::restore(writer, {"eax", "ebx", "ecx", "edx", "esi"});

    leaveFunction(writer);
   
    /* println version */
    
    defineFunction(writer, "_F7printlnB");

    as::save(writer, {"eax", "ebx", "ecx", "edx", "esi"});

    addPrintBoolBody(writer);

    writer.stream() << "call _F7println" << std::endl;

    as::restore(writer, {"eax", "ebx", "ecx", "edx", "esi"});

    leaveFunction(writer);
}

void addPrintLineFunction(AssemblyFileWriter& writer){
    defineFunction(writer, "_F7println");

    writer.stream() << "push S1" << std::endl;
    writer.stream() << "push 1" << std::endl;
    writer.stream() << "call _F5printS" << std::endl;
    writer.stream() << "add esp, 8" << std::endl;

    leaveFunction(writer);
}

void addPrintStringBody(AssemblyFileWriter& writer){
    writer.stream() << "mov esi, 0" << std::endl;

    writer.stream() << "mov eax, 4" << std::endl;
    writer.stream() << "mov ebx, 1" << std::endl;
    writer.stream() << "mov ecx, [ebp + 12]" << std::endl;
    writer.stream() << "mov edx, [ebp + 8]" << std::endl;
    writer.stream() << "int 80h" << std::endl;
}

void addPrintStringFunction(AssemblyFileWriter& writer){
    defineFunction(writer, "_F5printS");
    
    as::save(writer, {"eax", "ebx", "ecx", "edx", "esi"});

    addPrintStringBody(writer);

    as::restore(writer, {"eax", "ebx", "ecx", "edx", "esi"});

    leaveFunction(writer);
   
    /* println version */
    
    defineFunction(writer, "_F7printlnS");
    
    as::save(writer, {"eax", "ebx", "ecx", "edx", "esi"});

    addPrintStringBody(writer);

    writer.stream() << "call _F7println" << std::endl;

    as::restore(writer, {"eax", "ebx", "ecx", "edx", "esi"});

    leaveFunction(writer);
}

void addConcatFunction(AssemblyFileWriter& writer){
    defineFunction(writer, "concat");

    writer.stream() << "mov ebx, [ebp + 16]" << std::endl;
    writer.stream() << "mov ecx, [ebp + 8]" << std::endl;
    writer.stream() << "add ebx, ecx" << std::endl;             //ebx = number of bytes = return 2

    //alloc the total number of bytes
    writer.stream() << "push ebx" << std::endl;
    writer.stream() << "call eddi_alloc" << std::endl;
    writer.stream() << "add esp, 8" << std::endl;

    writer.stream() << "mov edi, eax" << std::endl;             //destination address for the movsb
    
    writer.stream() << "mov ecx, [ebp + 16]" << std::endl;      //number of bytes of the source
    writer.stream() << "mov esi, [ebp + 20]" << std::endl;      //source address

    writer.stream() << "rep movsb" << std::endl;                //copy the first part of the string into the destination

    writer.stream() << "mov ecx, [ebp + 8]" << std::endl;      //number of bytes of the source
    writer.stream() << "mov esi, [ebp + 12]" << std::endl;      //source address

    writer.stream() << "rep movsb" << std::endl;                //copy the second part of the string into the destination

    leaveFunction(writer);
}

void addAllocFunction(AssemblyFileWriter& writer){
    defineFunction(writer, "eddi_alloc");

    as::save(writer, {"ebx", "ecx", "edx"});

    writer.stream() << "mov ecx, [ebp + 8]" << std::endl;
    writer.stream() << "mov ebx, [Veddi_remaining]" << std::endl;

    writer.stream() << "cmp ecx, ebx" << std::endl;
    writer.stream() << "jle .alloc_normal" << std::endl;

    //Get the current address
    writer.stream() << "mov eax, 45" << std::endl;          //45 = sys_brk
    writer.stream() << "xor ebx, ebx" << std::endl;         //get end
    writer.stream() << "int 80h" << std::endl;

    //%eax is the current address 
    writer.stream() << "mov esi, eax" << std::endl;

    //Alloc new block of 16384K from the current address
    writer.stream() << "mov ebx, eax" << std::endl;
    writer.stream() << "add ebx, 16384" << std::endl;
    writer.stream() << "mov eax, 45" << std::endl;          //45 = sys_brk
    writer.stream() << "int 80h" << std::endl;

    //zero'd the new block
    writer.stream() << "mov edi, eax" << std::endl;         //edi = start of block

    writer.stream() << "sub edi, 4" << std::endl;           //edi points to the last DWORD available to us
    writer.stream() << "mov ecx, 4096" << std::endl;        //this many DWORDs were allocated
    writer.stream() << "xor eax, eax"  << std::endl;        //will write with zeroes
    writer.stream() << "std"  << std::endl;                 //walk backwards
    writer.stream() << "rep stosb"  << std::endl;           //write all over the reserved area
    writer.stream() << "cld"  << std::endl;                 //bring back the DF flag to normal state

    writer.stream() << "mov eax, esi" << std::endl;

    //We now have 16K of available memory starting at %esi
    writer.stream() << "mov dword [Veddi_remaining], 16384" << std::endl;
    writer.stream() << "mov [Veddi_current], esi" << std::endl;

    writer.stream() << ".alloc_normal:" << std::endl;

    //old = current
    writer.stream() << "mov eax, [Veddi_current]" << std::endl;

    //current += size
    writer.stream() << "mov ebx, [Veddi_current]" << std::endl;
    writer.stream() << "add ebx, ecx" << std::endl;
    writer.stream() << "mov [Veddi_current], ebx" << std::endl;

    //remaining -= size
    writer.stream() << "mov ebx, [Veddi_remaining]" << std::endl;
    writer.stream() << "sub ebx, ecx" << std::endl;
    writer.stream() << "mov [Veddi_remaining], ebx" << std::endl;

    writer.stream() << ".alloc_end:" << std::endl;

    as::restore(writer, {"ebx", "ecx", "edx"});

    leaveFunction(writer);
}

void addTimeFunction(AssemblyFileWriter& writer){
    defineFunction(writer, "_F4timeAI");

    writer.stream() << "xor eax, eax" << std::endl;
    writer.stream() << "cpuid" << std::endl;                //only to serialize instruction stream
    writer.stream() << "rdtsc" << std::endl;                //edx:eax = timestamp

    writer.stream() << "mov esi, [ebp + 8]" << std::endl;
    writer.stream() << "mov [esi - 4], eax" << std::endl;
    writer.stream() << "mov [esi - 8], edx" << std::endl;

    leaveFunction(writer);
}

void addDurationFunction(AssemblyFileWriter& writer){
    defineFunction(writer, "_F8durationAIAI");

    writer.stream() << "mov esi, [ebp + 12]" << std::endl;          //Start time stamp
    writer.stream() << "mov edi, [ebp + 8]" << std::endl;           //End time stamp

    //Print the high order bytes
    writer.stream() << "mov eax, [esi - 8]" << std::endl;
    writer.stream() << "mov ebx, [edi - 8]" << std::endl;
    writer.stream() << "sub eax, ebx" << std::endl;
   
    //if the first diff is 0, do not print 0
    writer.stream() << "cmp eax, 0" << std::endl;
    writer.stream() << "jz .second" << std::endl;

    //If it's negative, we print the positive only 
    writer.stream() << "cmp eax, 0" << std::endl;
    writer.stream() << "jge .push_first" << std::endl;
    writer.stream() << "neg eax" << std::endl;
    
    writer.stream() << ".push_first:" << std::endl; 
    writer.stream() << "push eax" << std::endl;
    writer.stream() << "call _F5printI" << std::endl;

    //Print the low order bytes
    writer.stream() << ".second:" << std::endl;
    writer.stream() << "mov eax, [esi - 4]" << std::endl;
    writer.stream() << "mov ebx, [edi - 4]" << std::endl;
    writer.stream() << "sub eax, ebx" << std::endl;
   
    //If it's negative, we print the positive only 
    writer.stream() << "cmp eax, 0" << std::endl;
    writer.stream() << "jge .push_second" << std::endl;
    writer.stream() << "neg eax" << std::endl;
   
    writer.stream() << ".push_second:" << std::endl; 
    writer.stream() << "push eax" << std::endl;
    writer.stream() << "call _F5printI" << std::endl;

    leaveFunction(writer);
}

} //end of anonymous namespace

void as::IntelX86CodeGenerator::addStandardFunctions(){
   addPrintIntegerFunction(writer); 
   addPrintFloatFunction(writer); 
   addPrintBoolFunction(writer);
   addPrintLineFunction(writer); 
   addPrintStringFunction(writer); 
   addConcatFunction(writer);
   addAllocFunction(writer);
   addTimeFunction(writer);
   addDurationFunction(writer);
}
