//=======================================================================
// Copyright Baptiste Wicht 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <unordered_map>

#include <boost/graph/dominator_tree.hpp>

#include "iterators.hpp"
#include "VisitorUtils.hpp"

#include "mtac/loop_optimizations.hpp"
#include "mtac/loop_analysis.hpp"
#include "mtac/ControlFlowGraph.hpp"
#include "mtac/Utils.hpp"

using namespace eddic;

namespace {

typedef mtac::ControlFlowGraph::InternalControlFlowGraph G;
typedef std::set<mtac::ControlFlowGraph::BasicBlockInfo> Loop;
typedef mtac::ControlFlowGraph::BasicBlockInfo Vertex;
typedef boost::property_map<mtac::ControlFlowGraph::InternalControlFlowGraph, boost::vertex_index_t>::type IndexMap;
typedef boost::iterator_property_map<std::vector<Vertex>::iterator, IndexMap> PredMap;

struct Usage {
    std::unordered_map<std::shared_ptr<Variable>, unsigned int> written;
};

Usage compute_usage(const Loop& loop, const G& g){
    Usage usage;

    for(auto& vertex : loop){
        auto bb = g[vertex].block;
        
        for(auto& statement : bb->statements){
            if(auto* ptr = boost::get<std::shared_ptr<mtac::Quadruple>>(&statement)){
                if(mtac::erase_result((*ptr)->op)){
                    ++(usage.written[(*ptr)->result]);
                }
            }
        }
    }

    return usage;
}

bool is_invariant(boost::optional<mtac::Argument>& argument, Usage& usage){
    if(argument){
        if(auto* ptr = boost::get<std::shared_ptr<Variable>>(&*argument)){
            return usage.written[*ptr] == 0;
        }
    }

    return true;
}

bool is_invariant(mtac::Statement& statement, Usage& usage){
    if(auto* ptr = boost::get<std::shared_ptr<mtac::Quadruple>>(&statement)){
        auto quadruple = *ptr;

        if(mtac::erase_result(quadruple->op)){
            //If there are more than one write to this variable, the computation is not invariant
            if(usage.written[quadruple->result] > 1){
                return false;
            }

            return is_invariant(quadruple->arg1, usage) && is_invariant(quadruple->arg2, usage);
        }

        return false;
    }

    return false;
}

std::shared_ptr<mtac::BasicBlock> create_pre_header(const Loop& loop, std::shared_ptr<mtac::Function> function, const G& g){
    //Create a new basic block and detach it from the function
    auto pre_header = function->newBasicBlock();
    function->getBasicBlocks().pop_back();
    
    auto first_bb = g[*loop.begin()].block;

    auto bit = iterate(function->getBasicBlocks());

    while(bit.has_next()){
        if(*bit == first_bb){
            bit.insert(pre_header);

            break;
        }

        ++bit;
    }
    
    return pre_header;
}

struct UsageCollector : public boost::static_visitor<bool> {
    std::shared_ptr<Variable> var;

    UsageCollector(std::shared_ptr<Variable> var) : var(var) {}

    template<typename T>
    bool collect(T& arg){
        if(auto* variablePtr = boost::get<std::shared_ptr<Variable>>(&arg)){
            return *variablePtr == var;
        }

        return false;
    }

    template<typename T>
    bool collect_optional(T& opt){
        if(opt){
            return collect(*opt);
        }

        return false;
    }

    bool operator()(std::shared_ptr<mtac::Quadruple> quadruple){
        return quadruple->result == var || collect_optional(quadruple->arg1) || collect_optional(quadruple->arg2);
    }
    
    bool operator()(std::shared_ptr<mtac::Param> param){
        return collect(param->arg);
    }
    
    bool operator()(std::shared_ptr<mtac::If> if_){
        return collect(if_->arg1) || collect_optional(if_->arg2);
    }
    
    bool operator()(std::shared_ptr<mtac::IfFalse> if_false){
        return collect(if_false->arg1) || collect_optional(if_false->arg2);
    }

