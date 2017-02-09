/*--------------------------------------------------------------------
  (C) Copyright 2006-2013 Barcelona Supercomputing Center
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
// --------------------------------------------------------------------*/

#include "cxx-cexpr.h"
#include "tl-counters.hpp"
#include "tl-psocrates-phase.hpp"
#include "tl-analysis-base.hpp"
#include "tl-analysis-utils.hpp"
#include "tl-symbol-utils.hpp"

namespace TL {
namespace Analysis {

    // *********************************************************************************** //
    // ******************* Code transformations for compiling with GCC ******************* //

namespace {
    TL::Counter &task_id = TL::CounterManager::get_counter("psocrates-task-id");
    TL::Counter &tdg_counter = TL::CounterManager::get_counter("psocrates-tdg");

    Nodecl::ExpressionStatement create_func_call_stmt(
            std::string func_name,
            const ObjectList<std::string>& param_names,
            const ObjectList<Type>& param_types,
            const Nodecl::List& arguments)
    {
        Scope global_sc = Scope::get_global_scope();

        // 1.- Create a reference to the function (make sure we only create the symbol once)
        TL::Symbol func_sym = global_sc.get_symbol_from_name(func_name);
        if (!func_sym.is_valid())
        {
            func_sym = SymbolUtils::new_function_symbol(
                /*scope*/ global_sc,
                /*function name*/ func_name,
                /*return symbol name*/ "",
                /*return type*/ TL::Type::get_void_type(),
                /*parameter names*/ param_names,
                /*parameter types*/ param_types);
            // Make it external
            symbol_entity_specs_set_is_extern(func_sym.get_internal_symbol(), 1);
            symbol_entity_specs_set_is_static(func_sym.get_internal_symbol(), 0);
        }

        // 2.- Create a reference to the function symbol
        Nodecl::NodeclBase func_ref = Nodecl::Symbol::make(func_sym);
        func_ref.set_type(func_sym.get_type().get_lvalue_reference_to());

        // 3.- Create the call to the function
        Nodecl::FunctionCall func_call_expr = Nodecl::FunctionCall::make(
                func_ref,
                /* arguments */ arguments,
                /* alternate name */ Nodecl::NodeclBase::null(),
                /* function form */ Nodecl::NodeclBase::null(),
                Type::get_void_type());

        // 4.- Insert the call in the AST
        return Nodecl::ExpressionStatement::make(func_call_expr);
    }
}

    /* **************************** Task Parts **************************** */



    /* *********************** GOMP Transformations *********************** */

namespace {

    bool In_single = false;

    class LIBTL_CLASS TaskVisitor : public Nodecl::ExhaustiveVisitor<void>
    {
    private:
        bool _contains_task;
    public:
        TaskVisitor()
            : _contains_task(false)
        {}

        bool contains_task() const
        {
            return _contains_task;
        }

        void visit(const Nodecl::OpenMP::Task& n)
        {
            _contains_task = true;
        }
    };

    void tokenizer(std::string str, std::set<std::string>& result)
    {
        std::string temporary("");
        for (std::string::const_iterator it = str.begin();
             it != str.end(); ++it)
        {
            const char & c(*it);
            if (c == ',' || c == ' ')
            {
                if (temporary != "")
                {
                    std::cerr << "   -> " << temporary << std::endl;
                    result.insert(temporary);
                    temporary = "";
                }
            }
            else
            {
                temporary += c;
            }
        }
        if (temporary != "")
        {
            result.insert(temporary);
        }
    }

    enum TraceKind {
        TraceRuntime,
        TraceTask,
        TraceTaskpart
    };

