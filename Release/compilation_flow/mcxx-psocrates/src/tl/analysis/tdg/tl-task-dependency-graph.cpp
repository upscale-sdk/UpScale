/*--------------------------------------------------------------------
 (C) Copyright 2006-2014 Barcelona Supercomputing Center             *
 Centro Nacional de Supercomputacion

 This file is part of Mercurium C/C++ source-to-source compiler.

 See AUTHORS file in the top level directory for information
 regarding developers and contributors.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 Mercurium C/C++ source-to-source compiler is distributed in the hope
 that it will be useful, but WITHOUT ANY WARRANTY; without even the
 implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public
 License along with Mercurium C/C++ source-to-source compiler; if
 not, write to the Free Software Foundation, Inc., 675 Mass Ave,
 Cambridge, MA 02139, USA.
 --------------------------------------------------------------------*/

#include <algorithm>
#include <cassert>
#include <climits>
#include <fstream>
#include <list>
#include <queue>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>

#include "cxx-cexpr.h"
#include "tl-compilerpipeline.hpp"
#include "tl-pcfg-utils.hpp"
#include "tl-task-dependency-graph.hpp"

namespace TL { 
namespace Analysis {
    
    typedef std::map<unsigned int, TDG_Node*> TDG_Node_map;
    typedef ObjectList<TDG_Edge*> TDG_Edge_list;
    typedef ObjectList<Node*> Node_list;
    typedef ObjectList<Edge*> Edge_list;
    typedef std::multimap<Node*, NBase> Node_to_NBase_map;

    static int node_id;
    static int control_id = 0;
    static int tdg_var_id = 0;

    static std::string full_report_name;
    static FILE* report_file;

namespace {

    Node_to_NBase_map _reported_offset_vars;

    //! Returns true when there is a path between 'current' and 'task'
    bool task_is_in_path(Node* control_structure, Node* current, Node* task)
    {
        if (current->is_visited())
            return false;

        current->set_visited(true);

        // Only traverse the nodes that are inside control_structure
        if (current == control_structure->get_graph_exit_node())
            return false;

        // Return true only when we find the task traversing the current path
        if (current->is_omp_task_node()
            || current->is_omp_async_target_node()
            || /*taskpart*/ (current->is_function_call_node()
                && ((current->get_statements()[0].as<Nodecl::FunctionCall>().get_called().get_symbol().get_name() == "GOMP_init_taskpart")
                    || (current->get_statements()[0].as<Nodecl::FunctionCall>().get_called().get_symbol().get_name() == "GOMP_end_taskpart"))))
        {
            if (current == task)
                return true;
            else
                return false;   // Return false because we do not want to traverse tasks depending on other tasks
                                // but only reach a task when traversing its corresponding task creation node
        }

        bool result = false;

        // Traverse the inner nodes if 'current' is a graph node
        if (current->is_graph_node())
            result = task_is_in_path(control_structure, current->get_graph_entry_node(), task);

        // Traverse the children
        const ObjectList<Node*>& children = current->get_children();
        for (ObjectList<Node*>::const_iterator it = children.begin();
             it != children.end() && !result; ++it)
            result = task_is_in_path(control_structure, *it, task);

        return result;
    }
    
    NBase get_condition_stmts(Node* cond_node)
    {
        NBase cond_stmt;
        
        if(cond_node->is_graph_node())
            cond_stmt = cond_node->get_graph_related_ast();
        else
        {
            NodeclList stmts = cond_node->get_statements();
            ERROR_CONDITION(stmts.size()!=1, "%s statements found in node %d condition. Only one statement expected.\n", 
                            stmts.size(), cond_node->get_id());
            cond_stmt = stmts[0];
        }
        
        return cond_stmt;
    }
    
    //! TaskDependencyGraph :: Returns a nodecl containing the condition that must fulfill 
    //! to follow the branch of an ifelse that takes to 'task'
    NBase get_ifelse_condition_and_path(
            Node* control_structure,
            Node* task,
            std::string& taken_branch)
    {
        NBase condition;

        // Get the statements that form the condition
        Node* cond_node = control_structure->get_condition_node();
        NBase cond_stmt = get_condition_stmts(cond_node);
        
        // Find which path (TRUE|FALSE) takes to the task and compute the condition accordingly
        ObjectList<Edge*> exit_edges = cond_node->get_exit_edges();
        for (ObjectList<Edge*>::iterator it = exit_edges.begin();
             it != exit_edges.end(); ++it)
        {
            if (task_is_in_path(control_structure, (*it)->get_target(), task))
            {
                condition = cond_stmt;
                taken_branch = ((*it)->is_true_edge() ? "1" : "2");
                
                break;  // Stop iterating, for we already found the task
            }
        }

        // Clean up the graph from the visits
        const ObjectList<Node*>& children = cond_node->get_children();
        for (ObjectList<Node*>::const_iterator it = children.begin();
            it != children.end(); ++it)
            ExtensibleGraph::clear_visits_in_level(*it, control_structure);

        return condition;
    }

    void report_default_offset(Node* n, Node* loop, const NBase& var)
    {
        if (TDG_DEBUG)
        {
            // Check whether the variable has already been reported
            if (_reported_offset_vars.find(n) != _reported_offset_vars.end())
            {
                std::pair<Node_to_NBase_map::iterator, Node_to_NBase_map::iterator> n_reported_vars = _reported_offset_vars.equal_range(n);
                for (Node_to_NBase_map::iterator it = n_reported_vars.first; it != n_reported_vars.second; ++it)
                {
                    if (Nodecl::Utils::structurally_equal_nodecls(it->second, var, /*skip_conversion_nodes*/true))
                        return;
                }
            }

            // Report the variable and store to avoid reporting it again
            WARNING_MESSAGE("Retrieving values for variable %s in node %d which is the IV of loop %d."
                            "This is not yet implemented. We set the offset '0' by default.\n",
                            var.prettyprint().c_str(), n->get_id(), loop->get_id());
            _reported_offset_vars.insert(std::pair<Node*, NBase>(n, var));
        }
    }

    struct StructuralCompareBind1
    {
        NBase n1;

        StructuralCompareBind1(const NBase n1_) : n1(n1_) { }

        virtual bool operator()(const NBase& n2) const
        {
            return Nodecl::Utils::structurally_equal_nodecls(n1, n2);
        }
    };

    void transform_expression_to_json_expression(
            ControlStructure* cs_node,
            NBase& expression,
            VarToNodeclMap& var_to_value_map,
            VarToNodeclMap& var_to_id_map,
            ObjectList<NBase>& ordered_vars,
            unsigned int& last_var_id);

    void get_variable_values(
            ControlStructure* cs_node,
            const NBase& var,
            VarToNodeclMap& var_to_value_map,
            VarToNodeclMap& var_to_id_map,
            ObjectList<NBase>& ordered_vars,
            unsigned int& last_var_id)
    {
        NBase range;
        ControlStructure* current_cs_node = cs_node;
check_ivs:
        // 1.- Check whether the variable is an Induction Variable
        if(current_cs_node->get_type() == Loop)
        {
            const Utils::InductionVarList& ivs = current_cs_node->get_pcfg_node()->get_induction_variables();
            if(Utils::induction_variable_list_contains_variable(ivs, var))
            {   // The variable is an IV: 
                // If we are parsing the original Control Structure, get the values of the Induction Variable
                if(cs_node == current_cs_node)
                {
                    Utils::InductionVar* iv = get_induction_variable_from_list(ivs, var);
                    const NodeclSet& iv_lb = iv->get_lb();
                    ERROR_CONDITION(iv_lb.size()>1,
                                    "More than one LB found for IV %s. This is not yet implemented.\n",
                                    iv->get_variable().prettyprint().c_str());
                    NBase lb = iv_lb.begin()->shallow_copy();
                    transform_expression_to_json_expression(
                            current_cs_node, lb, var_to_value_map, var_to_id_map, ordered_vars, last_var_id);
                    const NodeclSet& iv_ub = iv->get_ub();
                    ERROR_CONDITION(iv_lb.size()>1,
                                    "More than one LB found for IV %s. This is not yet implemented.\n",
                                    iv->get_variable().prettyprint().c_str());
                    NBase ub = iv_ub.begin()->shallow_copy();
                    transform_expression_to_json_expression(
                            current_cs_node, ub, var_to_value_map, var_to_id_map, ordered_vars, last_var_id);
                    NBase incr = iv->get_increment().shallow_copy();
                    transform_expression_to_json_expression(
                            current_cs_node, incr, var_to_value_map, var_to_id_map, ordered_vars, last_var_id);
                    range = Nodecl::Range::make(lb, ub, incr, Type::get_int_type());
                }
                else
                {   // TODO We should compute the offset here
                    report_default_offset(cs_node->get_pcfg_node(), current_cs_node->get_pcfg_node(), var);
                    range = Nodecl::IntegerLiteral::make(Type::get_int_type(), const_value_get_zero(/*bytes*/ 4, /*signed*/ 1));
                }
                goto end_get_vars;
            }
        }

        // 2.- The variable is not an IV: 
        // 2.1.- it may be an IV from an outer loop => get the proper offset
        current_cs_node = current_cs_node->get_enclosing_cs();
        while(current_cs_node!=NULL && current_cs_node->get_type()!=Loop)
            current_cs_node = current_cs_node->get_enclosing_cs();
        if(current_cs_node != NULL)
        {
            goto check_ivs;
        }
        // 2.2.- it is not an IV from an outer loop => get its values from Range Analysis
        {
            range = cs_node->get_pcfg_node()->get_range(var).shallow_copy();
            ERROR_CONDITION(range.is_null(), 
                            "No range found for non-induction_variable %s involved in loop condition.\n", 
                            var.prettyprint().c_str());
            transform_expression_to_json_expression(
                    cs_node, range, var_to_value_map, var_to_id_map, ordered_vars, last_var_id);
        }

end_get_vars:
        ;

        var_to_value_map[var] = range;
    }

    void transform_expression_to_json_expression(
            ControlStructure* cs_node,
            NBase& expression,
            VarToNodeclMap& var_to_value_map,
            VarToNodeclMap& var_to_id_map,
            ObjectList<NBase>& ordered_vars,
            unsigned int& last_var_id)
    {
        // This may happen when calling recursively (i.e. boundaries of an IV)
        if (expression.is_null())
            return;

        // Gather all variables and compute their corresponding identifier
        NodeclList new_vars;
        NodeclList vars_accesses = Nodecl::Utils::get_all_memory_accesses(expression);
        for (NodeclList::iterator it = vars_accesses.begin(); it != vars_accesses.end(); ++it)
        {
            // Check whether the variable has not been replaced yet
            if (ordered_vars.filter(StructuralCompareBind1(*it)).empty())
            {
                const NBase& var = it->shallow_copy();
                new_vars.append(var);
                ordered_vars.append(var);

                // Get the identifier corresponding to this variable
                std::stringstream id_ss; id_ss << "$" << ++last_var_id;
                var_to_id_map[var] = Nodecl::Text::make(id_ss.str());
            }
        }

        // Replace the original variables with their corresponding $id
        NodeclReplacer nr(var_to_id_map);
        nr.walk(expression);

        // Get the values of the involved variables
        for (NodeclList::iterator it = new_vars.begin(); it != new_vars.end(); ++it)
        {
            get_variable_values(cs_node, *it, var_to_value_map, var_to_id_map, ordered_vars, last_var_id);
        }
    }

