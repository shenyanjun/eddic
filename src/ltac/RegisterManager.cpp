//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <set>
#include <boost/range/adaptors.hpp>

#include "assert.hpp"
#include "FunctionContext.hpp"
#include "Type.hpp"
#include "Options.hpp"
#include "logging.hpp"

#include "ltac/Utils.hpp"
#include "ltac/RegisterManager.hpp"
#include "ltac/StatementCompiler.hpp"

#include "mtac/Utils.hpp"
#include "mtac/Printer.hpp"

using namespace eddic;

namespace {

template<typename Reg>
Reg get_free_reg(as::Registers<Reg>& registers, ltac::RegisterManager& manager){
    //Try to get a free register 
    for(Reg reg : registers){
        if(!registers.used(reg)){
            return reg;
        } 
        
        if(!registers.reserved(reg) && !manager.is_live(registers[reg]) && !registers[reg]->position().isParamRegister() && !registers[reg]->position().is_register()){
            registers.remove(registers[reg]);

            return reg;
        }
    }

    //There are no free register, take one
    Reg reg = registers.first();
    bool found = false;

    //First, try to take a register that doesn't need to be spilled (variable has not modified)
    for(Reg remaining : registers){
        if(!registers.reserved(remaining) && !registers[remaining]->position().isParamRegister() && !registers[remaining]->position().is_register()){
            if(!manager.is_written(registers[remaining])){
                reg = remaining;
                found = true;
                break;
            }
        }
    }

    //If there is no registers that doesn't need to be spilled, take the first one not reserved 
    if(!found){
        for(Reg remaining : registers){
            if(!registers.reserved(remaining) && !registers[remaining]->position().isTemporary() && !registers[remaining]->position().isParamRegister() && !registers[remaining]->position().is_register()){
                reg = remaining;
                found = true;
                break;
            }
        }
    }
    
    if(!found){
        for(Reg r : registers){
            std::cout << "Register " << r << log::endl;
            if(!registers.reserved(r)){
                if(registers.used(r)){
                    std::cout << "  used by " << registers[r]->name() << log::endl;
                } else {
                    std::cout << "  not used" << log::endl;
                }
            } else {
                std::cout << "  reserved" << log::endl;
            }
        }

        ASSERT_PATH_NOT_TAKEN("No register found");
    }

    manager.spills(reg);

    return reg; 
}

template<typename Reg> 
Reg get_reg(as::Registers<Reg>& registers, std::shared_ptr<Variable> variable, bool doMove, ltac::RegisterManager& manager){
    //The variable is already in a register
    if(registers.inRegister(variable)){
        return registers[variable];
    }

    Reg reg = get_free_reg(registers, manager);

    log::emit<Trace>("Registers") << "Found reg " << reg << log::endl;

    if(doMove){
        manager.move(variable, reg);
    }

    registers.setLocation(variable, reg);

    return reg;
}
    
template<typename Reg>
void safe_move(as::Registers<Reg>& registers, std::shared_ptr<Variable> variable, Reg reg, ltac::RegisterManager& manager){
    if(registers.used(reg)){
        if(registers[reg] != variable){
            manager.spills(reg);

            manager.move(variable, reg);
        }
    } else {
        manager.move(variable, reg);
    }
}
    
template<typename Reg>
void spills(as::Registers<Reg>& registers, Reg reg, ltac::Operator mov, ltac::RegisterManager& manager){
    //If the register is not used, there is nothing to spills
    if(registers.used(reg)){
        auto variable = registers[reg];

        //Do no spills variable stored in register
        if(variable->position().isParamRegister() || variable->position().is_register()){
            return;
        }

        //If the variable has not been written, there is no need to spill it
        if(manager.written.find(variable) != manager.written.end()){
            auto position = variable->position();
            if(position.isStack() || position.isParameter()){
                ltac::add_instruction(manager.function, mov, manager.access_compiler()->stack_address(position.offset()), reg);
            } else if(position.isGlobal()){
                ltac::add_instruction(manager.function, mov, ltac::Address("V" + position.name()), reg);
            } else if(position.isTemporary()){
                //If the variable is live, move it to another register, else do nothing
                if(manager.is_live(variable)){
                    registers.remove(variable);
                    manager.reserve(reg);

                    auto newReg = get_reg(registers, variable, false, manager);
                    ltac::add_instruction(manager.function, mov, newReg, reg);

                    manager.release(reg);

                    return; //Return here to avoid erasing variable from variables
                }
            }
        }

        //The variable is no more contained in the register
        registers.remove(variable);

        //The variable has not been written now
        manager.written.erase(variable);
    }
}

template<typename Reg>
void spills_all(as::Registers<Reg>& registers, ltac::RegisterManager& manager){
    log::emit<Trace>("Registers") << "Spills all" << log::endl;

    for(auto reg : registers){
        //The register can be reserved if the ending occurs in a special break case
        if(!registers.reserved(reg) && registers.used(reg)){
            auto variable = registers[reg];

            if(!variable->position().isTemporary()){
                manager.spills(reg);    
            }
        }
    }
}

} //end of anonymous namespace
    