    Nodecl::ExpressionStatement create_call_to_tracepoint(
            /*starting point of the task part*/ bool init,
            /*tracepoint in a task*/ TraceKind trace_kind)
    {
        std::string func_call_name;
        if (trace_kind == TraceTask)
        {
            if (init)
                func_call_name = "GOMP_start_tracepoint_task";
            else
                func_call_name = "GOMP_stop_tracepoint_task";

            return create_func_call_stmt(
                    /*func name*/ func_call_name,
                    /*param names*/ ObjectList<std::string>(),
                    /*param type*/ ObjectList<Type>(),
                    /*arguments*/ Nodecl::List());
        }
        else if (trace_kind == TraceTaskpart)
        {
            if (init)
                func_call_name = "GOMP_start_tracepoint_taskpart";
            else
                func_call_name = "GOMP_stop_tracepoint_taskpart";

            return create_func_call_stmt(
                    /*func name*/ func_call_name,
                    /*param names*/ ObjectList<std::string>(),
                    /*param type*/ ObjectList<Type>(),
                    /*arguments*/ Nodecl::List());
        }
        else if (trace_kind == TraceRuntime)
        {
            if (init)
                func_call_name = "GOMP_start_tracepoint_runtime";
            else
                func_call_name = "GOMP_stop_tracepoint_runtime";

            Type t = Type::get_unsigned_int_type();
            Nodecl::IntegerLiteral GOMP_task_id = Nodecl::IntegerLiteral::make(
                    /*type*/ t,
                    /*value*/ const_value_get_integer(task_id, /*num_bytes*/4, /*sign*/0));
            return create_func_call_stmt(
                    /*func name*/ func_call_name,
                    /*param names*/ ObjectList<std::string>(1, "GOMP_task_id"),
                    /*param type*/ ObjectList<Type>(1, t),
                    /*arguments*/ Nodecl::List::make(GOMP_task_id));
        }
        else
        {
            internal_error("Unreachable code", 0);
        }
    }
}


    /* ************************ Transformations previous to Analysis ************************ */

    GOMPtransformationsBeforeAnalysis::GOMPtransformationsBeforeAnalysis(Instrumentation instrumentation)
        : _instrumentation(instrumentation)
    {}

    void GOMPtransformationsBeforeAnalysis::unhandled_node(const Nodecl::NodeclBase& n)
    {
        WARNING_MESSAGE("Unhandled node of type '%s' while libGOMP transformations.\n",
                        ast_print_node_type(n.get_kind()));
    }

    // Add GOMP TDG set/unset runtime calls
    void GOMPtransformationsBeforeAnalysis::visit(const Nodecl::FunctionCode& n)
    {
        Nodecl::Context ctx = n.get_statements().as<Nodecl::Context>();
        Nodecl::List stmts = ctx.get_in_context().as<Nodecl::List>();

        // 1.- We only include calls to the GOMP library if the loop contains some task
        TaskVisitor tv;
        tv.walk(n);
        if (tv.contains_task())
        {
            task_id = 0;

            // 2.- Insert calls to GOMP_set_tdg_id() and GOMP_set_tdg_id()
            // 2.1.- Traverse the structure of the AST for a C/C++ Function
            Nodecl::CompoundStatement cmp_stmt = stmts.front().as<Nodecl::CompoundStatement>();
            Nodecl::List cmp_stmts = cmp_stmt.get_statements().as<Nodecl::List>();
            // 2.2.- Prepend set_id runtime call as the first statement of the function
            Nodecl::NodeclBase first_stmt = cmp_stmts.front();
            Type t = Type::get_unsigned_int_type();
            Nodecl::IntegerLiteral tdg_id = Nodecl::IntegerLiteral::make(
                    /*type*/ t,
                    /*value*/ const_value_get_integer(tdg_counter, /*num_bytes*/4, /*sign*/0));
            tdg_counter++;
            Nodecl::ExpressionStatement gomp_set_id_stmt = create_func_call_stmt(
                    /*func name*/ "GOMP_set_tdg_id",
                    /*param names*/ ObjectList<std::string>(1, "tdg_id"),
                    /*param type*/ ObjectList<Type>(1, t),
                    /*arguments*/ Nodecl::List::make(tdg_id));
            cmp_stmts.prepend(gomp_set_id_stmt);
            // 2.3.- Append taskpart runtime call as the last statement of the function
            Nodecl::NodeclBase last_stmt = cmp_stmts.back();
            Nodecl::ExpressionStatement gomp_unset_id_stmt = create_func_call_stmt(
                    /*func name*/ "GOMP_unset_tdg_id",
                    /*param names*/ ObjectList<std::string>(),
                    /*param type*/ ObjectList<Type>(),
                    /*arguments*/ Nodecl::List());
            if (last_stmt.is<Nodecl::ReturnStatement>())
            {
                last_stmt.prepend_sibling(gomp_unset_id_stmt);
            }
            else
            {
                last_stmt.append_sibling(gomp_unset_id_stmt);
            }
        }

        // 3.- Visit inner statements to apply other possible transformation (i.e. target id)
        walk(stmts);
    }

