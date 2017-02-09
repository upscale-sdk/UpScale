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
--------------------------------------------------------------------*/



#ifndef TL_PSOCRATES_PHASE_HPP
#define TL_PSOCRATES_PHASE_HPP

#include "tl-compilerphase.hpp"
#include "tl-nodecl-visitor.hpp"

namespace TL {
namespace Analysis {

    // TODO Task Parts support has been temporarily removed
    // Add a task_part_id call before each task scheduling point
    //       - the point immediately following the generation of an explicit task
    //       - after the point of completion of a task region
    //       - [TODO] in a taskyield region                     == not supported (like taskwait) ==
    //       - in a taskwait region
    //       - [TODO] at the end of a taskgroup region          == not supported ==
    //       - in an implicit and explicit barrier region
    //       - the point immediately following the generation of a target region
    //       - at the beginning and end of a target data region == not supported ==
    //       - [TODO] in a target update region                 == not supported ==

    enum Instrumentation {
        None,
        Tasks,
        All     // Includes Runtime and Tasks and Taskparts
    };

    // This phase prepares the code to be lowered for the Psocrates Phase
    // It performs transformations that have to be done before analysis:
    // - Add a task_id clause to each task / target construct
    // It also checks we are not using any non-accepted directive
    class LIBTL_CLASS GOMPtransformationsBeforeAnalysis : public Nodecl::ExhaustiveVisitor<void>
    {
    private:
        Instrumentation _instrumentation;

    public:
        GOMPtransformationsBeforeAnalysis(Instrumentation instrumentation);

        Ret unhandled_node(const Nodecl::NodeclBase& n);

        Ret visit(const Nodecl::FunctionCode& n);
        Ret visit(const Nodecl::OpenMP::For& n);
        Ret visit(const Nodecl::OpenMP::Master& n);
        Ret visit(const Nodecl::OpenMP::Parallel& n);
        Ret visit(const Nodecl::OpenMP::Sections& n);
        Ret visit(const Nodecl::OpenMP::Single& n);
        Ret visit(const Nodecl::OpenMP::TargetData& n);
        Ret visit(const Nodecl::OpenMP::TargetUpdate& n);
        Ret visit(const Nodecl::OpenMP::Task& n);
        Ret visit(const Nodecl::OpenMP::TaskwaitDeep& n);
        Ret visit(const Nodecl::OpenMP::TaskwaitShallow& n);
        Ret visit(const Nodecl::OpenMP::Taskyield& n);
    };

    // This phase prepares the code to be lowered for the Psocrates Phase
    // It performs transformations that have to be done after analysis:
    // - Add calls to GOMP_push_loop, GOMP_pop_loop and GOMP_inc_loop
    // - Remove task dependency clauses
    class LIBTL_CLASS GOMPtransformationsAfterAnalysis : public Nodecl::ExhaustiveVisitor<void>
    {
    private:

    public:
        Ret unhandled_node(const Nodecl::NodeclBase& n);

        Ret visit(const Nodecl::ForStatement& n);
        Ret visit(const Nodecl::OpenMP::Task& n);
    };

    //! Phase that allows doing the necessary transformation for the P-Socrates project
    class LIBTL_CLASS PsocratesPhase : public CompilerPhase
    {
    private:
        std::string _tdg_enabled_str;
        bool _tdg_enabled;
        void set_tdg(const std::string& tdg_enabled_str);

        std::string _taskparts_enabled_str;
        bool _taskparts_enabled;
        void set_taskparts(const std::string& taskparts_enabled_str);

        std::string _ompss_mode_str;
        bool _ompss_mode_enabled;
        void set_ompss_mode(const std::string& ompss_mode_str);

        std::string _functions_str;
        void set_functions(const std::string& functions_str);
        void set_json_functions(const std::string& functions_str) DEPRECATED;

        std::string _call_graph_str;
        bool _call_graph_enabled;
        void set_call_graph(const std::string& call_graph_str);

        std::string _instrumentation_str;
        Instrumentation _instrumentation;
        void set_instrumentation(const std::string& instrumentation_str);

    public:
        //! Constructor of this phase
        PsocratesPhase();

        //!Entry point of the phase
        virtual void run(TL::DTO& dto);

        virtual ~PsocratesPhase() {};
    };
}
}

#endif  // TL_PSOCRATES_PHASE_HPP
