//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <memory>
#include <unordered_map>

#include <boost/variant.hpp>

#include "tac/IntelX86CodeGenerator.hpp"

#include "AssemblyFileWriter.hpp"
#include "FunctionContext.hpp"

#include "il/Labels.hpp"

using namespace eddic;

tac::IntelX86CodeGenerator::IntelX86CodeGenerator(AssemblyFileWriter& w) : writer(w) {}

namespace eddic { namespace tac { 

enum Register {
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

std::string regToString(Register reg){
    switch(reg){
        case EAX:
            return "%eax";
        case EBX:
            return "%ebx";
        case ECX:
            return "%ecx";
        case EDX:
            return "%edx";
        case ESP:
            return "%esp";
        case EBP:
            return "%ebp";
        case ESI:
            return "%esi";
        case EDI:
            return "%edi";
        default:
            assert(false); //Not a register
    }
}

struct StatementCompiler : public boost::static_visitor<> {
    AssemblyFileWriter& writer;
    std::shared_ptr<tac::Function> function;

    std::unordered_map<std::shared_ptr<BasicBlock>, std::string> labels;
 
    std::vector<Register> registers;   
    std::shared_ptr<Variable> descriptors[Register::REGISTER_COUNT];
    std::unordered_map<std::shared_ptr<Variable>, Register> variables;

    StatementCompiler(AssemblyFileWriter& w, std::shared_ptr<tac::Function> f) : writer(w), function(f) {
        registers = {EDI, ESI, ECX, EDX, EBX, EAX};
    }

    void move(std::shared_ptr<Variable> variable, Register reg){
        auto position = variable->position();

        if(position.isStack()){
            writer.stream() << "movl " << (-1 * position.offset()) << "(%ebp), " << regToString(reg) << std::endl; 
        } else if(position.isParameter()){
            writer.stream() << "movl " << position.offset() << "(%ebp), " << regToString(reg) << std::endl; 
        } else if(position.isGlobal()){
            writer.stream() << "movl " << "VI" << position.name() << ", " << regToString(reg) << std::endl;
        } else if(position.isTemporary()){
            std::cout << "Trying to move " << variable->name() << " into " << regToString(reg) << std::endl;
//            assert(false); //We are in da shit
        }
    }

    std::string toString(std::shared_ptr<Variable> variable, int offset){
        auto position = variable->position();

        if(position.isStack()){
            return ::toString(-1 * (position.offset() + offset)) + "(%ebp)";
        } else if(position.isParameter()){
            return ::toString(position.offset() + offset) + "(%ebp)";
        } else if(position.isGlobal()){
            return "VI" + position.name() + "+" + ::toString(offset);
        } else if(position.isTemporary()){
            assert(false); //We are in da shit
        }

        assert(false);
    }
    
    void spills(Register reg){
        assert(descriptors[reg]);

        auto variable = descriptors[reg];
        auto position = variable->position();
        
        if(position.isStack()){
            writer.stream() << "movl " << regToString(reg) << ", " << (-1 * position.offset()) << "(%ebp)" << std::endl; 
        } else if(position.isParameter()){
            writer.stream() << "movl " << regToString(reg) << ", " <<  position.offset() << "(%ebp)" << std::endl; 
        } else if(position.isGlobal()){
            writer.stream() << "movl " << regToString(reg) << ", VI" << position.name() << std::endl;
        } else if(position.isTemporary()){
            std::cout << "Trying to spills " << variable->name() << " from " << regToString(reg) << std::endl;
        }

        //The variable is no more contained in the register
        descriptors[reg] = nullptr;
        variables.erase(variable);
    }

    Register getReg(std::shared_ptr<Variable> variable){
        //The variable is already in a register
        if(variables.find(variable) != variables.end()){
            return variables[variable];
        }
       
        //Try to get a free register 
        for(auto reg : registers){
            if(!descriptors[reg]){
                move(variable, reg);

                descriptors[reg] = variable;
                variables[variable] = reg;

                return reg;
            }
        }

        //There are no free register, take one
        auto reg = registers[0];
        spills(reg);

        move(variable, reg);

        descriptors[reg] = variable;
        variables[variable] = reg;

        return reg;
    }
    
    std::string toString(std::shared_ptr<Variable> variable, tac::Argument offset){
        if(auto* ptr = boost::get<int>(&offset)){
            return toString(variable, *ptr);
        }
        
        assert(boost::get<std::shared_ptr<Variable>>(&offset));

        auto* offsetVariable = boost::get<std::shared_ptr<Variable>>(&offset);
        auto position = variable->position();

        auto reg = getReg(variable);
        auto offsetReg = getReg(*offsetVariable);

        return "(" + regToString(reg) + ")" + regToString(offsetReg);
    }