    // FIXME This replacement does not take into account that input values of the variables
    // may be different on the left and right hand of the condition (for example, variables within a loop)
    void transform_node_condition_into_json_expr(
            ControlStructure* cs_node, 
            NBase& condition,
            VarToNodeclMap& var_to_value_map,
            ObjectList<NBase>& ordered_vars)
    {
        unsigned int last_var_id = 0;
        VarToNodeclMap var_to_id_map;
        return transform_expression_to_json_expression(
                    cs_node, condition, 
                    var_to_value_map, var_to_id_map, 
                    ordered_vars, last_var_id);
    }

    /*! This class gathers information of the variables in a dependency condition
     * in order to translate it into its JSON form */
    struct ConditionVisitor : public Nodecl::NodeclVisitor<void>
    {
        // *** Class members *** //
        TDG_Edge* _edge;
        int _id;
        VarToNodeclMap _source_var_to_value_map;
        VarToNodeclMap _target_var_to_value_map;
        
        // *** Constructor *** //
        ConditionVisitor(TDG_Edge* edge)
            : _edge(edge), _id(0),
              _source_var_to_value_map(), _target_var_to_value_map()
        {}
        
        VarToNodeclMap get_source_var_to_value_map() const
        {
            return _source_var_to_value_map;
        }

        VarToNodeclMap get_target_var_to_value_map() const
        {
            return _target_var_to_value_map;
        }

        void collect_condition_info(TDG_Node* n, const NBase& cond_expr, bool is_source)
        {
            // 1.- Get all the variables involved in cond_expr
            NodeclList tmp = Nodecl::Utils::get_all_memory_accesses(cond_expr);
            std::queue<NBase, std::deque<NBase> > vars(std::deque<NBase>(tmp.begin(), tmp.end()));

            // 2.- For each variable involved in the condition, gather its values
            Node* pcfg_n = n->get_pcfg_node();
            NodeclSet already_treated;
            while (!vars.empty())
            {
                // 2.1.- Get the current variable
                NBase v = vars.front();
                vars.pop();

                // 2.2.- Compute the value(s) of the variable
                NBase values;

                // 2.2.1.- First check whether this is an induction variable of any loop enclosing the task
                const ControlStList& cs_list = n->get_control_structures();
                for (ControlStList::const_iterator it = cs_list.begin(); it != cs_list.end(); ++it)
                {
                    ControlStructure* cs = it->first;
                    if(cs->get_type() == Loop)
                    {
                        bool is_loop_iv = false;
                        Utils::InductionVarList ivs = cs->get_pcfg_node()->get_induction_variables();
                        for (Utils::InductionVarList::iterator itt = ivs.begin(); itt != ivs.end(); ++itt)
                        {
                            if (Nodecl::Utils::structurally_equal_nodecls((*itt)->get_variable(), v, /*skip_conversions*/true))
                            {
                                is_loop_iv = true;
                                break;
                            }
                        }

                        if (is_loop_iv)
                        {   // The variable is an IV
                            report_default_offset(pcfg_n, cs->get_pcfg_node(), v);
                            values = Nodecl::IntegerLiteral::make(Type::get_int_type(), const_value_get_zero(/*bytes*/ 4, /*signed*/ 1));
                            goto insert_values;
                        }
                    }
                }

                // 2.2.2.- The variable is not an induction variable. Retrieve values from range analysis
                {
                    // 2.2.2.1.-  Make sure we have some value for the variable
                    values = pcfg_n->get_range(v);
                    ERROR_CONDITION(values.is_null(),
                                    "No range computed in node %d for variable '%s', in condition's %s '%s'.\n",
                                    pcfg_n->get_id(), v.prettyprint().c_str(), (is_source ? "LHS" : "RHS"), cond_expr.prettyprint().c_str());

                    // 2.2.2.2.- Store the variable so we do not treat it again (to avoid recursive definitions)
                    if (already_treated.find(v) == already_treated.end())
                        already_treated.insert(v);

                    // 2.2.2.3.- Add to 'vars' all the symbols involved in 'values' that has not yet been treated
                    NodeclSet to_treat;
                    tmp = Nodecl::Utils::get_all_memory_accesses(values);
                    for (NodeclList::iterator itt = tmp.begin(); itt != tmp.end(); ++itt)
                    {
                        NBase var = *itt;
                        if ((already_treated.find(var) == already_treated.end())
                            && (to_treat.find(var) == to_treat.end()))
                        {
                            vars.push(var);
                            to_treat.insert(var);
                        }
                    }
                }

insert_values:
                // 2.5.- Store the reaching definition related with the identifier of the variable defined
                if (is_source)
                    _source_var_to_value_map[v] = values;
                else
                    _target_var_to_value_map[v] = values;
            }
        }

        void unhandled_node(const NBase& n)
        {
            internal_error( "Unhandled node of type '%s' while visiting TDG condition.\n '%s' ",
            ast_print_node_type(n.get_kind()), n.prettyprint().c_str());
        }
        
        void join_list(ObjectList<void>& list)
        {
            if (TDG_DEBUG)
                WARNING_MESSAGE("Called method join_list in ConditionVisitor. This is not yet implemented", 0);
        }

        // The variables on the LHS correspond to the source of the 'edge'
        // and the variables on the RHS correspond to the target of the 'edge'
        void visit(const Nodecl::Equal& n)
        {
            // Recursively call with the LHS and RHS of the condition
            collect_condition_info(_edge->get_source(), n.get_lhs(), /*is_source*/true);
            collect_condition_info(_edge->get_target(), n.get_rhs(), /*is_source*/false);
        }
        
        void visit(const Nodecl::LogicalAnd& n)
        {
            walk(n.get_lhs());
            walk(n.get_rhs());
        }

        void visit(const Nodecl::LogicalOr& n)
        {
            walk(n.get_lhs());
            walk(n.get_rhs());
        }

        void visit(const Nodecl::LowerOrEqualThan& n)
        {
            // Recursively call with the LHS and RHS of the condition
            collect_condition_info(_edge->get_source(), n.get_lhs(), /*is_source*/true);
            collect_condition_info(_edge->get_target(), n.get_rhs(), /*is_source*/false);
        }

        void visit(const Nodecl::GreaterOrEqualThan& n)
        {
            // Recursively call with the LHS and RHS of the condition
            collect_condition_info(_edge->get_source(), n.get_lhs(), /*is_source*/true);
            collect_condition_info(_edge->get_target(), n.get_rhs(), /*is_source*/false);
        }
    };
    
    static void replace_vars_with_ids(
            const VarToNodeclMap& var_to_values_map,
            VarToNodeclMap& var_to_id_map,
            NBase& condition,
            unsigned int& id,
            bool is_source)
    {
        // NOTE: We cannot use NodeclReplacer because the LHS of the comparisons must be replaced with an identifier and
        // the RHS with another (example: x == x  -> $1 == $2)
        std::string condition_str = condition.prettyprint();
        for (VarToNodeclMap::const_iterator it = var_to_values_map.begin(); it != var_to_values_map.end(); ++it)
        {
            std::string var = it->first.prettyprint();
            std::stringstream ss; ss << "$" << id;

            size_t init = condition_str.find(var, 0);
            // Replace the variables with their id
            while (init != std::string::npos)
            {
                // check whether this occurrence belongs to the LHS or the RHS of the condition
                size_t tmp_or = condition_str.find("||", init);
                size_t tmp_and = condition_str.find("&&", init);
                size_t min_logical_op = (tmp_or < tmp_and ? tmp_or : tmp_and);
                size_t tmp_eq = condition_str.find("==", init);
                size_t tmp_l_eq = condition_str.find("<=", init);
                size_t tmp_g_eq = condition_str.find(">=", init);
                size_t min_comp_op = (tmp_eq < tmp_l_eq
                                            ? (tmp_eq < tmp_g_eq
                                                    ? tmp_eq
                                                    : tmp_g_eq)
                                            : (tmp_l_eq < tmp_g_eq
                                                    ? tmp_l_eq
                                                    : tmp_g_eq)
                                     );
                if ((is_source && (min_comp_op < min_logical_op))
                        || (!is_source && ((min_logical_op < min_comp_op) || (min_logical_op == std::string::npos))))
                {   // It is a LHS occurrence => replace
                    // Only replace if the substring found is not a part of another variable
                    // (example: var_name and var_name_longer)
                    std::string c_after_var_name = condition_str.substr(init+var.size(), 1);
                    if (c_after_var_name == "|" || c_after_var_name == "&"              // operation
                            || c_after_var_name == " " || c_after_var_name == ")"       // other characters
                            || condition_str.size() == init+var.size())                 // end of string
                    {
                        condition_str.replace(init, var.size(), ss.str());
                    }
                }
                // prepare the next iteration
                init = condition_str.find(var, min_logical_op);
            }

            // Identifier must always be incremented because
            // it defined the order in which the variables will be printed in the JSON
            ++id;
        }

        condition = Nodecl::Text::make(condition_str);
    }

    // This method returns a string corresponding to the prettyprinted version of a nodecl
    // where each symbol occurrence is replaced by a $id
    // Example:
    //     The expression :         'i == j'
    //     Will return the string:  '$1 == $2'
    void transform_edge_condition_into_json_expr(
            TDG_Edge* edge,
            NBase& condition,
            VarToNodeclMap& source_var_to_values_map,
            VarToNodeclMap& target_var_to_values_map,
            VarToNodeclMap& source_var_to_id_map,
            VarToNodeclMap& target_var_to_id_map)
    {
        ConditionVisitor cv(edge);
        // Traverse the condition to store information necessary for the transformation
        cv.walk(condition);

        // Set the output parameters
        source_var_to_values_map = cv.get_source_var_to_value_map();
        target_var_to_values_map = cv.get_target_var_to_value_map();

        // LHS and RHS variables must be treated separately
        unsigned int id = 1;
        replace_vars_with_ids(source_var_to_values_map, source_var_to_id_map, condition, id, /*is_source*/true);
        replace_vars_with_ids(target_var_to_values_map, target_var_to_id_map, condition, id, /*is_source*/false);
    }
}

    // ******************************************************************* //
    // ************ Task Dependency Graph Control Structures ************* //
    
    ControlStructure::ControlStructure(int cs_id, ControlStructureType type, 
                                       const NBase& condition, Node* pcfg_node)
        : _id(cs_id), _type(type), _condition(condition), _pcfg_node(pcfg_node), _enclosing(NULL)
    {}

    int ControlStructure::get_id() const
    {
        return _id;
    }

    ControlStructureType ControlStructure::get_type() const
    {
        return _type;
    }

    std::string ControlStructure::get_type_as_string() const
    {
        std::string result;
        switch(_type)
        {
            case Implicit:  result = "Implicit"; break;
            case Loop:      result = "Loop";     break;
            case IfElse:    result = "IfElse";   break;
            default:        result = "Blank";
        };
        return result;
    }

    NBase ControlStructure::get_condition() const
    {
        return _condition;
    }

    Node* ControlStructure::get_pcfg_node() const
    {
        return _pcfg_node;
    }

    ControlStructure* ControlStructure::get_enclosing_cs() const
    {
        return _enclosing;
    }

    void ControlStructure::set_enclosing_cs(ControlStructure* cs)
    {
        _enclosing = cs;
    }

    // ************ Task Dependency Graph Control Structures ************* //
    // ******************************************************************* //
    
 
    // ******************************************************************* //
    // ************** Task Dependency Graph Edges and Nodes ************** //
    