    void GOMPtransformationsBeforeAnalysis::visit(const Nodecl::OpenMP::For& n)
    {
        internal_error("OpenMP loops are not supported.\n", 0);
    }

    void GOMPtransformationsBeforeAnalysis::visit(const Nodecl::OpenMP::Master& n)
    {
        In_single = true;
        if (_instrumentation == All)
        {
            // 1.- Insert taskpart runtime calls at the beginning and the end of the single list of statements
            Nodecl::Context ctx = n.get_statements().as<Nodecl::List>().front().as<Nodecl::Context>();
            Nodecl::List cmp_stmt = ctx.get_in_context().as<Nodecl::List>();
            Nodecl::List stmts;
            // If the statements of the task are inside a CompoundStatement,
            //    just insert the calls at the beginning and the end of the list
            // Otherwise, we have to wrap the unique statement of the task in a
            //    CompoundStatement before inserting the calls.
            if (cmp_stmt[0].is<Nodecl::CompoundStatement>())
            {
                stmts = cmp_stmt[0].as<Nodecl::CompoundStatement>().get_statements().as<Nodecl::List>();
            }
            else
            {
                stmts = cmp_stmt.shallow_copy().as<Nodecl::List>();
                cmp_stmt.replace(Nodecl::CompoundStatement::make(stmts, Nodecl::NodeclBase::null()));
            }
            if (!stmts.empty())
            {
                stmts.prepend(create_call_to_tracepoint(/*init*/true, /*tracekind*/TraceTaskpart));
                stmts.append(create_call_to_tracepoint(/*init*/false, /*tracekind*/TraceTaskpart));
            }
        }

        walk(n.get_statements());
        In_single = false;
    }

    void GOMPtransformationsBeforeAnalysis::visit(const Nodecl::OpenMP::Parallel& n)
    {
        // TODO So far, implicit tasks are not yet instrumented
        // If so, identifiers will be broken since the parallel construct does not have a task_id associated

//         // 1.- Insert tracing calls before and after the parallel region if runtime instrumentation is requested
//         //     This is the implicit task created in the parallel
//         if (_instrumentation == All)
//         {
//             // 1.- Prepend taskpart runtime call before the parallel
//             n.prepend_sibling(create_call_to_tracepoint(/*init*/true, /*tracekind*/TraceRuntime));
//             n.append_sibling(create_call_to_tracepoint(/*init*/false, /*tracekind*/TraceRuntime));
//         }
// 
//         if (_instrumentation == All)
//         {
//             // 2.- Insert tracing calls at the beginning and the end of the parallel region
//             Nodecl::Context ctx = n.get_statements().as<Nodecl::List>().front().as<Nodecl::Context>();
//             Nodecl::NodeclBase stmt = ctx.get_in_context().as<Nodecl::List>().front();
//             // Case: there is only one statement associated with the directive
//             //       and no CompoundStatement wraps it
//             // Behavior: Insert a CompoundStatement before inserting the calls to taskparts methods
//             if (!stmt.is<Nodecl::CompoundStatement>())
//             {
//                 Nodecl::CompoundStatement cmp_stmt =
//                         Nodecl::CompoundStatement::make(Nodecl::List::make(stmt.shallow_copy()),
//                                                         Nodecl::NodeclBase::null());
//                 stmt.replace(cmp_stmt);
//             }
//             Nodecl::CompoundStatement cmp_stmt = stmt.as<Nodecl::CompoundStatement>();
//             Nodecl::List cmp_stmts = cmp_stmt.get_statements().as<Nodecl::List>();
//             cmp_stmts.prepend(create_call_to_tracepoint(/*init*/true, /*tracekind*/TraceTaskpart));
//             cmp_stmts.append(create_call_to_tracepoint(/*init*/false, /*tracekind*/TraceTaskpart));
//          }

        // 4.- Traverse the statements of the parallel construct
        walk(n.get_statements());
    }

    void GOMPtransformationsBeforeAnalysis::visit(const Nodecl::OpenMP::Sections& n)
    {
        internal_error("OpenMP sections are not supported.\n", 0);
    }