ltac::RegisterManager::RegisterManager(const std::vector<ltac::Register>& registers, const std::vector<ltac::FloatRegister>& float_registers, 
        std::shared_ptr<ltac::Function> function, std::shared_ptr<FloatPool> float_pool) : 
    AbstractRegisterManager(registers, float_registers), function(function), float_pool(float_pool) {
        //Nothing else to init
}

void ltac::RegisterManager::reset(){
    registers.reset();
    float_registers.reset();

    written.clear();
}

bool ltac::RegisterManager::in_reg(std::shared_ptr<Variable> var){
    return registers.inRegister(var);
}

void ltac::RegisterManager::copy(mtac::Argument argument, ltac::FloatRegister reg){
    assert(ltac::is_variable(argument) || mtac::isFloat(argument));

    if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&argument)){
        auto variable = *ptr;

        log::emit<Trace>("Registers") << "Copy " << variable->name() << log::endl;

        //If the variable is hold in a register, just move the register value
        if(float_registers.inRegister(variable)){
            auto oldReg = float_registers[variable];

            ltac::add_instruction(function, ltac::Operator::FMOV, reg, oldReg);
        } else {
            auto position = variable->position();

            //The temporary should have been handled by the preceding condition (hold in a register)
            assert(!position.isTemporary());

            if(position.isStack() || position.isParameter()){
                ltac::add_instruction(function, ltac::Operator::FMOV, reg, access_compiler()->stack_address(position.offset()));
            } else if(position.isGlobal()){
                ltac::add_instruction(function, ltac::Operator::FMOV, reg, ltac::Address("V" + position.name()));
            } 
        }
    } else if(auto* ptr = boost::get<double>(&argument)){
        auto label = float_pool->label(*ptr);
        ltac::add_instruction(function, ltac::Operator::FMOV, reg, ltac::Address(label));
    }
}

void ltac::RegisterManager::copy(mtac::Argument argument, ltac::Register reg){
    if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&argument)){
        auto variable = *ptr;

        //If the variable is hold in a register, just move the register value
        if(registers.inRegister(variable)){
            auto oldReg = registers[variable];

            ltac::add_instruction(function, ltac::Operator::MOV, reg, oldReg);
        } else {
            auto position = variable->position();

            //The temporary should have been handled by the preceding condition (hold in a register)
            assert(!position.isTemporary());

            if(position.isStack() || position.isParameter()){
                ltac::add_instruction(function, ltac::Operator::MOV, reg, access_compiler()->stack_address(position.offset()));
            } else if(position.isGlobal()){
                ltac::add_instruction(function, ltac::Operator::MOV, reg, ltac::Address("V" + position.name()));
            } 
        } 
    } else {
        //If it's a constant (int, double, string), just move it
        ltac::add_instruction(function, ltac::Operator::MOV, reg, to_arg(argument, *this));
    }
}