    TDG_Node::TDG_Node(Node* n, TDGNodeType type, Node* parent, Node* init_tp)
        : _id(++node_id), _pcfg_node(n), _type(type),
          _parent(parent), _entries(), _exits(),
          _control_structures(), _init_taskpart(init_tp)
    {}

    unsigned int TDG_Node::get_id() const
    {
        return _id;
    }

    Node* TDG_Node::get_pcfg_node() const
    {
        return _pcfg_node;
    }

    void TDG_Node::add_control_structure(ControlStructure* cs, std::string taken_branch)
    {
        _control_structures.push_back(std::pair<ControlStructure*, std::string>(cs, taken_branch));
    }

    ControlStList TDG_Node::get_control_structures() const
    {
        return _control_structures;
    }

    TDG_Edge::TDG_Edge(TDG_Node* source, TDG_Node* target, SyncKind kind, const NBase& condition)
        : _source(source), _target(target), _kind(kind), _condition(condition)
    {}
    
    TDG_Node* TDG_Edge::get_source() const
    {
        return _source;
    }
    
    TDG_Node* TDG_Edge::get_target() const
    {
        return _target;
    }
    
    // ************** Task Dependency Graph Edges and Nodes ************** //
    // ******************************************************************* //

    
    // ******************************************************************* //
    // ********************** Task Dependency Graph ********************** //

    TaskDependencyGraph::TaskDependencyGraph(
            ExtensibleGraph* pcfg,
            std::string json_name,
            bool taskparts_enabled,
            bool psocrates)
        : _id(UINT_MAX), _pcfg(pcfg), _json_name(json_name),
          _tdg_nodes(), _syms(), _pcfg_to_cs_map(),
          _taskparts_enabled(taskparts_enabled), _psocrates(psocrates)
    {
        Node* pcfg_node = _pcfg->get_graph();
        node_id = 0;

        if (_psocrates)
        {
            // 1.- Get the identifier of the graph (from the call to GOMP_set_tdg_id inserted previously)
            //     Only graphs with tasks are accepted, and those graphs must have the following structures
            Node* pcfg_function_code = pcfg_node->get_graph_entry_node()->get_children()[0];
            Node* pcfg_ctx = pcfg_function_code->get_graph_entry_node()->get_children()[0];
            Node* pcfg_first_node = pcfg_ctx->get_graph_entry_node()->get_children()[0];
            ERROR_CONDITION(!pcfg_first_node->is_function_call_graph_node(),
                            "The first node of a PCFG containing tasks must be a Function Call. Instead, we found a %s.\n",
                            pcfg_first_node->is_graph_node() ? pcfg_first_node->get_graph_type_as_string().c_str()
                                                             : pcfg_first_node->get_type_as_string().c_str());
            Node* func_call_node = pcfg_first_node->get_graph_entry_node()->get_children()[0];
            Nodecl::FunctionCall func_call_nodecl = func_call_node->get_statements()[0].as<Nodecl::FunctionCall>();
            Symbol func_call_sym = func_call_nodecl.get_called().get_symbol();
            ERROR_CONDITION(func_call_sym.get_name() != "GOMP_set_tdg_id",
                            "The first node of a PCFG containing tasks must be a call to GOMP_set_tdg_id", 0);
            Nodecl::List args = func_call_nodecl.get_arguments().as<Nodecl::List>();
            _id = const_value_cast_to_unsigned_int(args[0].as<Nodecl::IntegerLiteral>().get_constant());
        }

        // 2.- Build the nodes from the TDG
        create_tdg_task_nodes_from_pcfg(pcfg_node);
        ExtensibleGraph::clear_visits(pcfg_node);
        create_tdg_nodes_from_pcfg(pcfg_node);
        ExtensibleGraph::clear_visits(pcfg_node);
        if (_taskparts_enabled)
        {
            create_tdg_nodes_from_taskparts(pcfg_node);
            ExtensibleGraph::clear_visits(pcfg_node);
        }

        // 3.- Compute the data structures that will wrap the nodes
        set_tdg_nodes_control_structures();

        // 4.- Connect the tasks in the TDG
        connect_tdg_nodes_from_pcfg(pcfg_node);
    }

    std::string TaskDependencyGraph::get_name() const
    {
        return _json_name;
    }

    void TaskDependencyGraph::connect_tdg_nodes(
            TDG_Node* parent, TDG_Node* child,
            SyncKind sync_type, const NBase& condition)
    {
        TDG_Edge* edge = new TDG_Edge(parent, child, sync_type, condition);
        parent->_exits.insert(edge);
        child->_entries.insert(edge);
    }

    ExtensibleGraph* TaskDependencyGraph::get_pcfg() const
    {
        return _pcfg;
    }

    unsigned int TaskDependencyGraph::get_id() const
    {
        return _id;
    }

    TDG_Node* TaskDependencyGraph::find_tdg_node_from_pcfg_node(Node* n)
    {
        TDG_Node* result = NULL;
        for (TDG_Node_map::iterator it = _tdg_nodes.begin(); it != _tdg_nodes.end(); ++it)
        {
            if (it->second->_pcfg_node == n)
            {
                result = it->second;
                break;
            }
        }
        ERROR_CONDITION(result==NULL,
                        "PCFG node with id '%d' not found in the TDG",
                        n->get_id());
        return result;
    }

    void TaskDependencyGraph::create_tdg_node(
            Node* current,
            int tdg_node_id,
            TDGNodeType type,
            Node* init_tp)
    {
        // Avoid duplicating identifiers
        // This may happen when multiple directives with a BarrierAtEnd are consecutive
        while (_tdg_nodes.find(tdg_node_id) != _tdg_nodes.end())
            ++tdg_node_id;

        // Get the parent task
        Node* parent = current->get_outer_node();
        while (parent != _pcfg->get_graph()
            && !(parent->is_omp_task_node() || parent->is_omp_async_target_node()))
            parent = parent->get_outer_node();

        // Create the TDG node
        TDG_Node* tdg_current = new TDG_Node(current, type, parent, init_tp);

        // Insert the node in the TDG
        _tdg_nodes.insert(std::pair<unsigned int, TDG_Node*>(tdg_node_id, tdg_current));
    }

    void TaskDependencyGraph::create_tdg_task_nodes_from_pcfg(Node* current)
    {
        // 1.- Base case: the node has been visited
        if (current->is_visited())
            return;
        current->set_visited(true);

        // 2.- Case 1: the node is a graph -> call recursively with inner nodes
        if (current->is_graph_node())
            create_tdg_task_nodes_from_pcfg(current->get_graph_entry_node());
        //
        // 3.- Case 2: the node is a task|target|taskwait|barrier -> create the TDG node
        // The identifier needs to be ordered as the node appears in the source code
        // due to boxer requirements (to avoid backward dependencies during expansion)
        int tdg_node_id;
        TDGNodeType type = Unknown;
        // Create the TDG node from the PCFG node
        if (current->is_omp_task_node())
        {
            tdg_node_id = current->get_graph_related_ast().get_line();
            type = Task;
        }
        if (type != Unknown)
        {
            create_tdg_node(current, tdg_node_id, type);
        }

        // 4.- Iterate over the children
        Node_list children = current->get_children();
        for (Node_list::iterator it = children.begin(); it != children.end(); ++it)
            create_tdg_task_nodes_from_pcfg(*it);
    }

    void TaskDependencyGraph::create_tdg_nodes_from_pcfg(Node* current)
    {
        // 1.- Base case: the node has been visited
        if (current->is_visited())
            return;
        current->set_visited(true);

        // 2.- Case 1: the node is a graph -> call recursively with inner nodes
        if (current->is_graph_node())
            create_tdg_nodes_from_pcfg(current->get_graph_entry_node());

        // 3.- Case 2: the node is a task|target|taskwait|barrier -> create the TDG node
        // The identifier needs to be ordered as the node appears in the source code
        // due to boxer requirements (to avoid backward dependencies during expansion)
        int tdg_node_id;
        TDGNodeType type = Unknown;
        // Create the TDG node from the PCFG node
        if (current->is_omp_task_node())
        {   // Nothing to do: these  nodes are treated separately
//             tdg_node_id = current->get_graph_related_ast().get_line();
//             type = Task;
        }
        else if (current->is_omp_async_target_node())
        {
            tdg_node_id = current->get_graph_related_ast().get_line();
            type = Target;
        }
        else if (current->is_omp_sync_target_node())
        {
            internal_error("Psocrates project does not support undeferred tasks.\n", 0);
        }
        else if (current->is_omp_taskwait_node())
        {
            tdg_node_id = current->get_statements()[0].get_line();
            type = Taskwait;
        }
        else if (current->is_omp_barrier_graph_node())
        {   // Note that the Graph Barrier Node need no traversal
            // If the barrier is implicit in a parallel or any other construct,
            // then the identifier must not be the line, but the end of the associated statement, instead
            const NBase& barrier_ast = current->get_graph_related_ast();
            if (barrier_ast.is<Nodecl::OpenMP::BarrierAtEnd>())
            {   // The barrier is implicit
                const Nodecl::List& environ = barrier_ast.get_parent().as<Nodecl::List>();
                // iterate over the environ members until it is not a list
                NBase directive = environ.get_parent();
                while (directive.is<Nodecl::List>()) {
                    directive = directive.get_parent();
                }
                if (!directive.is<Nodecl::OpenMP::Parallel>()
                        && !directive.is<Nodecl::OpenMP::For>()
                        && !directive.is<Nodecl::OpenMP::Sections>()
                        && !directive.is<Nodecl::OpenMP::Single>())
                {
                    internal_error("Unexpected node kind '%s' with an implicit barrier at end.\n",
                                    ast_print_node_type(directive.get_kind()));
                }
                else
                {
                    NBase parent = directive.get_parent();
                    if (parent.is_null())
                    {   // The directive is the last in the list, we report the line of the directive
                        if (TDG_DEBUG)
                        {
                            WARNING_MESSAGE("Found barrier node implicit to directive in line %d. "
                                            "We are not able to compute where the directive ends.\n"
                                            "Reporting barrier line as the directive line.\n", directive.get_line());
                        }
                        tdg_node_id = barrier_ast.get_line();
                    }
                    else
                    {
                        // Artificial statements have a default locus (line is always 0)
                        // Navigate over the statements until we find one that comes from the source code
                        while (!parent.is_null()
                            && parent.get_line() == 0)
                        {
                            parent = parent.get_parent();
                        }
                        tdg_node_id = parent.get_line();
                    }
                }
            }
            else
            {   // This is a BarrierFull
                tdg_node_id = barrier_ast.get_line();
            }
            type = Barrier;
        }

        if (type != Unknown)
        {
            create_tdg_node(current, tdg_node_id, type);
        }

        // 4.- Iterate over the children
        Node_list children = current->get_children();
        for (Node_list::iterator it = children.begin(); it != children.end(); ++it)
            create_tdg_nodes_from_pcfg(*it);
    }

namespace {
    void get_end_taskparts_rec(Node* n, std::list<Node*>& end_taskparts)
    {
        // 1.- Base cases
        // 1.1.- The node has already been visited: skip it
        if (n->is_visited_aux())
            return;
        n->set_visited_aux(true);
        // 1.2.- The node is a graph: traverse the inner nodes
        if (n->is_graph_node())
            get_end_taskparts_rec(n->get_graph_entry_node(), end_taskparts);

        // 2.- Searched case
        if (n->is_function_call_node())
        {
            Nodecl::FunctionCall f = n->get_statements()[0].as<Nodecl::FunctionCall>();
            if (f.get_called().get_symbol().get_name() == "GOMP_end_taskpart")
            {
                end_taskparts.push_back(n);
                return;
            }
        }

        // 3.- Keep iterating
        // Since we do not traverse the PCFG from the beginning,
        // it may happen that we arrive in an exit node
        // without having visited its graph node
        ObjectList<Node*> children;
        if ((n->is_graph_node() && n->get_graph_exit_node()->is_visited_aux())
                || (!n->is_graph_node() && !n->is_exit_node()))
            children = n->get_children();
        else if (n->is_exit_node())
            children = n->get_outer_node()->get_children();
        for (ObjectList<Node*>::const_iterator it = children.begin();
             it != children.end(); ++it)
        {
            get_end_taskparts_rec(
                /*current*/ *it,
                /*result*/ end_taskparts);
        }
    }