    void GOMPtransformationsBeforeAnalysis::visit(const Nodecl::OpenMP::Single& n)
    {
        In_single = true;
        if (_instrumentation == All)
        {
            // 1.- Insert taskpart runtime calls at the beginning and the end of the single list of statements
            Nodecl::Context ctx = n.get_statements().as<Nodecl::List>().front().as<Nodecl::Context>();
            Nodecl::List cmp_stmt = ctx.get_in_context().as<Nodecl::List>();
            Nodecl::List stmts;
            // If the statements of the task are inside a CompoundStatement,
            //    just insert the calls at the beginning and the end of the list
            // Otherwise, we have to wrap the unique statement of the task in a
            //    CompoundStatement before inserting the calls.
            if (cmp_stmt[0].is<Nodecl::CompoundStatement>())
            {
                stmts = cmp_stmt[0].as<Nodecl::CompoundStatement>().get_statements().as<Nodecl::List>();
            }
            else
            {
                stmts = cmp_stmt.shallow_copy().as<Nodecl::List>();
                cmp_stmt.replace(Nodecl::CompoundStatement::make(stmts, Nodecl::NodeclBase::null()));
            }
            if (!stmts.empty())
            {
                stmts.prepend(create_call_to_tracepoint(/*init*/true, /*tracekind*/TraceTaskpart));
                stmts.append(create_call_to_tracepoint(/*init*/false, /*tracekind*/TraceTaskpart));
            }
        }

        // 2.- Visit single inner statements
        walk(n.get_statements());
        In_single = false;
    }

    void GOMPtransformationsBeforeAnalysis::visit(const Nodecl::OpenMP::TargetData& n)
    {
        internal_error("OpenMP target data is not supported.\n", 0);
    }

    void GOMPtransformationsBeforeAnalysis::visit(const Nodecl::OpenMP::TargetUpdate& n)
    {
        internal_error("OpenMP target update is not supported.\n", 0);
    }

    // Add the id clause
    void GOMPtransformationsBeforeAnalysis::visit(const Nodecl::OpenMP::Task& n)
    {
        if (In_single == false)
        {
            WARNING_MESSAGE("Task outside a Single construct. This is not supported in Psocrates environment.\n", 0);
        }

        // 1.- Add the id clause to the task
        Nodecl::IntegerLiteral t_id = Nodecl::IntegerLiteral::make(
                Type::get_unsigned_int_type(),
                const_value_get_integer(++task_id, /*num_bytes*/4, /*sign*/0));
        Nodecl::OpenMP::TaskId task_id_clause = Nodecl::OpenMP::TaskId::make(t_id);
        n.get_environment().as<Nodecl::List>().append(task_id_clause);

        // 2.- Insert tracing calls before and after the parallel region if runtime instrumentation is requested
        if (_instrumentation == All)
        {
            n.prepend_sibling(create_call_to_tracepoint(/*init*/false, /*tracekind*/TraceTaskpart));
            n.prepend_sibling(create_call_to_tracepoint(/*init*/true, /*tracekind*/TraceRuntime));
            n.append_sibling(create_call_to_tracepoint(/*init*/true, /*tracekind*/TraceTaskpart));
            n.append_sibling(create_call_to_tracepoint(/*init*/false, /*tracekind*/TraceRuntime));
        }

        if (_instrumentation == Tasks || _instrumentation == All)
        {
            // 3.- Insert tracing calls at the beginning and the end of the task region
            Nodecl::Context ctx = n.get_statements().as<Nodecl::List>().front().as<Nodecl::Context>();
            Nodecl::NodeclBase stmt = ctx.get_in_context().as<Nodecl::List>().front();
            // Case: there is only one statement associated with the directive
            //       and no CompoundStatement wraps it
            // Behavior: Insert a CompoundStatement before inserting the calls to the instrumentation
            if (!stmt.is<Nodecl::CompoundStatement>())
            {
                Nodecl::CompoundStatement cmp_stmt =
                        Nodecl::CompoundStatement::make(Nodecl::List::make(stmt.shallow_copy()),
                                                        Nodecl::NodeclBase::null());
                stmt.replace(cmp_stmt);
            }
            Nodecl::CompoundStatement cmp_stmt = stmt.as<Nodecl::CompoundStatement>();
            Nodecl::List cmp_stmts = cmp_stmt.get_statements().as<Nodecl::List>();
            cmp_stmts.prepend(create_call_to_tracepoint(/*init*/true, /*tracekind*/TraceTask));
            cmp_stmts.append(create_call_to_tracepoint(/*init*/false, /*tracekind*/TraceTask));
        }

        // 2.- Traverse the code of the task
        walk(n.get_statements());
    }