void ltac::RegisterManager::move(mtac::Argument argument, ltac::Register reg){
    if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&argument)){
        auto variable = *ptr;

        //If the variable is hold in a register, just move the register value
        if(registers.inRegister(variable)){
            auto oldReg = registers[variable];

            //Only if the variable is not already on the same register 
            if(oldReg != reg){
                ltac::add_instruction(function, ltac::Operator::MOV, reg, oldReg);

                //There is nothing more in the old register
                registers.remove(variable);
            }
        } else {
            auto position = variable->position();

            //The temporary should have been handled by the preceding condition (hold in a register)
            assert(!position.isTemporary());

            if(position.isStack() || position.isParameter()){
                ltac::add_instruction(function, ltac::Operator::MOV, reg, access_compiler()->stack_address(position.offset()));
            } else if(position.isGlobal()){
                ltac::add_instruction(function, ltac::Operator::MOV, reg, ltac::Address("V" + position.name()));
            } 
        } 

        //The variable is now held in the new register
        registers.setLocation(variable, reg);
    } else {
        //If it's a constant (int, double, string), just move it
        ltac::add_instruction(function, ltac::Operator::MOV, reg, to_arg(argument, *this));
    }
}

void ltac::RegisterManager::move(mtac::Argument argument, ltac::FloatRegister reg){
    assert(ltac::is_variable(argument) || mtac::isFloat(argument));

    if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&argument)){
        auto variable = *ptr;

        //If the variable is hold in a register, just move the register value
        if(float_registers.inRegister(variable)){
            auto oldReg = float_registers[variable];

            //Only if the variable is not already on the same register 
            if(oldReg != reg){
                ltac::add_instruction(function, ltac::Operator::FMOV, reg, oldReg);

                //There is nothing more in the old register
                float_registers.remove(variable);
            }
        } else {
            auto position = variable->position();

            //The temporary should have been handled by the preceding condition (hold in a register)
            assert(!position.isTemporary());

            if(position.isStack() || position.isParameter()){
                ltac::add_instruction(function, ltac::Operator::FMOV, reg, access_compiler()->stack_address(position.offset()));
            } else if(position.isGlobal()){
                ltac::add_instruction(function, ltac::Operator::FMOV, reg, ltac::Address("V" + position.name()));
            } 
        }

        //The variable is now held in the new register
        float_registers.setLocation(variable, reg);
    } else if(auto* ptr = boost::get<double>(&argument)){
        auto label = float_pool->label(*ptr);
        ltac::add_instruction(function, ltac::Operator::FMOV, reg, ltac::Address(label));
    }
}

ltac::Register ltac::RegisterManager::get_reg(std::shared_ptr<Variable> var){
    log::emit<Trace>("Registers") << "Get reg for " << var->name() << log::endl;
    auto reg = ::get_reg(registers, var, true, *this);
    log::emit<Trace>("Registers") << "Get reg for " << var->name() << " => " << reg << log::endl;
    return reg;
}

ltac::Register ltac::RegisterManager::get_reg_no_move(std::shared_ptr<Variable> var){
    log::emit<Trace>("Registers") << "Get reg no move for " << var->name() << log::endl;
    auto reg = ::get_reg(registers, var, false, *this);
    log::emit<Trace>("Registers") << "Get reg for " << var->name() << " => " << reg << log::endl;
    return reg;
}

ltac::FloatRegister ltac::RegisterManager::get_float_reg(std::shared_ptr<Variable> var){
    log::emit<Trace>("Registers") << "Get reg for " << var->name() << log::endl;
    auto reg = ::get_reg(float_registers, var, true, *this);
    log::emit<Trace>("Registers") << "Get reg for " << var->name() << " => " << reg << log::endl;
    return reg;
}

ltac::FloatRegister ltac::RegisterManager::get_float_reg_no_move(std::shared_ptr<Variable> var){
    log::emit<Trace>("Registers") << "Get reg no move for " << var->name() << log::endl;
    auto reg = ::get_reg(float_registers, var, false, *this);
    log::emit<Trace>("Registers") << "Get reg for " << var->name() << " => " << reg << log::endl;
    return reg;
}