    void get_end_taskparts(Node* n, std::list<Node*>& end_taskparts)
    {
        get_end_taskparts_rec(n, end_taskparts);
        ExtensibleGraph::clear_visits_aux(n);
    }
}

    void TaskDependencyGraph::create_tdg_nodes_from_taskparts(Node* current)
    {
        // 1.- Base case: the node has been visited
        if (current->is_visited())
            return;
        current->set_visited(true);

        // 2.- Case 1: the node is a graph -> call recursively with inner nodes
        if (current->is_graph_node())
            create_tdg_nodes_from_taskparts(current->get_graph_entry_node());

        // 3.- Case 2: the node is GOMP_init_taskpart
        //     -> create the TDG node for each possible taskpart
        if (current->is_function_call_node())
        {   // Check a possible task parts
            Nodecl::FunctionCall f = current->get_statements()[0].as<Nodecl::FunctionCall>();
            if (f.get_called().get_symbol().get_name() == "GOMP_init_taskpart")
            {
                // Create a TDG node for each reachable GOMP_end_taskpart
                std::list<Node*> end_taskparts;
                get_end_taskparts(current, end_taskparts);
                for (std::list<Node*>::iterator it = end_taskparts.begin();
                     it != end_taskparts.end(); ++it)
                {
                    // Artificial statements have a default locus (line is always 0)
                    // Navigate over the statements until we find one that comes from the source code
                    NBase parent = (*it)->get_statements()[0].get_parent();
                    while (!parent.is_null()
                        && parent.get_line() == 0)
                    {
                        parent = parent.get_parent();
                    }

                    create_tdg_node(*it, /*TDG node id*/ parent.get_line(),
                                    /*type*/ Taskpart, /*init_tp*/current);
                }
            }
        }

        // 4.- Iterate over the children
        Node_list children = current->get_children();
        for (Node_list::iterator it = children.begin(); it != children.end(); ++it)
            create_tdg_nodes_from_taskparts(*it);
    }

namespace {
    enum enclosing_cs_type {
        __undefined = 0,
        __init = 1,
        __end = 2,
        __none = 3
    };

    enclosing_cs_type get_enclosing_cs_type(Node* init_cs, Node* end_cs)
    {
        enclosing_cs_type enclosing_cs = __undefined;

        while (enclosing_cs == __undefined)
        {
            // Prepare the iteration by looking for the corresponding loop control structures
            while (init_cs != NULL && !init_cs->is_loop_node())
                init_cs = ExtensibleGraph::get_enclosing_control_structure(init_cs);
            while (end_cs != NULL && !end_cs->is_loop_node())
                end_cs = ExtensibleGraph::get_enclosing_control_structure(end_cs);

            // Check whether we have found a suitable case
            if ((init_cs == NULL && end_cs == NULL)
                || (init_cs == end_cs))
            {
                enclosing_cs = __none;
            }
            if (init_cs == NULL
                || (end_cs != NULL && ExtensibleGraph::node_contains_node(init_cs, end_cs)))
            {
                enclosing_cs = __init;
            }
            else if (end_cs == NULL
                || ( init_cs != NULL && ExtensibleGraph::node_contains_node(end_cs, init_cs)))
            {
                enclosing_cs = __end;
            }
        }

        return enclosing_cs;
    }
}

    // This method creates the control structures for simple nodes and
    // for those taskparts where the initial and end points are in the same control structure
    // and the initial point is found before the end point in a sequential order of the program
    void TaskDependencyGraph::create_control_structure_rec(
            CS_case cs_case,
            Node* init,
            Node* n,
            Node* control_structure,
            TDG_Node* tdg_node,
            ControlStructure* last_cs)
    {
        Node* init_cs = (init == NULL
                                ? NULL
                                : ExtensibleGraph::get_enclosing_control_structure(init));

        while (control_structure != NULL)
        {
            // 1.- Get control structure type and condition
            ControlStructureType cs_t;
            NBase condition;
            std::string taken_branch;
            if (control_structure->is_loop_node())
            {
                // get the type of the Control Structure
                cs_t = Loop;

                Node* next_control_structure = ExtensibleGraph::get_enclosing_control_structure(control_structure);
                switch (cs_case)
                {
                    case same_cs_init_before_end:
                    {
//                         std::cerr << "  --> same_cs_init_before_end : "
//                                   << (init==NULL?0:init->get_id()) << " -> " << n->get_id() << std::endl;
                        // Get the condition of the loop
                        Node* cond = control_structure->get_condition_node();
                        assert(cond != NULL);
                        NodeclList stmts = cond->get_statements();
                        assert(stmts.size() == 1);
                        condition = stmts[0];
                        break;
                    }
                    case same_cs_end_before_init:
                    {
//                         std::cerr << "  --> same_cs_end_before_init : "
//                                   << (init==NULL?0:init->get_id()) << " -> " << n->get_id() << std::endl;
                        const Utils::InductionVarList& ivs = control_structure->get_induction_variables();
                        ERROR_CONDITION(ivs.size() != 1,
                                        "Psocrates does not support loops with more than one Induction Variable.\n",
                                        0);
                        const NBase iv = ivs[0]->get_variable();
                        condition =
                                Nodecl::LogicalAnd::make(
                                    Nodecl::GreaterThan::make(
                                        iv.shallow_copy(),
                                        ivs[0]->get_lb().begin()->shallow_copy(),
                                        iv.get_type()),
                                    Nodecl::LowerOrEqualThan::make(
                                        iv.shallow_copy(),
                                        ivs[0]->get_ub().begin()->shallow_copy(),
                                        iv.get_type()),
                                    iv.get_type()
                                );

                        // End being before init only matters for the most inner loop
                        cs_case = same_cs_init_before_end;
                        break;
                    }
                    case init_encloses_end_init_before_end:
                    {
//                         std::cerr << "  --> init_encloses_end_init_before_end : "
//                                   << (init==NULL?0:init->get_id()) << " -> " << n->get_id() << std::endl;
                        const Utils::InductionVarList& ivs = control_structure->get_induction_variables();
                        ERROR_CONDITION(ivs.size() != 1,
                                        "Psocrates does not support loops with more than one Induction Variable.\n",
                                        0);
                        condition =
                                Nodecl::Equal::make(
                                    ivs[0]->get_variable().shallow_copy(),
                                    ivs[0]->get_lb().begin()->shallow_copy(),
                                    ivs[0]->get_variable().get_type()
                                );

                        // Check the relation between the init control structure and the next end control structure
                        if (next_control_structure != NULL)
                        {
                            Node* next_cs = ExtensibleGraph::get_enclosing_control_structure(next_control_structure);
                            enclosing_cs_type enclosing_cs = get_enclosing_cs_type(init_cs, next_cs);
                            if (enclosing_cs == __none)
                                cs_case = same_cs_init_before_end;
                        }
                        break;
                    }
                    case init_encloses_end_end_before_init:
                    {
//                         std::cerr << "  --> init_encloses_end_end_before_init : "
//                                   << (init==NULL?0:init->get_id()) << " -> " << control_structure->get_id() << std::endl;
                        const Utils::InductionVarList& ivs = control_structure->get_induction_variables();
                        ERROR_CONDITION(ivs.size() != 1,
                                        "Psocrates does not support loops with more than one Induction Variable.\n",
                                        0);
                        condition =
                                Nodecl::Equal::make(
                                    ivs[0]->get_variable().shallow_copy(),
                                    ivs[0]->get_lb().begin()->shallow_copy(),
                                    ivs[0]->get_variable().get_type()
                                );

                        // Check the relation between the init control structure and the next end control structure
                        if (next_control_structure != NULL)
                        {
                            Node* next_cs = ExtensibleGraph::get_enclosing_control_structure(next_control_structure);
                            enclosing_cs_type enclosing_cs = get_enclosing_cs_type(init_cs, next_cs);
                            if (enclosing_cs == __none)
                                cs_case = same_cs_end_before_init;
                        }
                        break;
                    }
                    case end_encloses_init_init_before_end:
                    {
//                         std::cerr << "  --> end_encloses_init_init_before_end : "
//                                   << (init==NULL?0:init->get_id()) << " -> " << n->get_id() << std::endl;
                        const Utils::InductionVarList& ivs = control_structure->get_induction_variables();
                        ERROR_CONDITION(ivs.size() != 1,
                                        "Psocrates does not support loops with more than one Induction Variable.\n",
                                        0);
                        condition =
                                Nodecl::Equal::make(
                                    ivs[0]->get_variable().shallow_copy(),
                                    ivs[0]->get_ub().begin()->shallow_copy(),
                                    ivs[0]->get_variable().get_type()
                                );

                        // Check the relation between the init control structure and the next end control structure
                        if (next_control_structure != NULL)
                        {
                            Node* next_cs = ExtensibleGraph::get_enclosing_control_structure(next_control_structure);
                            enclosing_cs_type enclosing_cs = get_enclosing_cs_type(init_cs, next_cs);
                            if (enclosing_cs == __none)
                                cs_case = same_cs_init_before_end;
                        }
                        break;
                    }
                    case end_encloses_init_end_before_init:
                    {
//                         std::cerr << "  --> end_encloses_init_end_before_init : "
//                                   << (init==NULL?0:init->get_id()) << " -> " << n->get_id() << std::endl;
                        const Utils::InductionVarList& ivs = control_structure->get_induction_variables();
                        ERROR_CONDITION(ivs.size() != 1,
                                        "Psocrates does not support loops with more than one Induction Variable.\n",
                                        0);
                        condition =
                                Nodecl::Equal::make(
                                    ivs[0]->get_variable().shallow_copy(),
                                    ivs[0]->get_ub().begin()->shallow_copy(),
                                    ivs[0]->get_variable().get_type()
                                );

                        // Check the relation between the init control structure and the next end control structure
                        if (next_control_structure != NULL)
                        {
                            Node* next_cs = ExtensibleGraph::get_enclosing_control_structure(next_control_structure);
                            enclosing_cs_type enclosing_cs = get_enclosing_cs_type(init_cs, next_cs);
                            if (enclosing_cs == __none)
                                cs_case = same_cs_end_before_init;
                        }
                        break;
                    }
                    default:
                    {
                        internal_error("Unreachable code.\n", 0);
                    }
                }
            }
            else if (control_structure->is_ifelse_statement())
            {
                // get the type of the Control Structure
                cs_t = IfElse;

                // Check whether the statement is in the TRUE or the FALSE branch of the condition
                if (cs_case==end_encloses_init_init_before_end
                        || cs_case==end_encloses_init_end_before_init)
                {
                    condition = get_ifelse_condition_and_path(
                            control_structure,
                            init,
                            taken_branch);
                }
                else
                {
                    condition = get_ifelse_condition_and_path(
                            control_structure,
                            n,
                            taken_branch);
                }
            }
            else
            {
                internal_error("Unexpected node type %s when printing condition to TDG.\n"
                                "Expected Loop or IfElse.\n",
                                control_structure->get_type_as_string().c_str());
            }

            // 2.- Store the symbols involved in the condition in the list of used symbols in the graph
            ERROR_CONDITION(condition.is_null(),
                            "No condition has been computed for task %d in control structure %d.\n",
                            n->get_id(), control_structure->get_id());
            store_condition_list_of_symbols(condition, n->get_reaching_definitions_in());

            // 3.- Create a new control structure, if it did not exist yet
            ControlStructure* cs = NULL;
            std::pair<PCFG_to_CS::iterator, PCFG_to_CS::iterator> css =
                    _pcfg_to_cs_map.equal_range(std::pair<Node*, Node*>(control_structure, NULL));
            for (PCFG_to_CS::iterator it = css.first; it != css.second && cs == NULL; ++it)
            {
                const NBase& cond = it->second->get_condition();
                if (Nodecl::Utils::structurally_equal_nodecls(cond, condition, /*skip conversions*/true))
                {
                    cs = it->second;
                }
            }
            if (cs == NULL)
            {   // The control structure did not exist yet
                cs = new ControlStructure(++control_id, cs_t, condition, control_structure);
                _pcfg_to_cs_map.insert(std::pair<std::pair<Node*, Node*>, ControlStructure*>
                        (std::pair<Node*, Node*>(control_structure, NULL), cs));
            }

            last_cs->set_enclosing_cs(cs);
            tdg_node->add_control_structure(cs, taken_branch);

            // Prepare next iteration
            last_cs = cs;
            control_structure = ExtensibleGraph::get_enclosing_control_structure(control_structure);
        }
    }