    void GOMPtransformationsBeforeAnalysis::visit(const Nodecl::OpenMP::TaskwaitDeep& n)
    {
        // 1.- Insert tracing calls before and after the taskwait
        if (_instrumentation == All)
        {
            n.prepend_sibling(create_call_to_tracepoint(/*init*/false, /*tracekind*/TraceTaskpart));
            n.append_sibling(create_call_to_tracepoint(/*init*/true, /*tracekind*/TraceTaskpart));
        }
    }

    void GOMPtransformationsBeforeAnalysis::visit(const Nodecl::OpenMP::TaskwaitShallow& n)
    {
        // 1.- Insert tracing calls before and after the taskwait
        if (_instrumentation == All)
        {
            n.prepend_sibling(create_call_to_tracepoint(/*init*/false, /*tracekind*/TraceTaskpart));
            n.append_sibling(create_call_to_tracepoint(/*init*/true, /*tracekind*/TraceTaskpart));
        }
    }

    void GOMPtransformationsBeforeAnalysis::visit(const Nodecl::OpenMP::Taskyield& n)
    {
        internal_error("OpenMP task yield is not supported.\n", 0);
    }


    /* ********************** END Transformations previous to Analysis ********************** */


    /* ************************ Transformations posterior to Analysis *********************** */

    void GOMPtransformationsAfterAnalysis::unhandled_node(const Nodecl::NodeclBase& n)
    {
        WARNING_MESSAGE("Unhandled node of type '%s' while libGOMP transformations.\n",
                        ast_print_node_type(n.get_kind()));
    }

    // Add GOMP inc/push/pop runtime calls
    void GOMPtransformationsAfterAnalysis::visit(const Nodecl::ForStatement& n)
    {
        // 1.- We only include calls to the GOMP library if the loop contains some task
        TaskVisitor tv;
        tv.walk(n);
        if (!tv.contains_task())
            return;

        // 2.- Visit inner statements to apply transformation from inner to outer scopes
        Nodecl::NodeclBase for_stmt = n.get_statement();
        walk(for_stmt);

        // 3.- Insert call to GOMP_inc_loop()
        Nodecl::ExpressionStatement gomp_inc_stmt = create_func_call_stmt(
                /*func name*/ "GOMP_inc_loop",
                /*param names*/ ObjectList<std::string>(),
                /*param type*/ ObjectList<Type>(),
                /*arguments*/ Nodecl::List());
        Nodecl::Context for_stmt_ctx = for_stmt.as<Nodecl::List>()[0].as<Nodecl::Context>();
        // Note: we always have a CompoundStatement here because
        // either the input source has it
        // or the frontend introduces it
        Nodecl::CompoundStatement for_stmt_comp = for_stmt_ctx.get_in_context().as<Nodecl::List>()[0].as<Nodecl::CompoundStatement>();
        Nodecl::List for_stmts = for_stmt_comp.get_statements().as<Nodecl::List>();
        if (for_stmts.is_null())
        {
            Nodecl::List new_for_stmts = Nodecl::List::make(gomp_inc_stmt);
            for_stmt_ctx.set_in_context(new_for_stmts);
        }
        else
        {
            for_stmts.insert(for_stmts.begin(), gomp_inc_stmt);
        }

        // 4.- Create the list of statements that will be contained in the new CompoundStatement
        //     that will replace the current ForStatement
        Nodecl::ExpressionStatement gomp_push_stmt = create_func_call_stmt(
                /*func name*/ "GOMP_push_loop",
                /*param names*/ ObjectList<std::string>(),
                /*param type*/ ObjectList<Type>(),
                /*arguments*/ Nodecl::List());
        Nodecl::ExpressionStatement gomp_pop_stmt = create_func_call_stmt(
                /*func name*/ "GOMP_pop_loop",
                /*param names*/ ObjectList<std::string>(),
                /*param type*/ ObjectList<Type>(),
                /*arguments*/ Nodecl::List());
        Nodecl::List compound_list = Nodecl::List::make(gomp_push_stmt, n.shallow_copy(), gomp_pop_stmt);
        Nodecl::CompoundStatement compound = Nodecl::CompoundStatement::make(
                compound_list,
                Nodecl::NodeclBase::null());

        // 5.- Replace the ForStatement with the new CompoundStatement
        n.replace(compound);
    }