    template<typename T>
    bool operator()(T&){
        return false;
    }
};

bool use_variable(std::shared_ptr<mtac::BasicBlock> bb, std::shared_ptr<Variable> var){
    UsageCollector collector(var);

    for(auto& statement : bb->statements){
        if(visit(collector, statement)){
            return true;
        }
    }

    return false;
}

/*!
 * \brief Test if an invariant is valid or not. 
 * An invariant defining v is valid if: 
 * 1. It is in a basic block that dominates all other uses of v
 * 2. It is in a basic block that dominates all exit blocks of the loop
 */
bool is_valid_invariant(std::shared_ptr<mtac::BasicBlock> source_bb, mtac::Statement statement, const Loop& loop, const G& g, const PredMap& domTreePredMap){
    auto quadruple = boost::get<std::shared_ptr<mtac::Quadruple>>(statement);
    auto var = quadruple->result;

    for(auto& vertex : loop){
        auto bb = g[vertex].block;

        //A bb always dominates itself => no need to consider the source basic block
        if(bb != source_bb){
            if(use_variable(bb, var)){
                if(boost::get(domTreePredMap, vertex) != boost::graph_traits<G>::null_vertex()){
                    auto dominator = boost::get(domTreePredMap, vertex);

                    //If the bb is not dominated by the source bb, it is not valid
                    if(g[dominator].block != source_bb){
                        return false;
                    }
                }
            }
        }
    }
    
    auto exit_block = *loop.rbegin();

    if(g[exit_block].block == source_bb){
        return true;
    }
                
    if(boost::get(domTreePredMap, exit_block) != boost::graph_traits<G>::null_vertex()){
        auto dominator = boost::get(domTreePredMap, exit_block);

        //If the exit bb is not dominated by the source bb, it is not valid
        if(g[dominator].block != source_bb){
            return false;
        }
    }
    
    return true;
}

bool loop_invariant_code_motion(const Loop& loop, std::shared_ptr<mtac::Function> function, const G& g, const PredMap& domTreePredMap){
    std::shared_ptr<mtac::BasicBlock> pre_header;

    bool optimized = false;

    auto usage = compute_usage(loop, g);

    for(auto& vertex : loop){
        auto bb = g[vertex].block;

        auto it = iterate(bb->statements); 

        while(it.has_next()){
            auto statement = *it;

            if(is_invariant(statement, usage)){
                if(is_valid_invariant(bb, statement, loop, g, domTreePredMap)){
                    //Create the preheader if necessary
                    if(!pre_header){
                        pre_header = create_pre_header(loop, function, g);
                    }

                    it.erase();
                    pre_header->statements.push_back(statement);

                    optimized = true;

                    continue;
                } 
            }

            ++it;
        }
    }

    return optimized;
}

struct LinearEquation {
    std::shared_ptr<Variable> i;
    int e;
    int d;
};

typedef std::unordered_map<std::shared_ptr<Variable>, LinearEquation> InductionVariables;

InductionVariables find_all_candidates(const Loop& loop, const G& g){
    std::unordered_map<std::shared_ptr<Variable>, LinearEquation> candidates;
    
    for(auto& vertex : loop){
        auto bb = g[vertex].block;

        for(auto& statement : bb->statements){
            if(auto* ptr = boost::get<std::shared_ptr<mtac::Quadruple>>(&statement)){
                if(mtac::erase_result((*ptr)->op)){
                    candidates[(*ptr)->result] = {(*ptr)->result, 0, 0};
                }
            }
        }
    }

    return candidates;
}

InductionVariables find_basic_induction_variables(const Loop& loop, const G& g){
    auto basic_induction_variables = find_all_candidates(loop, g);

    for(auto& vertex : loop){
        auto bb = g[vertex].block;

        for(auto& statement : bb->statements){
            if(auto* ptr = boost::get<std::shared_ptr<mtac::Quadruple>>(&statement)){
                auto quadruple = *ptr;
                auto var = quadruple->result;

                //If it is not a candidate, do not test it
                if(!basic_induction_variables.count(var)){
                    continue;
                }

                auto value = basic_induction_variables[var];

                //TODO In the future, induction variables written several times could be splitted into several induction variables
                if(value.e != 0){
                    basic_induction_variables.erase(var);

                    continue;
                }

                if(quadruple->op == mtac::Operator::ADD){
                    auto arg1 = *quadruple->arg1;
                    auto arg2 = *quadruple->arg2;

                    if(mtac::isInt(arg1) && mtac::equals(arg2, var)){
                        basic_induction_variables[var] = {var, 1, boost::get<int>(arg1)};
                    } else if(mtac::isInt(arg2) && mtac::equals(arg1, var)){
                        basic_induction_variables[var] = {var, 1, boost::get<int>(arg2)}; 
                    } else {
                        basic_induction_variables.erase(var);
                    }
                } else {
                    basic_induction_variables.erase(var);
                }
            } else if(auto* ptr = boost::get<std::shared_ptr<mtac::Call>>(&statement)){
                auto call = *ptr;

                if(call->return_){
                    basic_induction_variables.erase(call->return_);
                }

                if(call->return2_){
                    basic_induction_variables.erase(call->return2_);
                }
            }
        }
    }

    return basic_induction_variables;
}

InductionVariables find_dependent_induction_variables(const Loop& loop, const G& g, InductionVariables& basic_induction_variables){
    auto dependent_induction_variables = find_all_candidates(loop, g);

    for(auto& vertex : loop){
        auto bb = g[vertex].block;

        for(auto& statement : bb->statements){
            if(auto* ptr = boost::get<std::shared_ptr<mtac::Quadruple>>(&statement)){
                auto quadruple = *ptr;
                auto var = quadruple->result;

                //If it is not a candidate, do not test it
                if(!dependent_induction_variables.count(var)){
                    continue;
                }
                
                //If it is a basic induction variable, it is not a dependent induction variable
                if(basic_induction_variables.count(var)){
                    dependent_induction_variables.erase(var);

                    continue;
                }

                auto value = dependent_induction_variables[var];

                //TODO In the future, induction variables written several times could be splitted into several induction variables
                if(value.e != 0){
                    dependent_induction_variables.erase(var);

                    continue;
                }

                if(quadruple->op == mtac::Operator::MUL){
                    auto arg1 = *quadruple->arg1;
                    auto arg2 = *quadruple->arg2;

                    if(mtac::isInt(arg1) && mtac::isVariable(arg2)){
                        auto variable = boost::get<std::shared_ptr<Variable>>(arg2);

                        if(basic_induction_variables.count(variable)){
                            dependent_induction_variables[var] = {variable, boost::get<int>(arg1), 0}; 
                            continue;
                        }
                    } else if(mtac::isInt(arg2) && mtac::isVariable(arg1)){
                        auto variable = boost::get<std::shared_ptr<Variable>>(arg1);
                        
                        if(basic_induction_variables.count(variable)){
                            dependent_induction_variables[var] = {variable, boost::get<int>(arg2), 0}; 
                            continue;
                        }
                    } 
                } else if(quadruple->op == mtac::Operator::ADD){
                    auto arg1 = *quadruple->arg1;
                    auto arg2 = *quadruple->arg2;

                    if(mtac::isInt(arg1) && mtac::isVariable(arg2)){
                        auto variable = boost::get<std::shared_ptr<Variable>>(arg2);

                        if(basic_induction_variables.count(variable)){
                            dependent_induction_variables[var] = {variable, 1, boost::get<int>(arg1)}; 
                            continue;
                        }
                    } else if(mtac::isInt(arg2) && mtac::isVariable(arg1)){
                        auto variable = boost::get<std::shared_ptr<Variable>>(arg1);
                        
                        if(basic_induction_variables.count(variable)){
                            dependent_induction_variables[var] = {variable, 1, boost::get<int>(arg2)}; 
                            continue;
                        }
                    }
                } else if(quadruple->op == mtac::Operator::SUB){
                    auto arg1 = *quadruple->arg1;
                    auto arg2 = *quadruple->arg2;

                    if(mtac::isInt(arg1) && mtac::isVariable(arg2)){
                        auto variable = boost::get<std::shared_ptr<Variable>>(arg2);

                        if(basic_induction_variables.count(variable)){
                            dependent_induction_variables[var] = {variable, -1, -1 * boost::get<int>(arg1)}; 
                            continue;
                        }
                    } else if(mtac::isInt(arg2) && mtac::isVariable(arg1)){
                        auto variable = boost::get<std::shared_ptr<Variable>>(arg1);
                        
                        if(basic_induction_variables.count(variable)){
                            dependent_induction_variables[var] = {variable, 1, -1 * boost::get<int>(arg2)}; 
                            continue;
                        }
                    } 
                } else if(quadruple->op == mtac::Operator::MINUS){
                    auto arg1 = *quadruple->arg1;

                    if(mtac::isVariable(arg1)){
                        auto variable = boost::get<std::shared_ptr<Variable>>(arg1);

                        if(basic_induction_variables.count(variable)){
                            dependent_induction_variables[var] = {variable, -1, 0}; 
                            continue;
                        }
                    } 
                } 
                
                dependent_induction_variables.erase(var);
            } else if(auto* ptr = boost::get<std::shared_ptr<mtac::Call>>(&statement)){
                auto call = *ptr;

                if(call->return_){
                    dependent_induction_variables.erase(call->return_);
                }

                if(call->return2_){
                    dependent_induction_variables.erase(call->return2_);
                }
            }
        }
    }

    return dependent_induction_variables;
}

bool loop_strength_reduction(const Loop& loop, std::shared_ptr<mtac::Function> function, const G& g){
    std::shared_ptr<mtac::BasicBlock> pre_header;

    bool optimized = false;

    auto basic = find_basic_induction_variables(loop, g);
    auto dependent = find_dependent_induction_variables(loop, g, basic);
    
    std::cout << "Basic induction variables" << std::endl;

    for(auto& biv : basic){
        std::cout << biv.first->name() << " = " << biv.second.e << " * " << biv.second.i->name() << " + " << biv.second.d << std::endl;
    }

    std::cout << "Dependent induction variables" << std::endl;
    
    for(auto& biv : dependent){
        std::cout << biv.first->name() << " = " << biv.second.e << " * " << biv.second.i->name() << " + " << biv.second.d << std::endl;
    }

    for(auto& vertex : loop){
        auto bb = g[vertex].block;


    }

    return optimized;
}

} //end of anonymous namespace