    void TaskDependencyGraph::create_control_structures(
            TDG_Node* tdg_node,
            ControlStructure* last_cs)
    {
        // Case 1: the node is a taskpart => we must consider both the initial and the end points
        //                                   and the control structures involved with both nodes
        if (_taskparts_enabled && tdg_node->_type == Taskpart)
        {
            // The condition of the control structure depends on
            // the execution path differences between its initial node and its end node
            // Thus, the values of the variables to fulfill the condition will depend the next cases:
            //   * A. init is outside a loop and end is inside => loop lower bound
            //   * B. init is inside a loop and end is outside => loop upper bound + stride
            //   * C. init and end are inside a loop => same as for a regular node (lb-stride, ub+stride, stride)
            //   * D. init and end are outside any loop => there is no control structure

            Node* init = tdg_node->_init_taskpart;
            Node* end = tdg_node->_pcfg_node;
            Node* init_cs;
            Node* end_cs;

            // 1.- Check whether is the init or the end that is in a most outer level
            // regarding loop control structures
            init_cs = ExtensibleGraph::get_enclosing_control_structure(init);
            end_cs = ExtensibleGraph::get_enclosing_control_structure(end);
            enclosing_cs_type enclosing_cs = get_enclosing_cs_type(init_cs, end_cs);

            // 2.- Create the control structures involved in the existence of the taskpart
            init_cs = ExtensibleGraph::get_enclosing_control_structure(init);
            end_cs = ExtensibleGraph::get_enclosing_control_structure(end);
            if (enclosing_cs == __none)
            {
                if (ExtensibleGraph::node_is_ancestor_of_node(init, end))
                {
                    create_control_structure_rec(
                            same_cs_init_before_end, init, end, end_cs, tdg_node, last_cs);
                }
                else
                {
                    create_control_structure_rec(
                            same_cs_end_before_init, init, end, end_cs, tdg_node, last_cs);
                }
            }
            else if (enclosing_cs == __init)
            {
                if (ExtensibleGraph::node_is_ancestor_of_node(init, end))
                {
                    create_control_structure_rec(
                        init_encloses_end_init_before_end, init, end, end_cs, tdg_node, last_cs);
                }
                else
                {
                    create_control_structure_rec(
                        init_encloses_end_end_before_init, init, end, end_cs, tdg_node, last_cs);
                }
            }
            else if (enclosing_cs == __end)
            {
                if (ExtensibleGraph::node_is_ancestor_of_node(init, end))
                {
                    create_control_structure_rec(
                        end_encloses_init_init_before_end, init, end, init_cs, tdg_node, last_cs);
                }
                else
                {
                    create_control_structure_rec(
                        end_encloses_init_end_before_init, init, end, init_cs, tdg_node, last_cs);
                }
            }
            else
            {
                internal_error("Unreachable code.\n", 0);
            }
        }
        // Case 2: the node is not a taskpart => just consider the node and its control structures
        else
        {
            Node* n = tdg_node->_pcfg_node;
            Node* control_structure = ExtensibleGraph::get_enclosing_control_structure(n);

            create_control_structure_rec(
                    same_cs_init_before_end, NULL, n, control_structure, tdg_node, last_cs);
        }
    }

    void TaskDependencyGraph::set_tdg_nodes_control_structures()
    {
        for (TDG_Node_map::iterator it = _tdg_nodes.begin(); it != _tdg_nodes.end(); ++it)
        {
            TDG_Node* tdg_node = it->second;
            Node* node = tdg_node->_pcfg_node;
            Node* init_taskpart = tdg_node->_init_taskpart;
            ControlStructure* last_cs = NULL;

            // 1.- Add the implicit control structure:
            //     this is necessary to set the values of the variables reaching a task
            {
                ControlStructure* cs = new ControlStructure(++control_id, Implicit, NBase::null(), NULL);
                std::string taken_branch;
                _pcfg_to_cs_map.insert(std::pair<std::pair<Node*, Node*>, ControlStructure*>
                        (std::pair<Node*, Node*>(node, init_taskpart), cs));
                tdg_node->add_control_structure(cs, taken_branch);
                last_cs = cs;
            }

            // 2.- Add the real control structures
            create_control_structures(tdg_node, last_cs);
        }
    }

    void TaskDependencyGraph::store_condition_list_of_symbols(const NBase& condition, const NodeclMap& reach_defs)
    {
        NodeclSet already_treated;
        NodeclList tmp = Nodecl::Utils::get_all_memory_accesses(condition);
        std::queue<NBase, std::deque<NBase> > vars(std::deque<NBase>(tmp.begin(), tmp.end()));
        while (!vars.empty())
        {
            NBase n = vars.front();         
            vars.pop();
            already_treated.insert(n);
            _syms.insert(std::pair<NBase, unsigned int>(n, 0));

            if (n.is_constant())
                continue;

            // Add all the variables found in the reaching definitions of the current variable
            ERROR_CONDITION(reach_defs.find(n) == reach_defs.end(), 
                            "No reaching definition found for variable '%s' while gathering all necessary symbols.\n", 
                            n.prettyprint().c_str());
            
            std::pair<NodeclMap::const_iterator, NodeclMap::const_iterator> reach_defs_map = reach_defs.equal_range(n);
            NodeclSet to_treat;
            for(NodeclMap::const_iterator it = reach_defs_map.first; it != reach_defs_map.second; ++it)
            {
                tmp = Nodecl::Utils::get_all_memory_accesses(it->second.first);
                for(NodeclList::iterator itt = tmp.begin(); itt != tmp.end(); ++itt)
                {
                    NBase var = *itt;
                    if ((already_treated.find(var) == already_treated.end()) && 
                        (to_treat.find(var) == to_treat.end()))
                    {
                        to_treat.insert(var);
                        vars.push(var);
                    }
                }
            }
        }
    }

    // Traverse the PCFG forward until:
    // - a barrier is found
    // - a taskwait in the same task region is found
    // - the current task region is exited
    void TaskDependencyGraph::connect_tasks_to_previous_synchronization(Node* sync)
    {
        // Start traversing the children of the synchronization node
        std::queue<Node*> worklist;
        ObjectList<Node*> children = sync->get_children();
        for (ObjectList<Node*>::iterator it = children.begin(); it != children.end(); ++it)
            worklist.push(*it);

        std::set<Node*> visited;
        while (!worklist.empty())
        {
            Node* n = worklist.front();
            worklist.pop();

            // Base cases:
            // - the node has already been visited
            if (visited.find(n) != visited.end())
                continue;
            // - we have found a taskwait or a barrier node
            if (n->is_omp_taskwait_node() || n->is_omp_barrier_graph_node())
                continue;

            // Treat the current node: if it is a task, then it has to be synchronized here!
            visited.insert(n);
            if (n->is_omp_task_node())
            {
                TDG_Node* tdg_sync = find_tdg_node_from_pcfg_node(sync);
                TDG_Node* tdg_child_task = find_tdg_node_from_pcfg_node(n);
                connect_tdg_nodes(tdg_sync, tdg_child_task, __Static,
                                  /*condition*/ Nodecl::NodeclBase::null());
            }

            // Prepare following iterations
            if (n->is_graph_node())
            {
                if (!n->is_omp_task_node())
                    worklist.push(n->get_graph_entry_node());
            }
            else if (n->is_exit_node())
            {
                children = n->get_outer_node()->get_children();
                for (ObjectList<Node*>::const_iterator it = children.begin();
                     it != children.end(); ++it)
                    worklist.push(*it);
            }
            else
            {
                children = n->get_children();
                for (ObjectList<Node*>::const_iterator it = children.begin();
                     it != children.end(); ++it)
                    worklist.push(*it);
            }
        }
    }

    void TaskDependencyGraph::connect_tdg_nodes_from_pcfg(Node* n)
    {
        connect_dependent_nodes(n);
        ExtensibleGraph::clear_visits(n);
        if (_taskparts_enabled)
            connect_taskparts();
    }