    std::string arg(tac::Argument argument){
        if(auto* ptr = boost::get<int>(&argument)){
            return "$" + ::toString(*ptr);
        } else if(auto* ptr = boost::get<std::string>(&argument)){
            return "$" + *ptr;
        } else if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&argument)){
            return regToString(getReg(*ptr));
        }

        assert(false);
    }

    void operator()(std::shared_ptr<tac::Goto>& goto_){
       writer.stream() << "goto " << labels[goto_->block] << std::endl; 
    }

    void operator()(std::shared_ptr<tac::Param>& param){
        writer.stream() << "pushl " << arg(param->arg) << std::endl;
    }

    void operator()(std::shared_ptr<tac::Call>& call){
        writer.stream() << "call " << call->function << std::endl;
        
        if(call->params > 0){
            writer.stream() << "addl " << call->params << ", %esp" << std::endl;
        }

        if(call->return_){
            descriptors[Register::EAX] = call->return_;
            variables[call->return_] = Register::EAX;
        }

        if(call->return2_){
            descriptors[Register::EBX] = call->return2_;
            variables[call->return2_] = Register::EBX;
        }
    }
    
    void operator()(std::shared_ptr<tac::Return>& return_){
        //A return without args is the same as exiting from the function
        if(return_->arg1){
            if(descriptors[Register::EAX]){
                spills(Register::EAX);
            }
            
            writer.stream() << "movl " << arg(*return_->arg1) << ", %eax" << std::endl;

            if(return_->arg2){
                if(descriptors[Register::EBX]){
                    spills(Register::EBX);
                }

                writer.stream() << "movl " << arg(*return_->arg2) << ", %ebx" << std::endl;
            }
        }
        
        if(function->context->size() > 0){
            writer.stream() << "addl $" << function->context->size() << " , %esp" << std::endl;
        }

        writer.stream() << "leave" << std::endl;
        writer.stream() << "ret" << std::endl;
    }
    
    void operator()(std::shared_ptr<tac::Quadruple>& quadruple){
        if(!quadruple->op){
            writer.stream() << "movl " << arg(quadruple->arg1) << ", " << arg(quadruple->result) << std::endl;            
        } else {
            switch(*quadruple->op){
                case Operator::ADD://TODO Find a way to optimize statements like a = a + b or a = b + a
                    writer.stream() << "addl " << arg(quadruple->arg1) << ", " << arg(*quadruple->arg2) << std::endl;
                    writer.stream() << "movl " << arg(*quadruple->arg2) << ", " << arg(quadruple->result) << std::endl;
                    break;            
                case Operator::SUB:
                    writer.stream() << "subl " << arg(*quadruple->arg2) << ", " << arg(quadruple->arg1) << std::endl;
                    writer.stream() << "movl " << arg(quadruple->arg1) << ", " << arg(quadruple->result) << std::endl;
                    break;            
                case Operator::MUL:
                    //TODO
                    break;            
                case Operator::DIV:
                    //TODO
                    break;            
                case Operator::MOD:
                    //TODO
                    break;            
                case Operator::DOT:{
                    assert(boost::get<std::shared_ptr<Variable>>(&quadruple->arg1));
                    assert(boost::get<int>(&*quadruple->arg2));

                    int offset = boost::get<int>(*quadruple->arg2);
                    auto variable = boost::get<std::shared_ptr<Variable>>(quadruple->arg1);

                    writer.stream() << "movl " << toString(variable, offset) << ", " << arg(quadruple->result) << std::endl;
                    break;
                }
                case Operator::DOT_ASSIGN:{
                    assert(boost::get<int>(&quadruple->arg1));

                    int offset = boost::get<int>(quadruple->arg1);

                    writer.stream() << "movl " << arg(*quadruple->arg2) << ", " << toString(quadruple->result, offset) << std::endl;
                    break;
                }
                case Operator::ARRAY:
                    assert(boost::get<std::shared_ptr<Variable>>(&quadruple->arg1));
                    
                    writer.stream() << "movl " << toString(boost::get<std::shared_ptr<Variable>>(quadruple->arg1), *quadruple->arg2) << ", " << arg(quadruple->result) << std::endl;
                    break;            
                case Operator::ARRAY_ASSIGN:
                    writer.stream() << "movl " << arg(*quadruple->arg2) << ", " << toString(quadruple->result, quadruple->arg1) << std::endl;
                    break;            
            }
        }
    }
    