    // Remove dependency clauses
    void GOMPtransformationsAfterAnalysis::visit(const Nodecl::OpenMP::Task& n)
    {
        // 1.- Remove the dependency clauses
        TL::ObjectList<Nodecl::NodeclBase> deps_to_remove;
        Nodecl::List environ = n.get_environment().as<Nodecl::List>();
        for (Nodecl::List::iterator it = environ.begin(); it != environ.end(); ++it)
        {
            if (it->is<Nodecl::OpenMP::DepIn>()
                || it->is<Nodecl::OpenMP::DepOut>()
                || it->is<Nodecl::OpenMP::DepInout>())
            {
                deps_to_remove.append(*it);
            }
        }
        for (TL::ObjectList<Nodecl::NodeclBase>::iterator it = deps_to_remove.begin();
             it != deps_to_remove.end(); ++it)
        {
            Nodecl::Utils::remove_from_enclosing_list(*it);
        }
    }

    /* ********************** END Transformations posterior to Analysis ********************* */

    /* ****************************** END GOMP Transformations ****************************** */

    // ***************** END code transformations for compiling with GCC ***************** //
    // *********************************************************************************** //



    // *********************************************************************************** //
    // *********************** Phase for Psocrates transformations *********************** //

    PsocratesPhase::PsocratesPhase()
            : _tdg_enabled_str(""), _tdg_enabled(false),
              _taskparts_enabled_str(""), _taskparts_enabled(false),
              _ompss_mode_str(""), _ompss_mode_enabled(false),
              _functions_str(""), _call_graph_str(""), _call_graph_enabled(true),
              _instrumentation_str(""), _instrumentation(None)
    {
        set_phase_name("Phase for P-Socrates project transformations");

        register_parameter("tdg_enabled",
                           "If set to '1' enables tdg analysis, otherwise it is disabled",
                           _tdg_enabled_str,
                           "0").connect(std::bind(&PsocratesPhase::set_tdg, this, std::placeholders::_1));

        register_parameter("taskparts_enabled",
                           "If set to '1' enables taskparts analysis, otherwise it is disabled",
                           _taskparts_enabled_str,
                           "0").connect(std::bind(&PsocratesPhase::set_taskparts, this, std::placeholders::_1));

        register_parameter("ompss_mode",
                           "Enables OmpSs semantics instead of OpenMP semantics",
                           _ompss_mode_str,
                           "0").connect(std::bind(&PsocratesPhase::set_ompss_mode, this, std::placeholders::_1));

        register_parameter("json_function",
                           "Points out the function that has to be analyzed",
                           _functions_str,
                           "").connect(std::bind(&PsocratesPhase::set_json_functions, this, std::placeholders::_1));

        register_parameter("functions",
                           "Points out the function that has to be analyzed",
                           _functions_str,
                           "").connect(std::bind(&PsocratesPhase::set_functions, this, std::placeholders::_1));

        register_parameter("call_graph",
                           "If set to '1' enbles analyzing the call graph of all functions specified in parameter 'functions'",
                           _call_graph_str,
                           "1").connect(std::bind(&PsocratesPhase::set_call_graph, this, std::placeholders::_1));

        register_parameter("instrumentation",
                           "Values accepted are: 'none', 'tasks' and 'all'.\n"
                           "  - If set to 'none', no instrumentation is inserted.\n"
                           "  - If set to 'tasks', only code within implicit tasks is instrumented.\n"
                           "  - If set to 'all', both code in implicit tasks and runtime overhead in implicit tasks are implemented.",
                           _instrumentation_str,
                           "0").connect(std::bind(&PsocratesPhase::set_instrumentation, this, std::placeholders::_1));
    }

