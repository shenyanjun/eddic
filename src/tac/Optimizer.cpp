//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <boost/variant.hpp>

#include "tac/Optimizer.hpp"
#include "tac/Program.hpp"
#include "tac/Utils.hpp"

using namespace eddic;

namespace {

enum class Pass : unsigned int {
    DATA_MINING,
    OPTIMIZE
};

struct ArithmeticIdentities : public boost::static_visitor<tac::Statement> {
    bool optimized;

    ArithmeticIdentities() : optimized(false) {}

    tac::Statement operator()(std::shared_ptr<tac::Quadruple>& quadruple){
        bool old = optimized;
        optimized = true;

        if(quadruple->op){
            switch(*quadruple->op){
                case tac::Operator::ADD:
                    if(tac::equals<int>(quadruple->arg1, 0)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, *quadruple->arg2);
                    } else if(tac::equals<int>(*quadruple->arg2, 0)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, quadruple->arg1);
                    }

                    break;
                case tac::Operator::SUB:
                    if(tac::equals<int>(*quadruple->arg2, 0)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, quadruple->arg1);
                    } 

                    //a = b - b => a = 0
                    if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&quadruple->arg1)){
                        if(tac::equals<std::shared_ptr<Variable>>(*quadruple->arg2, *ptr)){
                            return std::make_shared<tac::Quadruple>(quadruple->result, 0);
                        }
                    }
                    
                    //a = 0 - b => a = -b
                    if(tac::equals<int>(quadruple->arg1, 0)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, *quadruple->arg2, tac::Operator::MINUS);
                    }

                    break;
                case tac::Operator::MUL:
                    if(tac::equals<int>(quadruple->arg1, 1)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, *quadruple->arg2);
                    } else if(tac::equals<int>(*quadruple->arg2, 1)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, quadruple->arg1);
                    }
                    
                    if(tac::equals<int>(quadruple->arg1, 0)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, 0);
                    } else if(tac::equals<int>(*quadruple->arg2, 0)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, 0);
                    }
                    
                    if(tac::equals<int>(quadruple->arg1, -1)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, *quadruple->arg2, tac::Operator::MINUS);
                    } else if(tac::equals<int>(*quadruple->arg2, -1)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, quadruple->arg1, tac::Operator::MINUS);
                    }

                    break;
                case tac::Operator::DIV:
                    if(tac::equals<int>(*quadruple->arg2, 1)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, quadruple->arg1);
                    }

                    if(tac::equals<int>(quadruple->arg1, 0)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, 0);
                    }

                    //a = b / b => a = 1
                    if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&quadruple->arg1)){
                        if(tac::equals<std::shared_ptr<Variable>>(*quadruple->arg2, *ptr)){
                            return std::make_shared<tac::Quadruple>(quadruple->result, 1);
                        }
                    }
                    
                    if(tac::equals<int>(*quadruple->arg2, 1)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, quadruple->arg1, tac::Operator::MINUS);
                    }

                    break;
                default:
                    break;
            }
        }

        optimized = old;
        return quadruple;
    }

    template<typename T>
    tac::Statement operator()(T& statement) const { 
        return statement;
    }
};

struct ReduceInStrength : public boost::static_visitor<tac::Statement> {
    bool optimized;

    ReduceInStrength() : optimized(false) {}

    tac::Statement operator()(std::shared_ptr<tac::Quadruple>& quadruple){
        bool old = optimized;
        optimized = true;

        if(quadruple->op){
            switch(*quadruple->op){
                case tac::Operator::MUL:
                    if(tac::equals<int>(quadruple->arg1, 2)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, *quadruple->arg2, tac::Operator::ADD, *quadruple->arg2);
                    } else if(tac::equals<int>(*quadruple->arg2, 2)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, quadruple->arg1, tac::Operator::ADD, quadruple->arg1);
                    }

                    break;
                default:
                    break;
            }
        }

        optimized = old;
        return quadruple;
    }

    template<typename T>
    tac::Statement operator()(T& statement) const { 
        return statement;
    }
};

struct ConstantFolding : public boost::static_visitor<tac::Statement> {
    bool optimized;