    void operator()(std::shared_ptr<tac::IfFalse>& ifFalse){
        writer.stream() << "cmpl " << arg(ifFalse->arg1) << ", " << arg(ifFalse->arg2) << std::endl;

        switch(ifFalse->op){
            case BinaryOperator::EQUALS:
                writer.stream() << "jne " << labels[ifFalse->block] << std::endl;
                break;
            case BinaryOperator::NOT_EQUALS:
                writer.stream() << "je " << labels[ifFalse->block] << std::endl;
                break;
            case BinaryOperator::LESS:
                writer.stream() << "jge " << labels[ifFalse->block] << std::endl;
                break;
            case BinaryOperator::LESS_EQUALS:
                writer.stream() << "jg " << labels[ifFalse->block] << std::endl;
                break;
            case BinaryOperator::GREATER:
                writer.stream() << "jle " << labels[ifFalse->block] << std::endl;
                break;
            case BinaryOperator::GREATER_EQUALS:
                writer.stream() << "jl " << labels[ifFalse->block] << std::endl;
                break;
        }
    }

    void operator()(std::string&){
        assert(false); //There is no more label after the basic blocks have been extracted
    }
};

}}

void tac::IntelX86CodeGenerator::compile(std::shared_ptr<tac::BasicBlock> block, StatementCompiler& compiler){
    std::string label = newLabel();

    compiler.labels[block] = label;

    writer.stream() << label << ":" << std::endl;

    for(auto& statement : block->statements){
        boost::apply_visitor(compiler, statement);
    }

    //TODO End the basic block
}

bool updateLiveness(std::unordered_map<std::shared_ptr<Variable>, bool>& liveness, tac::Argument arg){
    if(auto* variable = boost::get<std::shared_ptr<Variable>>(&arg)){
        if(liveness.find(*variable) != liveness.end()){
            if((*variable)->position().isGlobal()){
                liveness[*variable] = true;
            } else {
                liveness[*variable] = false;
            }
        }

        bool live = liveness[*variable];

        //variable is live
        liveness[*variable] = true;

        return live;
    }

    return false;
}

void tac::IntelX86CodeGenerator::computeLiveness(std::shared_ptr<tac::Function> function){
    std::vector<std::shared_ptr<BasicBlock>>::reverse_iterator bit = function->getBasicBlocks().rbegin();
    std::vector<std::shared_ptr<BasicBlock>>::reverse_iterator bend = function->getBasicBlocks().rend(); 
    
    std::unordered_map<std::shared_ptr<Variable>, bool> liveness;

    while(bit != bend){
        std::vector<tac::Statement>::reverse_iterator sit = (*bit)->statements.rbegin();
        std::vector<tac::Statement>::reverse_iterator send = (*bit)->statements.rend(); 

        while(sit != send){
            auto statement = *sit;

            if(auto* ptr = boost::get<std::shared_ptr<tac::Param>>(&statement)){
                (*ptr)->liveVariable = updateLiveness(liveness, (*ptr)->arg);
            } else if(auto* ptr = boost::get<std::shared_ptr<tac::Return>>(&statement)){
                if((*ptr)->arg1){
                    (*ptr)->liveVariable1 = updateLiveness(liveness, (*(*ptr)->arg1));
                }
                
                if((*ptr)->arg2){
                    (*ptr)->liveVariable2 = updateLiveness(liveness, (*(*ptr)->arg2));
                }
            } else if(auto* ptr = boost::get<std::shared_ptr<tac::IfFalse>>(&statement)){
                (*ptr)->liveVariable1 = updateLiveness(liveness, (*ptr)->arg1);
                (*ptr)->liveVariable2 = updateLiveness(liveness, (*ptr)->arg2);
            } else if(auto* ptr = boost::get<std::shared_ptr<tac::Quadruple>>(&statement)){
                (*ptr)->liveVariable1 = updateLiveness(liveness, (*ptr)->arg1);
                
                if((*ptr)->arg2){
                    (*ptr)->liveVariable2 = updateLiveness(liveness, (*(*ptr)->arg2));
                }

                (*ptr)->liveResult = liveness[(*ptr)->result];

                liveness[(*ptr)->result] = false; 
            }

            sit++;
        }

        bit++;
    }
}

void tac::IntelX86CodeGenerator::compile(std::shared_ptr<tac::Function> function){
    computeLiveness(function);

    writer.stream() << std::endl << function->getName() << ":" << std::endl;
    
    writer.stream() << "pushl %ebp" << std::endl;
    writer.stream() << "movl %esp, %ebp" << std::endl;

    auto size = function->context->size();
    //Only if necessary, allocates size on the stack for the local variables
    if(size > 0){
        writer.stream() << "subl $" << size << " , %esp" << std::endl;
    }

    StatementCompiler compiler(writer, function);
    for(auto& block : function->getBasicBlocks()){
        compile(block, compiler);
    }
    
    //Only if necessary, deallocates size on the stack for the local variables
    if(size > 0){
        writer.stream() << "addl $" << size << " , %esp" << std::endl;
    }
    
    writer.stream() << "leave" << std::endl;
    writer.stream() << "ret" << std::endl;
}

void tac::IntelX86CodeGenerator::generate(tac::Program& program){
    resetNumbering();

    for(auto& function : program.functions){
        compile(function);
    }
}