    void TaskDependencyGraph::connect_dependent_nodes(Node* n)
    {
        if (n->is_visited())
            return;

        n->set_visited(true);

        if (n->is_omp_task_node()
            || n->is_omp_async_target_node())
        {
            // Connect all tasks synchronized here with the new Taskwait/Barrier TDG_Node
            TDG_Node* tdg_sync = find_tdg_node_from_pcfg_node(n);
            const Edge_list& sync_exits = n->get_exit_edges();
            for (Edge_list::const_iterator it = sync_exits.begin(); it != sync_exits.end(); ++it)
            {
                Node* child = (*it)->get_target();
                if (child->is_omp_task_node()
                    || child->is_omp_async_target_node()
                    || child->is_omp_taskwait_node()
                    || child->is_omp_barrier_graph_node())
                {
                    TDG_Node* tdg_child_task = find_tdg_node_from_pcfg_node(child);
                    const NBase& cond = (*it)->get_condition();
                    connect_tdg_nodes(tdg_sync, tdg_child_task, (*it)->get_sync_kind(), cond);
                    store_condition_list_of_symbols(cond, n->get_reaching_definitions_out());
                }
            }
        }
        else if (n->is_omp_taskwait_node() || n->is_omp_barrier_graph_node())
        {
            TDG_Node* tdg_sync = find_tdg_node_from_pcfg_node(n);
            // Look for the real node to whom the current synchronization is connected
            Edge_list sync_exits = n->get_exit_edges();
            while (sync_exits.size()==1 &&
                    (sync_exits[0]->get_target()->is_omp_flush_node()
                        || sync_exits[0]->get_target()->is_exit_node()))
            {
                Node* child = sync_exits[0]->get_target();
                if (child->is_exit_node())
                    sync_exits = child->get_outer_node()->get_exit_edges();
                else
                    sync_exits = child->get_exit_edges();
            }
            // Connect the synchronization to the exit node if it is a task or another synchronization
            for (Edge_list::iterator it = sync_exits.begin(); it != sync_exits.end(); ++it)
            {
                Node* child = (*it)->get_target();
                if (child->is_omp_task_node()
                    || child->is_omp_async_target_node()
                    || child->is_omp_taskwait_node()
                    || child->is_omp_barrier_graph_node())
                {
                    TDG_Node* tdg_child_task = find_tdg_node_from_pcfg_node(child);
                    // In this case, the condition is always Static
                    // Furthermore, if we have skipped nodes in the previous while
                    // it may happen that we cannot call to get_condition method
//                     const NBase& cond = (*it)->get_condition();
//                     connect_tdg_nodes(tdg_sync, tdg_child_task, (*it)->get_sync_kind(), cond);
//                     store_condition_list_of_symbols(cond, n->get_reaching_definitions_out());
                    connect_tdg_nodes(tdg_sync, tdg_child_task,
                                        __Static,
                                        /*condition*/ Nodecl::NodeclBase::null());
                }
            }

            // It may happen that a task appears after a taskwait|barrier,
            // but they are not directly connected in the PCFG, so we have to analyze that situation here
            connect_tasks_to_previous_synchronization(n);
        }

        if (n->is_graph_node())
            connect_dependent_nodes(n->get_graph_entry_node());

        // Iterate over the children
        const Node_list& children = n->get_children();
        for (Node_list::const_iterator it = children.begin(); it != children.end(); ++it)
            connect_dependent_nodes(*it);
    }

namespace {
    // BFT on the PCFG from node end_tp
    // NOTE: Any "next init taskpart" must be in the same context as end_tp
    Node* find_next_init_taskpart(Node* end_tp)
    {
        // 1.- Initialize the list of nodes we will use for the BFT
        // Function calls are wrapped in a graph node
        // and we need to work in the context of that graph node
        Node* graph_end_tp = end_tp->get_outer_node();
        std::queue<Node*> worklist;
        ObjectList<Node*> children = graph_end_tp->get_children();
        for (ObjectList<Node*>::iterator it = children.begin();
             it != children.end(); ++it)
            worklist.push(*it);

        // 2.- Actual BFT traversal
        Node* next_init_tp = NULL;
        while (!worklist.empty())
        {
            // Consider next node
            Node* n = worklist.front();
            worklist.pop();

            // Check whether it is an "init taskpart"
            if (n->is_function_call_graph_node()
                && n->get_graph_entry_node()->get_children()[0]->is_function_call_node()
                && (n->get_graph_entry_node()->get_children()[0]->get_statements()[0].as<Nodecl::FunctionCall>().get_called().get_symbol().get_name() == "GOMP_init_taskpart"))
            {
                next_init_tp = n->get_graph_entry_node()->get_children()[0];
                break;
            }

            // Keep iterating over children avoiding:
            // - back edges
            // - exiting the current context
            const ObjectList<Edge*>& exit_edges = n->get_exit_edges();
            for (ObjectList<Edge*>::const_iterator ite = exit_edges.begin();
                 ite != exit_edges.end(); ++ite)
            {
                if ((*ite)->is_back_edge() || (*ite)->is_task_edge())
                    continue;
                worklist.push((*ite)->get_target());
            }
        }
        // NOTE next_init_tp will be NULL for the last taskpart of the code

        return next_init_tp;
    }
}

    void TaskDependencyGraph::connect_taskparts()
    {
        // 1.- Connect each init taskpart to its immediate next end taskparts
        for (TDG_Node_map::iterator it = _tdg_nodes.begin();
             it != _tdg_nodes.end(); ++it)
        {
            TDG_Node* tdg_n = it->second;
            if (tdg_n->_type == Taskpart)
            {
                Node* next_init_tp = find_next_init_taskpart(tdg_n->_pcfg_node);
                for (TDG_Node_map::iterator itt = _tdg_nodes.begin();
                     itt != _tdg_nodes.end(); ++itt)
                {
                    TDG_Node* tdg_m = itt->second;
                    if (tdg_m->_type == Taskpart
                        && tdg_m->_init_taskpart == next_init_tp)
                    {
                        connect_tdg_nodes(tdg_n, tdg_m, __Static,
                                          /*condition*/ Nodecl::NodeclBase::null());
                    }
                }
            }
        }

        // 2.- Connect each task/target to its immediately previous taskpart
        for (TDG_Node_map::iterator it = _tdg_nodes.begin(); it != _tdg_nodes.end(); ++it)
        {
            TDG_Node* tdg_n = it->second;
            if (tdg_n->_type == Task || tdg_n->_type == Target)
            {
                // Get the parent node in the PCFG corresponding to the GOMP_end_taskpart call
                Node* task = tdg_n->_pcfg_node;
                Node* task_creation = ExtensibleGraph::get_task_creation_from_task(task);
                const ObjectList<Node*>& parents = task_creation->get_parents();
                ERROR_CONDITION(parents.size() != 1,
                                "A task creation node is expected to have exactly one parent "
                                "but task '%d' has '%d' parents.\n", task->get_id(), parents.size());
                ERROR_CONDITION(!parents[0]->is_function_call_graph_node(),
                                "The parent of a task creation node is expected to be a function call, "
                                "but parent of task '%d' is a %s",
                                task->get_id(), parents[0]->get_type_as_string().c_str());
                Node* previous_end_tp = parents[0]->get_graph_exit_node()->get_parents()[0];

                for (TDG_Node_map::iterator itt = _tdg_nodes.begin();
                     itt != _tdg_nodes.end(); ++itt)
                {
                    TDG_Node* tdg_m = itt->second;
                    if (tdg_m->_type == Taskpart
                        && tdg_m->_pcfg_node == previous_end_tp)
                        connect_tdg_nodes(tdg_m, tdg_n, __Static,
                                        /*condition*/ Nodecl::NodeclBase::null());
                }
            }
        }
    }

    // Returns as string a nodecl representing the value of a variable
    // When this value is a range with only one element,
    // then we return a sequence with the unique value
    std::string TaskDependencyGraph::print_value(const NBase& var, const NBase& value) const
    {
        std::string value_str;
        if (value.is<Nodecl::Range>())
        {
            const Nodecl::Range& value_range = value.as<Nodecl::Range>();
            const Nodecl::NodeclBase& lb = value_range.get_lower();
            const Nodecl::NodeclBase& ub = value_range.get_upper();
            if (report_file != NULL)
            {
                if (lb.is<Nodecl::Analysis::MinusInfinity>()
                    || ub.is<Nodecl::Analysis::PlusInfinity>())
                {
                    fprintf(report_file,
                            "    Infinit loop boundary found for variable '%s' in line '%d'. "
                            "Boxer will not be able to expand the TDG properly.\n",
                            var.prettyprint().c_str(),
                            var.get_line());
                }
            }
            if (Nodecl::Utils::structurally_equal_nodecls(lb, ub, /*skip_conversions*/true)
                    || (lb.is_constant() && ub.is_constant()
                            && const_value_is_zero(const_value_sub(lb.get_constant(), ub.get_constant()))))
            {   // Return a sequence with the unique value
                value_str = "{" + lb.prettyprint() + "}";
            }
            else
            {   // Return the range, since it has more than one value
                value_str = value.prettyprint();
            }
        }
        else
        {   // Normally, return the string representing the value
            value_str = value.prettyprint();
        }
        return value_str;
    }

    void TaskDependencyGraph::print_tdg_node_to_dot(TDG_Node* tdg_n, std::ofstream& dot_tdg) const
    {
        // Print the control structures (subgraphs) where the node is enclosed in
        ControlStList control_structures = tdg_n->get_control_structures();
        std::string indent = "\t";
        unsigned int n_cs = 0;
        for (ControlStList::const_reverse_iterator it = control_structures.rbegin();
             it != control_structures.rend(); ++it)
        {
            ControlStructure* cs = it->first;
            if (cs->get_type() != Implicit)
            {
                dot_tdg << indent << "subgraph cluster_" << ++node_id << "{\n";
                indent += "\t";
                std::string label = cs->get_condition().prettyprint();
                if (cs->get_type()==IfElse) {
                    std::string branch = it->second;
                    label += " (" + std::string(branch=="1" ? "true" : "false") + ")";
                }
                dot_tdg << indent << "label=\"" << label << "\";\n";
                dot_tdg << indent << "color=\"" << (cs->get_type()==Loop ? "deeppink" : "deepskyblue1") << "\";\n";
                dot_tdg << indent << "style=\"dashed\";\n";
                ++n_cs;
            }
        }
        
        // Create the node
        Node* n = tdg_n->_pcfg_node;
        std::string task_label = "";
        TDGNodeType ntype = tdg_n->_type;
        if(ntype == Task)
        {
            // Get the name of the task
            Nodecl::OpenMP::Task task = n->get_graph_related_ast().as<Nodecl::OpenMP::Task>();
            task_label = "Task :: " + task.get_locus_str();
            Nodecl::List environ = task.get_environment().as<Nodecl::List>();
            for(Nodecl::List::iterator it = environ.begin(); it != environ.end(); ++it)
                if(it->is<Nodecl::OmpSs::TaskLabel>())
                {
                    task_label = "_" + it->prettyprint();
                    break;
                }
        }
        else if (ntype == Taskpart)
        {
            NBase tp_stmt = n->get_statements()[0];
            std::stringstream ss;
                    ss << tdg_n->_init_taskpart->get_id();
                    ss << " -> ";
                    ss << tdg_n->_pcfg_node->get_id();
            task_label = "Taskpart :: " + ss.str();
        }
        else if(ntype == Taskwait)
        {
            NBase tw_stmt = n->get_statements()[0];
            task_label = "Taskwait :: " + tw_stmt.get_locus_str();
        }
        else if(ntype == Barrier)
        {
            NBase barrier_stmt = n->get_graph_related_ast();
            task_label = "Barrier :: " + barrier_stmt.get_locus_str();
        }
        
        // print the node
        std::stringstream ss; ss << tdg_n->_id;
        std::string current_id = ss.str();
        dot_tdg << indent << current_id << " [label=\"[" << current_id << "] " << task_label << "\"];\n";
        
        
        // Close the subgraphs of the control structures
        for(unsigned int i = 0; i < n_cs; ++i)
        {
            indent = indent.substr(0, indent.size()-1);
            dot_tdg << indent << "}\n";
        }
        
        // Create the connections from the current node to its children
        std::string headlabel, taillabel, style, condition;
        for(TDG_Edge_list::iterator it = tdg_n->_exits.begin(); it != tdg_n->_exits.end(); ++it)
        {
            // Get the edge info in a string
            style = "style=\"" + std::string((*it)->_kind == __Static ? "solid" : "dashed") + "\"";
            if(!(*it)->_condition.is_null())
                condition = ", label=\"" + (*it)->_condition.prettyprint() + "\"";
            else
                condition = ", label=\"true\"";
            // Create the dot edge
            std::stringstream child_id; child_id << (*it)->_target->_id;
            dot_tdg << "\t" << current_id << " -> " << child_id.str() 
                    << "[" << style << condition /*<< headlabel << ", " << taillabel*/ << "];\n";
        }
    }
    