bool mtac::loop_invariant_code_motion(std::shared_ptr<mtac::Function> function){
    auto graph = mtac::build_control_flow_graph(function);
    auto g = graph->get_graph();

    auto natural_loops = find_natural_loops(g);

    if(natural_loops.empty()){
        return false;
    }

    bool optimized = false;
    
    std::vector<Vertex> domTreePredVector = std::vector<Vertex>(boost::num_vertices(g), boost::graph_traits<G>::null_vertex());
    PredMap domTreePredMap = boost::make_iterator_property_map(domTreePredVector.begin(), boost::get(boost::vertex_index, g));

    boost::lengauer_tarjan_dominator_tree(g, boost::vertex(0, g), domTreePredMap);

    for(auto& loop : natural_loops){
        optimized |= ::loop_invariant_code_motion(loop, function, g, domTreePredMap);
    }
    
    return optimized;
}

bool mtac::loop_strength_reduction(std::shared_ptr<mtac::Function> function){
    auto graph = mtac::build_control_flow_graph(function);
    auto g = graph->get_graph();
    
    auto natural_loops = find_natural_loops(g);

    if(natural_loops.empty()){
        return false;
    }

    bool optimized = false;
    
    for(auto& loop : natural_loops){
        optimized |= ::loop_strength_reduction(loop, function, g);
    }

    return optimized;
}
