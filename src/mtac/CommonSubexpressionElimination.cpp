//=======================================================================
// Copyright Baptiste Wicht 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include "assert.hpp"
#include "Variable.hpp"
#include "FunctionContext.hpp"
#include "Type.hpp"
#include "iterators.hpp"

#include "mtac/CommonSubexpressionElimination.hpp"
#include "mtac/Utils.hpp"
#include "mtac/Printer.hpp"
#include "mtac/Statement.hpp"

#include "ltac/Statement.hpp"

using namespace eddic;

typedef mtac::CommonSubexpressionElimination::ProblemDomain ProblemDomain;

std::ostream& mtac::operator<<(std::ostream& os, Expression& expression){
    mtac::Printer printer;
    os << "Expression {expression = ";
    printer.print_inline(expression.expression, os);
    return os << "}";
}

inline bool are_equivalent(std::shared_ptr<mtac::Quadruple> first, std::shared_ptr<mtac::Quadruple> second){
    return first->op == second->op && *first->arg1 == *second->arg1 && *first->arg2 == *second->arg2;
}

ProblemDomain mtac::CommonSubexpressionElimination::meet(ProblemDomain& in, ProblemDomain& out){
    eddic_assert(!in.top() || !out.top(), "At least one lattice should not be a top element");

    if(in.top()){
        return out;
    } else if(out.top()){
        return in;
    } else {
        typename ProblemDomain::Values values;

        for(auto& in_value : in.values()){
            for(auto& out_value : out.values()){
                if(are_equivalent(in_value.expression, out_value.expression)){
                    values.push_back(in_value);
                }
            }
        }

        ProblemDomain result(values);
        return result;
    }
}

ProblemDomain mtac::CommonSubexpressionElimination::transfer(mtac::basic_block_p basic_block, mtac::Statement& statement, ProblemDomain& in){
    auto out = in;

    if(auto* ptr = boost::get<std::shared_ptr<mtac::Quadruple>>(&statement)){
        auto op = (*ptr)->op;

        if(mtac::is_expression(op)){
            bool exists = false;
            for(auto& expression : out.values()){
                if(are_equivalent(*ptr, expression.expression)){
                    exists = true;
                    break;
                }
            }

            if(!exists){
                Expression expression;
                expression.expression = *ptr;
                expression.source = basic_block;

                out.values().push_back(expression);
            }
        }

        if(mtac::erase_result(op)){
            auto it = iterate(out.values());

            while(it.has_next()){
                auto& expression = (*it).expression;

                if(expression->arg1){
                    if(auto* var_ptr = boost::get<std::shared_ptr<Variable>>(&*expression->arg1)){
                        if(*var_ptr == (*ptr)->result){
                            it.erase();
                            continue;
                        }
                    }
                }
                
                if(expression->arg2){
                    if(auto* var_ptr = boost::get<std::shared_ptr<Variable>>(&*expression->arg2)){
                        if(*var_ptr == (*ptr)->result){
                            it.erase();
                            continue;
                        }
                    }
                }

                ++it;
            }
        }
    }

    return out;
}

ProblemDomain mtac::CommonSubexpressionElimination::Boundary(mtac::function_p /*function*/){
    return default_element();
}

ProblemDomain mtac::CommonSubexpressionElimination::Init(mtac::function_p function){
    if(init){
        ProblemDomain result(*init);
        return result;
    }

    typename ProblemDomain::Values values;
    
    for(auto& block : function){
        for(auto& statement : block->statements){
            if(auto* ptr = boost::get<std::shared_ptr<mtac::Quadruple>>(&statement)){
                if(mtac::is_expression((*ptr)->op)){
                    bool exists = false;
                    for(auto& expression : values){
                        if(are_equivalent(*ptr, expression.expression)){
                            exists = true;
                            break;
                        }
                    }
                    
                    if(!exists){
                        Expression expression;
                        expression.expression = *ptr;
                        expression.source = block;

                        values.push_back(expression);
                    }
                }
            }
        }
    }

    init = values;
    
    ProblemDomain result(values);
    return result;
}

bool mtac::CommonSubexpressionElimination::optimize(mtac::Statement& statement, std::shared_ptr<mtac::DataFlowResults<ProblemDomain>> global_results){
    auto& results = global_results->IN_S[statement];

    if(results.top()){
        return false;
    }

    if(auto* ptr = boost::get<std::shared_ptr<mtac::Quadruple>>(&statement)){
        auto quadruple = *ptr;

        if(mtac::is_expression(quadruple->op)){
            for(auto& expression : results.values()){
                auto source_statement = expression.expression;

                if(are_equivalent(source_statement, quadruple)){

                    mtac::Operator assign_op;
                    if(quadruple->op >= mtac::Operator::ADD && quadruple->op <= mtac::Operator::MOD){
                        assign_op = mtac::Operator::ASSIGN;
                    } else {
                        assign_op = mtac::Operator::FASSIGN;
                    } 

                    if(optimized.find(source_statement) == optimized.end()){
                        std::shared_ptr<Variable> temp;
                        if(quadruple->op >= mtac::Operator::ADD && quadruple->op <= mtac::Operator::MOD){
                            temp = expression.source->context->new_temporary(INT);
                        } else {
                            temp = expression.source->context->new_temporary(FLOAT);
                        } 

                        auto it = expression.source->statements.begin();
                        auto end = expression.source->statements.end();

                        while(it != end){
                            if(boost::get<std::shared_ptr<Quadruple>>(&*it)){
                                auto target = boost::get<std::shared_ptr<Quadruple>>(*it);
                                if(target == source_statement){
                                    auto quadruple = std::make_shared<mtac::Quadruple>(source_statement->result, temp, assign_op);

                                    ++it;
                                    expression.source->statements.insert(it, quadruple);

                                    break;
                                }
                            }

                            ++it;
                        }
                        
                        source_statement->result = temp;
                        
                        optimized.insert(source_statement);
                    }

                    if(optimized.find(quadruple) == optimized.end()){
                        quadruple->op = assign_op;
                        quadruple->arg1 = source_statement->result;
                        quadruple->arg2.reset();

                        optimized.insert(quadruple);

                        return true;
                    } 
                }
            }
        }
    }

    return false;
}