    void TaskDependencyGraph::print_tdg_to_dot() const
    {
        // Create the directory of dot files if it has not been previously created
        char buffer[1024];
        char* err = getcwd(buffer, 1024);
        if(err == NULL)
            internal_error ("An error occurred while getting the path of the current directory", 0);
        struct stat st;
        std::string directory_name = std::string(buffer) + "/dot/";
        if(stat(directory_name.c_str(), &st) != 0)
        {
            int dot_directory = mkdir(directory_name.c_str(), S_IRWXU);
            if(dot_directory != 0)
                internal_error ("An error occurred while creating the dot directory in '%s'", 
                                 directory_name.c_str());
        }
        
        // Create the file where we will store the DOT TDG
        std::string dot_file_name = directory_name + _pcfg->get_name() + "_tdg.dot";
        std::ofstream dot_tdg;
        dot_tdg.open(dot_file_name.c_str());
        if(!dot_tdg.good())
            internal_error ("Unable to open the file '%s' to store the TDG.", dot_file_name.c_str());
        
        // Create the DOT graphs
        if(VERBOSE)
            std::cerr << "- TDG DOT file '" << dot_file_name << "'" << std::endl;
        dot_tdg << "digraph TDG {\n";
            dot_tdg << "\tcompound=true;\n";
            for (TDG_Node_map::const_iterator it = _tdg_nodes.begin(); it != _tdg_nodes.end(); ++it)
                print_tdg_node_to_dot(it->second, dot_tdg);
        dot_tdg << "}\n";
        dot_tdg.close();
        if(!dot_tdg.good())
            internal_error ("Unable to close the file '%s' where TDG has been stored.", dot_file_name.c_str());
        ExtensibleGraph::clear_visits(_pcfg->get_graph());
    }
    
    void TaskDependencyGraph::print_tdg_control_structs_to_json(std::ofstream& json_tdg) const
    {
        json_tdg << "\t\t\"control_structures\" : [\n";
        
        // Print the Controls Structures involved in the tasks instantiation
        if(!_pcfg_to_cs_map.empty())
        {
            for(PCFG_to_CS::const_iterator it = _pcfg_to_cs_map.begin(); it != _pcfg_to_cs_map.end(); )
            {
                ControlStructure* cs = it->second;
                json_tdg << "\t\t\t{\n";
                
                    json_tdg << "\t\t\t\t\"id\" : " << cs->get_id() << ",\n";
                    json_tdg << "\t\t\t\t\"type\" : \"" << cs->get_type_as_string();
                    if(cs->get_pcfg_node() != NULL)
                    {
                        json_tdg << "\",\n";
                        json_tdg << "\t\t\t\t\"locus\" : \"" << cs->get_pcfg_node()->get_graph_related_ast().get_locus_str() << "\",\n";
                        json_tdg << "\t\t\t\t\"when\" : {\n";
                        print_condition(NULL, cs, json_tdg, "\t\t\t\t\t");
                        json_tdg << "\t\t\t\t}";
                        if(cs->get_type() == IfElse)
                        {
                            json_tdg << ",\n";
                            unsigned int nbranches = 0;
                            ObjectList<Node*> branches = cs->get_pcfg_node()->get_condition_node()->get_children();
                            // branches list contains 2 elements
                            if(!branches[0]->is_exit_node())
                                nbranches++;
                            if(!branches[1]->is_exit_node())
                                nbranches++;
                            json_tdg << "\t\t\t\t\"nbranches\" : " << nbranches << "\n";
                        }
                        else
                        {
                            json_tdg << "\n";
                        }
                    }
                    else
                    {
                        // If the task has some dependency with variables involved,
                        // add here the values of those variables
                        json_tdg << "\"\n";
                    }
                    
                ++it;
                if(it != _pcfg_to_cs_map.end())
                    json_tdg << "\t\t\t},\n";
                else
                    json_tdg << "\t\t\t}\n";
            }
        }
        
        json_tdg << "\t\t],\n" ;
    }

    void TaskDependencyGraph::print_tdg_syms_to_json(std::ofstream& json_tdg)
    {
        if (!_syms.empty())
        {
            json_tdg << "\t\t\"variables\" : [\n";
            for (std::map<NBase, unsigned int>::iterator it = _syms.begin(); it != _syms.end(); )
            {
                NBase n = it->first;
                json_tdg << "\t\t\t{\n";
                    json_tdg << "\t\t\t\t\"id\" : " << ++tdg_var_id << ",\n";
                    json_tdg << "\t\t\t\t\"name\" : \"" << n.prettyprint() << "\",\n";
                    json_tdg << "\t\t\t\t\"locus\" : \"" << n.get_locus_str() << "\",\n";
                    Type t = n.get_type().no_ref().advance_over_typedefs().advance_over_typedefs();
                    json_tdg << "\t\t\t\t\"type\" : \"" << t.get_declaration(n.retrieve_context(), /*no symbol name*/"") << "\"\n";
                _syms[n] = tdg_var_id;
                ++it;
                if (it != _syms.end())
                    json_tdg << "\t\t\t},\n";
                else
                    json_tdg << "\t\t\t}\n";
            }
            json_tdg << "\t\t],\n" ;
        }
    }

    static void replace_vars_values_with_ids(const VarToNodeclMap& var_to_id_map, NBase& value)
    {
        NodeclReplacer nr(var_to_id_map);
        nr.walk(value);
    }
    
    void TaskDependencyGraph::print_dependency_variables_to_json(
            std::ofstream& json_tdg,
            const VarToNodeclMap& var_to_value_map,
            const VarToNodeclMap& var_to_id_map,
            const NBase& condition,
            std::string indent,
            bool is_source,
            bool add_final_comma) const
    {
        for (VarToNodeclMap::const_iterator it = var_to_value_map.begin(); it != var_to_value_map.end(); )
        {
            std::map<NBase, unsigned int>::const_iterator its = _syms.find(it->first);
            ERROR_CONDITION(its == _syms.end(),
                            "Variable %s, found in condition %s, "
                            "has not been found during the phase of gathering the variables",
                            it->first.prettyprint().c_str(), condition.prettyprint().c_str());
            json_tdg << indent << "\t{\n";
            json_tdg << indent << "\t\t\"id\" : " << its->second << ",\n";
            NBase value = it->second.shallow_copy();
            replace_vars_values_with_ids(var_to_id_map, value);
            json_tdg << indent << "\t\t\"values\" : \""<< print_value(its->first, value) << "\",\n";
            json_tdg << indent << "\t\t\"side\" : \"" << (is_source ? "source" : "target") << "\"\n";
            ++it;
            if (it != var_to_value_map.end() || add_final_comma)
                json_tdg << indent << "\t},\n";
            else
                json_tdg << indent << "\t}\n";
        }
    }

    void TaskDependencyGraph::print_condition(
            TDG_Edge* edge,
            ControlStructure* n,
            std::ofstream& json_tdg,
            std::string indent) const
    {
        json_tdg << indent << "\"expression\" : ";
        assert(edge!=NULL || n!=NULL);
        if ((edge != NULL && !edge->_condition.is_null()) || (n != NULL))
        {
            NBase condition;
            // 1.- This condition belongs to a control structure
            if (n != NULL)
            {
                // Get the condition
                condition = n->get_condition().shallow_copy();
                VarToNodeclMap var_to_value_map;
                ObjectList<NBase> ordered_vars;
                transform_node_condition_into_json_expr(n, condition,
                        var_to_value_map, ordered_vars);
                json_tdg << "\"" << condition.prettyprint() << "\",\n";
                // Get the IVs of the related PCFG if the node is a Loop control structure
                Utils::InductionVarList ivs;
                if (n->get_type() == Loop)
                    ivs = n->get_pcfg_node()->get_induction_variables();
                // Generate the list of involved variables
                json_tdg << indent << "\"vars\" : [\n";
                for (ObjectList<NBase>::iterator itt = ordered_vars.begin(); itt != ordered_vars.end(); )
                {
                    VarToNodeclMap::iterator it = var_to_value_map.find(*itt);
                    std::map<NBase, unsigned int>::const_iterator its = _syms.find(it->first);
                    ERROR_CONDITION(its == _syms.end(),
                                    "Variable %s, found in condition %s, "
                                    "has not been found during the phase of gathering the variables",
                                    it->first.prettyprint().c_str(), condition.prettyprint().c_str());
                    json_tdg << indent << "\t{\n";
                        json_tdg << indent << "\t\t\"id\" : " << its->second << ",\n";
                        if (!ivs.empty())
                        {   // Only induction variables are always printed as ranges.
                            // Other kinds of variables are printed as sequences if they can take only one value
                            if (Utils::induction_variable_list_contains_variable(ivs, it->first))
                                json_tdg << indent << "\t\t\"values\" : \""<< it->second.prettyprint() << "\"\n";
                            else
                                json_tdg << indent << "\t\t\"values\" : \""<< print_value(it->first, it->second) << "\"\n";
                        }
                        else
                            json_tdg << indent << "\t\t\"values\" : \""<< print_value(it->first, it->second) << "\"\n";
                    ++itt;
                    if (itt != ordered_vars.end())
                        json_tdg << indent << "\t},\n";
                    else
                        json_tdg << indent << "\t}\n";
                }
            }
            // 2.- This condition belongs to a dependency expression
            else
            {
                // Get the condition
                condition = edge->_condition.shallow_copy();
                VarToNodeclMap source_var_to_value_map, target_var_to_value_map;
                VarToNodeclMap source_var_to_id_map, target_var_to_id_map;
                transform_edge_condition_into_json_expr(edge, condition,
                        source_var_to_value_map, target_var_to_value_map,
                        source_var_to_id_map, target_var_to_id_map);
                json_tdg << "\"" << condition.prettyprint() << "\",\n";
                // Generate the list of involved variables
                json_tdg << indent << "\"vars\" : [\n";
                print_dependency_variables_to_json(json_tdg, source_var_to_value_map, source_var_to_id_map,
                        condition, indent, /*is_source*/true, /*add_final_comma*/true);
                print_dependency_variables_to_json(json_tdg, target_var_to_value_map, target_var_to_id_map,
                        condition, indent, /*is_source*/false, /*add_final_comma*/false);
            }
            
            json_tdg << indent << "]\n";
        }
        else
        {   // There is no condition => TRUE
            json_tdg << "true\n";
        }
    }

namespace {
    void compute_size(Type t, std::stringstream& size)
    {
        // Base case: the compiler cannot compute the size of the type
        if (t.is_incomplete())
        {
           size << "sizeof(" << t.print_declarator() << ")";
        }
        // The compiler computes the size
        else
        {
            size << t.get_size();
        }
    }

    void get_size_str(const Nodecl::List& map_vars, std::stringstream& size)
    {
        for (Nodecl::List::const_iterator itv = map_vars.begin();
             itv != map_vars.end(); )
        {
            compute_size(itv->get_type().no_ref(), size);
            ++itv;
            if (itv != map_vars.end())
                size << " + ";
        }
    }
}