    ConstantFolding() : optimized(false) {}

    tac::Statement operator()(std::shared_ptr<tac::Quadruple>& quadruple){
        bool old = optimized;
        optimized = true;
        
        if(quadruple->op){
            switch(*quadruple->op){
                case tac::Operator::ADD:
                    if(tac::isInt(quadruple->arg1) && tac::isInt(*quadruple->arg2)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, 
                            boost::get<int>(quadruple->arg1) + boost::get<int>(*quadruple->arg2));
                    }

                    break;
                case tac::Operator::SUB:
                    if(tac::isInt(quadruple->arg1) && tac::isInt(*quadruple->arg2)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, 
                            boost::get<int>(quadruple->arg1) - boost::get<int>(*quadruple->arg2));
                    }

                    break;
                case tac::Operator::MUL:
                    if(tac::isInt(quadruple->arg1) && tac::isInt(*quadruple->arg2)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, 
                            boost::get<int>(quadruple->arg1) * boost::get<int>(*quadruple->arg2));
                    }

                    break;
                case tac::Operator::DIV:
                    if(tac::isInt(quadruple->arg1) && tac::isInt(*quadruple->arg2)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, 
                            boost::get<int>(quadruple->arg1) / boost::get<int>(*quadruple->arg2));
                    }

                    break;
                case tac::Operator::MOD:
                    if(tac::isInt(quadruple->arg1) && tac::isInt(*quadruple->arg2)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, 
                            boost::get<int>(quadruple->arg1) % boost::get<int>(*quadruple->arg2));
                    }
                case tac::Operator::MINUS:
                    if(tac::isInt(quadruple->arg1)){
                        return std::make_shared<tac::Quadruple>(quadruple->result, -1 * boost::get<int>(quadruple->arg1));
                    }

                    break;
                default:
                    break;
            }
        }

        optimized = old;
        return quadruple;
    }

    tac::Statement operator()(std::shared_ptr<tac::IfFalse>& ifFalse){
        if(tac::isInt(ifFalse->arg1) && tac::isInt(ifFalse->arg2)){
            int left = boost::get<int>(ifFalse->arg1);
            int right = boost::get<int>(ifFalse->arg2);

            bool value = false;

            switch(ifFalse->op){
                case tac::BinaryOperator::EQUALS:
                    value = left == right;

                    break;
                case tac::BinaryOperator::NOT_EQUALS:
                    value = left != right;

                    break;
                case tac::BinaryOperator::LESS:
                    value = left < right;

                    break;
                case tac::BinaryOperator::LESS_EQUALS:
                    value = left <= right;

                    break;
                case tac::BinaryOperator::GREATER:
                    value = left > right;

                    break;
                case tac::BinaryOperator::GREATER_EQUALS:
                    value = left >= right;

                    break;
            }

            //replace if_false true by no-op
            if(value){
               return tac::NoOp();
            } 
            //replace if_false false by goto 
            else {
               auto goto_ = std::make_shared<tac::Goto>();
               
               goto_->label = ifFalse->label;
               goto_->block = ifFalse->block;

               return goto_; 
            }
        }

        return ifFalse;
    }

    template<typename T>
    tac::Statement operator()(T& statement) const { 
        return statement;
    }
};

struct ConstantPropagation : public boost::static_visitor<tac::Statement> {
    bool optimized;

    ConstantPropagation() : optimized(false) {}

    std::unordered_map<std::shared_ptr<Variable>, int> constants;

    tac::Statement operator()(std::shared_ptr<tac::Quadruple>& quadruple){
        bool old = optimized;
        optimized = true;

        if(!quadruple->op){
            if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&quadruple->arg1)){
                if(constants.find(*ptr) != constants.end()){
                    quadruple->arg1 = constants[*ptr];
                }
            }