    void PsocratesPhase::run(TL::DTO& dto)
    {
        AnalysisBase analysis(_ompss_mode_enabled);

        Nodecl::NodeclBase ast = *std::static_pointer_cast<Nodecl::NodeclBase>(dto["nodecl"]);

        // 1.- LibGOMP transformations previous to analysis
        GOMPtransformationsBeforeAnalysis gompt_pre(_instrumentation);
        gompt_pre.walk(ast);

        // 2.- Static generation of the Task Dependency Graph
        ObjectList<TaskDependencyGraph*> tdgs;

        // _functions_str is a comma-separated list of function names
        // Transform it into a set of strings
        std::set<std::string> functions;
        tokenizer(_functions_str, functions);

        // debugging purposes
        if (debug_options.print_pcfg ||
            debug_options.print_pcfg_w_context ||
            debug_options.print_pcfg_w_analysis ||
            debug_options.print_pcfg_full)
        {
            if (VERBOSE)
                std::cerr << "==========  Generating and printing PCFG to dot file  ==========" << std::endl;
            analysis.parallel_control_flow_graph(ast, functions, _call_graph_enabled);
            ObjectList<ExtensibleGraph*> pcfgs = analysis.get_pcfgs();
            for (ObjectList<ExtensibleGraph*>::iterator it = pcfgs.begin(); it != pcfgs.end(); ++it)
                analysis.print_pcfg((*it)->get_name());
            if (VERBOSE)
                std::cerr << "========  Generating and printing PCFG to dot file done  =======" << std::endl;
        }

        if (_tdg_enabled)
        {
            if (VERBOSE)
                std::cerr << "====================  Testing TDG creation  ====================" << std::endl;
            tdgs = analysis.task_dependency_graph(ast, functions, _call_graph_enabled, _taskparts_enabled, /*psocrates*/true);
            if (VERBOSE)
                std::cerr << "==================  Testing TDG creation done  =================" << std::endl;
        }

        if (debug_options.print_tdg)
        {
            if (VERBOSE)
                std::cerr << "==================  Printing TDG to dot file  =================" << std::endl;
            for (ObjectList<TaskDependencyGraph*>::iterator it = tdgs.begin(); it != tdgs.end(); ++it)
                analysis.print_tdg((*it)->get_name());
            if (VERBOSE)
                std::cerr << "===============  Printing TDG to dot file done  ===============" << std::endl;
        }

        if (debug_options.tdg_to_json)
        {
            if (VERBOSE)
                std::cerr << "==================  Printing TDG to json file  ================" << std::endl;
            analysis.tdgs_to_json(tdgs);
            if (VERBOSE)
                std::cerr << "===============  Printing TDG to json file done  ==============" << std::endl;
        }

        // 3.- LibGOMP transformations posterior to analysis
        GOMPtransformationsAfterAnalysis gompt_post;
        gompt_post.walk(ast);
    }

    void PsocratesPhase::set_tdg(const std::string& tdg_enabled_str)
    {
        if (tdg_enabled_str == "1")
            _tdg_enabled = true;
    }

    void PsocratesPhase::set_taskparts(const std::string& taskparts_enabled_str)
    {
        if (taskparts_enabled_str == "1")
        {
            _taskparts_enabled = true;
            WARNING_MESSAGE("Taskparts analysis is not tested", "");
        }
    }

    void PsocratesPhase::set_ompss_mode(const std::string& ompss_mode_str)
    {
        if (ompss_mode_str == "1")
            _ompss_mode_enabled = true;
    }

    void PsocratesPhase::set_json_functions(const std::string& json_functions_str)
    {
        WARNING_MESSAGE("Parameter 'json_function' is DEPRECATED. Use 'functions' instead.", 0);
        set_functions(json_functions_str);
    }

    void PsocratesPhase::set_functions(const std::string& functions_str)
    {
        if (functions_str != "")
            _functions_str = functions_str;
    }

    void PsocratesPhase::set_call_graph(const std::string& call_graph_enabled_str)
    {
        if (call_graph_enabled_str == "0")
            _call_graph_enabled = false;
    }

    void PsocratesPhase::set_instrumentation(const std::string& instrumentation_str)
    {
        if (instrumentation_str == "none")
        {
            _instrumentation = None;
        }
        else if (instrumentation_str == "tasks")
        {
            _instrumentation = Tasks;
        }
        else if (instrumentation_str == "all")
        {
            _instrumentation = All;
        }
        else
        {
            WARNING_MESSAGE("Unexpected value '%s' in parameter 'instrumentation'. "
                            "Only 'none', 'tasks' and 'all' are accepted.\n",
                            instrumentation_str.c_str());
        }
    }

    // ********************* END Phase for Psocrates transformations ********************* //
    // *********************************************************************************** //
}
}

EXPORT_PHASE(TL::Analysis::PsocratesPhase);