    void TaskDependencyGraph::print_tdg_nodes_to_json(std::ofstream& json_tdg)
    {
        json_tdg << "\t\t\"nodes\" : [\n";
        for (TDG_Node_map::const_iterator it = _tdg_nodes.begin(); it != _tdg_nodes.end(); )
        {
            TDG_Node* n = it->second;
            json_tdg << "\t\t\t{\n";
                
            // node identifier
            json_tdg << "\t\t\t\t\"id\" : " << n->_id << ",\n";
                
            // node locus and type
            if (n->_type == Task || n->_type == Target || n->_type == Barrier)
            {   // These are graph nodes
                // We split locus into filename and line because
                // for implicit barrier nodes the line is computed when filling the _tdg_nodes map
                json_tdg << "\t\t\t\t\"locus\" : \"" << n->_pcfg_node->get_graph_related_ast().get_filename()
                         << ":" << it->first << "\",\n";
                std::string type_str =
                        ((n->_type == Task) ? "Task"
                                            : ((n->_type == Target) ? "Target"
                                                                    : "Barrier"));
                json_tdg << "\t\t\t\t\"type\" : \"" << type_str << "\"";

                if (n->_type == Target)
                {   // Add the size of the data moved into the device and from it
                    PCFGPragmaInfo clauses = n->_pcfg_node->get_pragma_node_info();
                    std::stringstream size_in, size_out;
                    if (clauses.has_clause(NODECL_OPEN_M_P_MAP_TO))
                    {
                        Nodecl::OpenMP::MapTo map_to = clauses.get_clause(NODECL_OPEN_M_P_MAP_TO).as<Nodecl::OpenMP::MapTo>();
                        get_size_str(map_to.get_map_to().as<Nodecl::List>(), size_in);
                    }
                    if (clauses.has_clause(NODECL_OPEN_M_P_MAP_FROM))
                    {
                        Nodecl::OpenMP::MapFrom map_from = clauses.get_clause(NODECL_OPEN_M_P_MAP_FROM).as<Nodecl::OpenMP::MapFrom>();
                        get_size_str(map_from.get_map_from().as<Nodecl::List>(), size_out);
                    }
                    if (clauses.has_clause(NODECL_OPEN_M_P_MAP_TO_FROM))
                    {
                        if (!size_in.str().empty())
                        {
                            size_in << " + ";
                        }
                        if (!size_out.str().empty())
                        {
                            size_out << " + ";
                        }
                        Nodecl::OpenMP::MapToFrom map_tofrom = clauses.get_clause(NODECL_OPEN_M_P_MAP_TO_FROM).as<Nodecl::OpenMP::MapToFrom>();
                        get_size_str(map_tofrom.get_map_tofrom().as<Nodecl::List>(), size_in);
                        get_size_str(map_tofrom.get_map_tofrom().as<Nodecl::List>(), size_out);
                    }
                    json_tdg << ",\n";
                    json_tdg << "\t\t\t\t\"size_in\" : \"" << size_in.str() << "\",\n";
                    json_tdg << "\t\t\t\t\"size_out\" : \"" << size_out.str() << "\"";
                }
            }
            else
            {   // This is a normal node
                json_tdg << "\t\t\t\t\"locus\" : \"" << n->_pcfg_node->get_statements()[0].get_filename()
                         << ":" << it->first << "\",\n";
                if (n->_type == Taskpart)
                {
                    // FIXME Since boxer has not been modified to accept Taskparts,
                    //       we represent them as Tasks
//                     json_tdg << "\t\t\t\t\"type\" : \"Taskpart\"";
                    json_tdg << "\t\t\t\t\"type\" : \"Task\"";

                    // FIXME Since boxer has not been modified to accept the new attribute 'parent_id'
                    //       we do not insert it yet
                    // Add the identifier of the creating task (0 by default) and
                    // the order of the taskpart
//                     int parent_id = (n->_parent->get_id() == 1
//                             ? 0 /* parent task is the implicit task */
//                             : find_tdg_node_from_pcfg_node(n->_parent)->_id);
//                     json_tdg << ",\n";
//                     json_tdg << "\t\t\t\t\"parent_id\" : \"" << parent_id << "\"";
                }
                else if (n->_type == Taskwait)
                {
                    json_tdg << "\t\t\t\t\"type\" : \"Taskwait\"";
                }
                else
                {
                    internal_error("Unexpected TDG node type %d.\n", n->_type);
                }
            }

            // node control structures
            ControlStList control_structures = n->get_control_structures();
            json_tdg << ",\n";
            json_tdg << "\t\t\t\t\"control\" : [\n";
            for (ControlStList::iterator itt = control_structures.begin(); itt != control_structures.end(); )
            {
                json_tdg << "\t\t\t\t\t{\n";
                json_tdg << "\t\t\t\t\t\t\"control_id\" : " << itt->first->get_id();
                if (itt->first->get_type() == IfElse)
                {
                    json_tdg << ",\n";
                    json_tdg << "\t\t\t\t\t\t\"branch_id\" : [" << itt->second << "]\n";
                }
                else
                {
                    json_tdg << "\n";
                }
                ++itt;
                if (itt != control_structures.end())
                    json_tdg << "\t\t\t\t\t},\n";
                else
                    json_tdg << "\t\t\t\t\t}\n";
            }
            json_tdg << "\t\t\t\t]\n";
            
            ++it;
            if (it != _tdg_nodes.end())
                json_tdg << "\t\t\t},\n";
            else
                json_tdg << "\t\t\t}\n";
        }
        json_tdg << "\t\t]";
    }
    
    void TaskDependencyGraph::print_tdg_edges_to_json(std::ofstream& json_tdg) const
    {
        // Get all edges in the graph
        TDG_Edge_list edges;
        for (TDG_Node_map::const_iterator it = _tdg_nodes.begin(); it != _tdg_nodes.end(); ++it)
            edges.append(it->second->_exits);
        
        // Print the edges into the dot file
        if(!edges.empty())
        {
            json_tdg << ",\n";
            json_tdg << "\t\t\"dependencies\" : [\n";
            for(TDG_Edge_list::iterator it = edges.begin(); it != edges.end(); )
            {
                json_tdg << "\t\t\t{\n";
                    json_tdg << "\t\t\t\t\"source\" : " << (*it)->_source->_id << ",\n";
                    json_tdg << "\t\t\t\t\"target\" : " << (*it)->_target->_id << ",\n";
                    json_tdg << "\t\t\t\t\"when\" : {\n";
                        print_condition(*it, NULL, json_tdg, "\t\t\t\t\t");
                    json_tdg << "\t\t\t\t}\n";
                ++it;
                if(it != edges.end())
                    json_tdg << "\t\t\t},\n";
                else
                    json_tdg << "\t\t\t}\n";
            }
            json_tdg << "\t\t]\n";
        }
        else
            json_tdg << "\n";
    }

    void TaskDependencyGraph::print_tdg_to_json(std::ofstream& json_tdg)
    {
        json_tdg << "\t{\n";
            TL::Symbol sym = _pcfg->get_function_symbol();
            json_tdg << "\t\t\"tdg_id\" : " << _id << ",\n";
            json_tdg << "\t\t\"function\" : \"" << (sym.is_valid() ? sym.get_name() : "") << "\",\n";
            json_tdg << "\t\t\"locus\" : \"" << _pcfg->get_nodecl().get_locus_str() << "\",\n";
            print_tdg_syms_to_json(json_tdg);
            print_tdg_control_structs_to_json(json_tdg);
            print_tdg_nodes_to_json(json_tdg);
            print_tdg_edges_to_json(json_tdg);
        json_tdg << "\t}";

        ExtensibleGraph::clear_visits(_pcfg->get_graph());
    }

    void TaskDependencyGraph::print_tdgs_to_json(const ObjectList<TaskDependencyGraph*>& tdgs)
    {
        if (tdgs.empty())
            return;

        // 1.- Create the directory of json files if it has not been previously created
        char directory_name[1024];
        char* err = getcwd(directory_name, 1024);
        if(err == NULL)
            internal_error ("An error occurred while getting the path of the current directory", 0);
        struct stat st;
        std::string full_directory_name = std::string(directory_name) + "/json/";
        if(stat(full_directory_name.c_str(), &st) != 0)
        {
            int json_directory = mkdir(full_directory_name.c_str(), S_IRWXU);
            if(json_directory != 0)
                internal_error ("An error occurred while creating the json directory in '%s'",
                                 full_directory_name.c_str());
        }

        // 2.- Create the file where we will store the JSON TDG
        std::string json_file_name = full_directory_name + "tdgs.json";
        std::ofstream json_tdg;
        json_tdg.open(json_file_name.c_str());
        if(!json_tdg.good())
            internal_error ("Unable to open the file '%s' to store the TDG.", json_file_name.c_str());

        // 3.- Create (or open if already exists) the file where the PSocrates reports will be stored
        if (full_report_name == "")
        {
            ExtensibleGraph* pcfg = (*tdgs.begin())->get_pcfg();
            std::string report_name = pcfg->get_nodecl().get_filename();
            report_name = report_name.substr(0, report_name.find(".", 0));
            full_report_name = std::string(directory_name) + "/" + report_name + "_psocrates.report";
            report_file = fopen(full_report_name.c_str(), "wt");
            TL::CompiledFile current = TL::CompilationProcess::get_current_file();
            std::string current_file = current.get_filename();
            fputs("/* ---------------------------------------------------------\n", report_file);
            fputs("   This file is a report of the Task Dependency Graphs\n", report_file);
            fputs("   generated in the context of the P-Socrates project.\n", report_file);
            fputs("   Information about non-supported features is provided.\n\n", report_file);
            fprintf(report_file, "   ===== Compiled file '%s' =====\n\n", current_file.c_str());
            fputs("--------------------------------------------------------- */\n\n\n\n", report_file);
        }
        else
        {
            report_file = fopen(full_report_name.c_str(), "a");
        }
        if (report_file == NULL)
        {
            internal_error("Unable to open the file '%s' to store Psocrates report.",
                           full_report_name.c_str());
        }

        // 4.- Create the JSON graphs ordered by identifier (filling the report on the way)
        if(VERBOSE)
            std::cerr << "- TDG JSON file '" << json_file_name << "'" << std::endl;
        json_tdg << "{\n";
        json_tdg << "\"num_tdgs\" : " << tdgs.size() << ",\n";
        json_tdg << "\"tdgs\" : [\n";
        unsigned int current_id = 0;
        std::set<TaskDependencyGraph*> tdgs_set(tdgs.begin(), tdgs.end());
        std::set<TaskDependencyGraph*>::iterator it = tdgs_set.begin();
        while (!tdgs_set.empty() && it != tdgs_set.end())
        {
            if ((*it)->get_id() != current_id)
            {
                ++it;
                continue;
            }

            //  Print in report's file
            ExtensibleGraph* pcfg = (*it)->get_pcfg();
            std::string func_name =
                    (pcfg->get_function_symbol().is_valid()
                            ? pcfg->get_function_symbol().get_name()
                            : (*it)->get_name());
            fprintf(report_file, "Funtion '%s' \n", func_name.c_str());

            (*it)->print_tdg_to_json(json_tdg);

            // Prepare next iteration
            current_id++;
            tdgs_set.erase(it);
            it = tdgs_set.begin();

            //  Print in report's file
            fprintf(report_file, "END funtion '%s'\n\n", func_name.c_str());

            // Print in JSON's file
            if (!tdgs_set.empty())
                json_tdg << ",\n";
            else
                json_tdg << "\n";

        }
        ERROR_CONDITION(!tdgs_set.empty(),
                        "Not all TDGs have been printed!\n", 0);

        json_tdg << "]\n";
        json_tdg << "}\n";

        // 5.- Close the JSON file
        json_tdg.close();
        if (!json_tdg.good())
            internal_error ("Unable to close the file '%s' where PCFG has been stored.", json_file_name.c_str());

        // 6.- Close the report file
        int res = fclose(report_file);
        if (res == EOF)
            internal_error("Unable to close the file '%s' where Psocrates report has been stored.", full_report_name.c_str());
    }
}
}