            if(auto* ptr = boost::get<int>(&quadruple->arg1)){
                constants[quadruple->result] = *ptr;
            }
        } else {
            if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&quadruple->arg1)){
                if(constants.find(*ptr) != constants.end()){
                    quadruple->arg1 = constants[*ptr];
                }
            }

            if(quadruple->arg2){
                if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&*quadruple->arg2)){
                    if(constants.find(*ptr) != constants.end()){
                        quadruple->arg2 = constants[*ptr];
                    }
                }
            }

            //The result is not constant at this point
            constants.erase(quadruple->result);
        }

        optimized = old;
        return quadruple;
    }

    template<typename T>
    tac::Statement operator()(T& statement){ 
        return statement;
    }
};

struct RemoveAssign : public boost::static_visitor<bool> {
    bool optimized;
    Pass pass;

    RemoveAssign() : optimized(false) {}

    std::unordered_set<std::shared_ptr<Variable>> used;

    bool operator()(std::shared_ptr<tac::Quadruple>& quadruple){
        if(pass == Pass::DATA_MINING){
            if(!quadruple->op){
                if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&quadruple->arg1)){
                    used.insert(*ptr);
                }
            } else {
                if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&quadruple->arg1)){
                    used.insert(*ptr);
                }

                if(quadruple->arg2){
                    if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&*quadruple->arg2)){
                        used.insert(*ptr);
                    }
                }
            }
            
            return true;
        } else {
            //These operators are not erasing result
            if(quadruple->op && (*quadruple->op == tac::Operator::PARAM || *quadruple->op == tac::Operator::DOT_ASSIGN || *quadruple->op == tac::Operator::ARRAY_ASSIGN)){
                return true;
            }

            if(used.find(quadruple->result) == used.end()){
                //The other kind of variables can be used in other basic block
                if(quadruple->result->position().isTemporary()){
                    optimized = true;
                    return false;
                }
            }

            return true;
        }
    }
    
    template<typename T>
    bool operator()(T&){ 
        return true;
    }
};

template<typename Visitor>
bool apply_to_all(tac::Program& program){
    Visitor visitor;

    for(auto& function : program.functions){
        for(auto& block : function->getBasicBlocks()){
            for(auto& statement : block->statements){
                statement = boost::apply_visitor(visitor, statement);
            }
        }
    }

    return visitor.optimized;
}

template<typename Visitor>
bool apply_to_basic_blocks(tac::Program& program){
    bool optimized = false;

    for(auto& function : program.functions){
        for(auto& block : function->getBasicBlocks()){
            Visitor visitor;

            for(auto& statement : block->statements){
                statement = boost::apply_visitor(visitor, statement);
            }

            optimized |= visitor.optimized;
        }
    }

    return optimized;
}

template<typename Visitor>
bool apply_to_basic_blocks_two_pass(tac::Program& program){
    bool optimized = false;

    for(auto& function : program.functions){
        for(auto& block : function->getBasicBlocks()){
            Visitor visitor;
            visitor.pass = Pass::DATA_MINING;

            auto it = block->statements.begin();
            auto end = block->statements.end();

            while(it != end){
                bool keep = boost::apply_visitor(visitor, *it);
                
                if(!keep){
                    it = block->statements.erase(it);   
                }

                it++;
            }

            visitor.pass = Pass::OPTIMIZE;

            it = block->statements.begin();
            end = block->statements.end();

            while(it != end){
                bool keep = boost::apply_visitor(visitor, *it);
                
                if(!keep){
                    it = block->statements.erase(it);   
                }

                it++;
            }

            optimized |= visitor.optimized;
        }
    }

    return optimized;
}

}

void tac::Optimizer::optimize(tac::Program& program) const {
    bool optimized;
    do {
        optimized = false;

        //Optimize using arithmetic identities
        optimized |= apply_to_all<ArithmeticIdentities>(program);
        
        //Reduce arithtmetic instructions in strength
        optimized |= apply_to_all<ReduceInStrength>(program);

        //Constant folding
        optimized |= apply_to_all<ConstantFolding>(program);

        //Constant propagation
        optimized |= apply_to_basic_blocks<ConstantPropagation>(program);

        //Remove unused assignations
        optimized |= apply_to_basic_blocks_two_pass<RemoveAssign>(program);
    } while (optimized);
    
    //TODO Copy propagation
    //TODO Find a way to optimize branches in a way that dead code is never outputted and there are no jump if not necessary
}