void ltac::RegisterManager::safe_move(std::shared_ptr<Variable> variable, ltac::Register reg){
    log::emit<Trace>("Registers") << "Safe move " << variable->name() << " in " << reg << log::endl;
    return ::safe_move(registers, variable, reg, *this);
}

void ltac::RegisterManager::safe_move(std::shared_ptr<Variable> variable, ltac::FloatRegister reg){
    log::emit<Trace>("Registers") << "Safe move " << variable->name() << " in " << reg << log::endl;
    return ::safe_move(float_registers, variable, reg, *this);
}

void ltac::RegisterManager::spills(ltac::Register reg){
    log::emit<Trace>("Registers") << "Spills Register " << reg << log::endl;
    ::spills(registers, reg, ltac::Operator::MOV, *this);
}

void ltac::RegisterManager::spills(ltac::FloatRegister reg){
    log::emit<Trace>("Registers") << "Spills Float Register " << reg << log::endl;
    ::spills(float_registers, reg, ltac::Operator::FMOV, *this);
}

void ltac::RegisterManager::spills_if_necessary(ltac::Register reg, mtac::Argument arg){
    if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&arg)){
        if(!registers.inRegister(*ptr, reg)){
            spills(reg);
        }
    } else {
        spills(reg);
    }
}

void ltac::RegisterManager::spills_all(){
    ::spills_all(registers, *this);
    ::spills_all(float_registers, *this);
}

ltac::Register ltac::RegisterManager::get_free_reg(){
    auto reg = ::get_free_reg(registers, *this);
    reserve(reg);
    return reg;
}

ltac::FloatRegister ltac::RegisterManager::get_free_float_reg(){
    auto reg = ::get_free_reg(float_registers, *this);
    reserve(reg);
    return reg;
}

bool ltac::RegisterManager::is_live(std::shared_ptr<Variable> variable, mtac::Statement statement){
    if(liveness->IN_S[statement].values().find(variable) != liveness->IN_S[statement].values().end()){
        return true;
    } else if(liveness->OUT_S[statement].values().find(variable) != liveness->OUT_S[statement].values().end()){
        return true;
    } else {
        return false;
    }
}

bool ltac::RegisterManager::is_escaped(std::shared_ptr<Variable> variable){
    if(pointer_escaped->count(variable)){
        log::emit<Trace>("Registers") << variable->name() << " is escaped " << log::endl;

        return true;
    }

    log::emit<Trace>("Registers") << variable->name() << " is not escaped " << log::endl;

    return false;
}

bool ltac::RegisterManager::is_live(std::shared_ptr<Variable> variable){
    auto live = is_live(variable, current);
    log::emit<Trace>("Registers") << variable->name() << " is live " << live << log::endl;
    return live;
}
    
void ltac::RegisterManager::collect_parameters(std::shared_ptr<eddic::Function> definition, PlatformDescriptor* descriptor){
    for(auto parameter : definition->parameters){
        auto param = definition->context->getVariable(parameter.name);

        if(param->position().isParamRegister()){
            if(mtac::is_single_int_register(param->type())){
                registers.setLocation(param, ltac::Register(descriptor->int_param_register(param->position().offset())));
            } else if(mtac::is_single_float_register(param->type())){
                float_registers.setLocation(param, ltac::FloatRegister(descriptor->float_param_register(param->position().offset())));
            }
        }
    }
}

void ltac::RegisterManager::collect_variables(std::shared_ptr<eddic::Function> definition, PlatformDescriptor* descriptor){
    for(auto& variable_pair : definition->context->stored_variables()){
        auto variable = variable_pair.second;

        if(variable->position().is_register()){
            if(mtac::is_single_int_register(variable->type())){
                registers.setLocation(variable, ltac::Register(descriptor->int_variable_register(variable->position().offset())));
            } else if(mtac::is_single_float_register(variable->type())){
                float_registers.setLocation(variable, ltac::FloatRegister(descriptor->float_variable_register(variable->position().offset())));
            }
        }
    }
}

void ltac::RegisterManager::restore_pushed_registers(){
    //Restore the int parameters in registers (in the reverse order they were pushed)
    for(auto& reg : boost::adaptors::reverse(int_pushed)){
        access_compiler()->pop(reg);
    }

    //Restore the float parameters in registers (in the reverse order they were pushed)
    for(auto& reg : boost::adaptors::reverse(float_pushed)){
        ltac::add_instruction(function, ltac::Operator::FMOV, reg, ltac::Address(ltac::SP, 0));
        ltac::add_instruction(function, ltac::Operator::ADD, ltac::SP, static_cast<int>(FLOAT->size()));
        access_compiler()->bp_offset -= FLOAT->size();
    }

    //Each register has been restored
    int_pushed.clear();
    float_pushed.clear();

    //All the parameters have been handled by now, the next param will be the first for its call
    first_param = true;
}

void ltac::RegisterManager::save_registers(std::shared_ptr<mtac::Param> param, PlatformDescriptor* descriptor){
    if(first_param){
        if(param->function){
            std::set<ltac::Register> overriden_registers;
            std::set<ltac::FloatRegister> overriden_float_registers;
    
            if(param->function->standard || option_defined("fparameter-allocation")){
                unsigned int maxInt = descriptor->numberOfIntParamRegisters();
                unsigned int maxFloat = descriptor->numberOfFloatParamRegisters();

                for(auto& parameter : param->function->parameters){
                    auto type = param->function->getParameterType(parameter.name);
                    unsigned int position = param->function->getParameterPositionByType(parameter.name);

                    if(mtac::is_single_int_register(type) && position <= maxInt){
                        overriden_registers.insert(ltac::Register(descriptor->int_param_register(position)));
                    }

                    if(mtac::is_single_float_register(type) && position <= maxFloat){
                        overriden_float_registers.insert(ltac::FloatRegister(descriptor->float_param_register(position)));
                    }
                }
            }

            if(param->function->context){
                for(auto variable_pair : param->function->context->stored_variables()){
                    auto variable = variable_pair.second;

                    if(variable->position().is_register()){
                        if(variable->type() == INT){
                            overriden_registers.insert(ltac::Register(variable->position().offset()));
                        } else {
                            overriden_float_registers.insert(ltac::FloatRegister(variable->position().offset()));
                        }
                    }
                }
            }

            for(auto& reg : overriden_registers){
                //If the parameter register is already used by a variable or a parent parameter
                if(!registers.reserved(reg) && registers.used(reg)){
                    if(registers[reg]->position().isParamRegister() || registers[reg]->position().is_register()){
                        int_pushed.push_back(reg);
                        access_compiler()->push(reg);
                    } else {
                        spills(reg);
                    }
                }
            }
            
            for(auto& reg : overriden_float_registers){
                //If the parameter register is already used by a variable or a parent parameter
                if(float_registers.used(reg)){
                    if(float_registers[reg]->position().isParamRegister() || float_registers[reg]->position().is_register()){
                        float_pushed.push_back(reg);

                        ltac::add_instruction(function, ltac::Operator::SUB, ltac::SP, static_cast<int>(FLOAT->size()));
                        ltac::add_instruction(function, ltac::Operator::FMOV, ltac::Address(ltac::SP, 0), reg);
                        access_compiler()->bp_offset += FLOAT->size();
                    } else {
                        spills(reg);
                    }
                }
            }
        }

        //The following parameters are for the same call
        first_param = false;
    }
}

void ltac::RegisterManager::set_current(mtac::Statement statement){
    current = statement;

    log::emit<Trace>("Registers") << "Current statement " << statement << log::endl;
}
        
bool ltac::RegisterManager::is_written(std::shared_ptr<Variable> variable){
    return written.find(variable) != written.end();
}

void ltac::RegisterManager::set_written(std::shared_ptr<Variable> variable){
    written.insert(variable);
}

std::shared_ptr<ltac::StatementCompiler> ltac::RegisterManager::access_compiler(){
    if(auto ptr = compiler.lock()){
        return ptr;
    }

    ASSERT_PATH_NOT_TAKEN("The shared_ptr on StatementCompiler has expired");
}
