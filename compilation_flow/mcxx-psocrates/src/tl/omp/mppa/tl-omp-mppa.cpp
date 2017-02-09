/*--------------------------------------------------------------------
  (C) Copyright 2006-2015 Barcelona Supercomputing Center
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


#include "tl-omp-mppa.hpp"
#include "tl-declare-target-visitor.hpp"
#include "tl-lowering-visitor.hpp"
#include "tl-compilerpipeline.hpp"
#include "tl-symbol-utils.hpp"
#include "cxx-driver-utils.h"
#include "codegen-phase.hpp"
#include <iostream>
#include "cxx-cexpr.h"
#include <errno.h>

namespace TL { namespace MPPA {

    Lowering::Lowering()
    {
        set_phase_name("GOMP MPPA-I/O lowering");
        set_phase_description("This phase lowers from Mercurium parallel IR into calls to the "
                "GNU Offloading and Multi Processing Library for the MPPA-I/O nodes");

        register_parameter("omp_dry_run",
                "Disables OpenMP transformation",
                _openmp_dry_run,
                "0");
    }

    void Lowering::pre_run(DTO& dto)
    {
    }

    void Lowering::run(DTO& dto)
    {
        if (_openmp_dry_run != "0")
        {
            std::cerr << "Not running MPPA-I/O GOMP phase (by request)" << std::endl;
            return;
        }

        std::cerr << "MPPA-I/O GOMP phase" << std::endl;

        
        Nodecl::NodeclBase n = *std::static_pointer_cast<Nodecl::NodeclBase>(dto["nodecl"]);

        DeclareTargetVisitor declare_target_visitor(this);
        declare_target_visitor.walk(n);

        TL::ObjectList<TL::Symbol> declare_target_functions = declare_target_visitor.get_declare_target_functions();

        Nodecl::Utils::SimpleSymbolMap current_cluster_mapping = this->get_cluster_mapping();
        for (TL::ObjectList<TL::Symbol>::iterator it = declare_target_functions.begin();
                it != declare_target_functions.end();
                it++)
        {
            TL::Symbol &sym(*it);

            Nodecl::NodeclBase target_function_clone = 
                Nodecl::Utils::deep_copy(sym.get_function_code(), sym.get_scope(), current_cluster_mapping);
            this->register_cluster_declaration(target_function_clone);
        }

        LoweringVisitor lowering_visitor(this);
        lowering_visitor.walk(n);
    }

    void Lowering::phase_cleanup(DTO&)
    {
    }

    void Lowering::phase_cleanup_end_of_pipeline(DTO&)
    {
        emit_cluster_code();
    }

    void Lowering::register_cluster_declaration(Nodecl::NodeclBase node)
    {
        cluster_code.append(node);
    }

    void Lowering::register_cluster_function(Nodecl::NodeclBase node)
    {
        register_cluster_declaration(node);
        cluster_functions.append(node.as<Nodecl::FunctionCode>().get_symbol());
    }

    void Lowering::register_cluster_mapping(TL::Symbol sym, TL::Symbol new_sym)
    {
        cluster_mapping.add_map(sym, new_sym);
    }

    Nodecl::Utils::SimpleSymbolMap Lowering::get_cluster_mapping()
    {
        return cluster_mapping;
    }

    void Lowering::emit_cluster_code()
    {
        if (cluster_code.is_null())
            return;

        std::cerr << "Emitting MPPA Cluster code" << std::endl;

        compute_mmpa_architecture();

        // omp functions
        TL::Symbol omp_functions = TL::Scope::get_global_scope().new_symbol("omp_functions");
        omp_functions.get_internal_symbol()->kind = SK_VARIABLE;
        symbol_entity_specs_set_is_user_declared(omp_functions.get_internal_symbol(), 1);

        TL::Symbol mppa_omp_datum = TL::Scope::get_global_scope().get_symbol_from_name("omp_param_t");
        ERROR_CONDITION(!mppa_omp_datum.is_valid(), "Symbol  omp_param_t not found", 0);

        TL::Type function_array_type = TL::Type::get_void_type().get_function_returning(
                TL::ObjectList<TL::Type>(1, mppa_omp_datum.get_user_defined_type().get_pointer_to()))
            .get_pointer_to().get_array_to(
                    const_value_to_nodecl(const_value_get_signed_int(cluster_functions.size())),
                    TL::Scope::get_global_scope());

        omp_functions.get_internal_symbol()->type_information = function_array_type.get_internal_type();

        TL::ObjectList<Nodecl::NodeclBase> braced_items;
        for (TL::ObjectList<TL::Symbol>::iterator it = cluster_functions.begin();
                it != cluster_functions.end();
                it++)
        {
            int idx = it - cluster_functions.begin();
            braced_items.append(
                    Nodecl::IndexDesignator::make(
                        const_value_to_nodecl(
                            const_value_get_signed_int(idx)),
                        it->make_nodecl(/* set_ref */ true),
                        it->get_type().get_lvalue_reference_to()));
        }

        omp_functions.set_value(
                Nodecl::StructuredValue::make(
                    Nodecl::List::make(braced_items),
                    Nodecl::StructuredValueBracedImplicit::make(),
                    omp_functions.get_type()));

        cluster_code.append(Nodecl::ObjectInit::make(omp_functions));

        // Create main
        TL::ObjectList<std::string> main_arg_names(2);
        main_arg_names[0] = "argc";
        main_arg_names[1] = "argv";

        TL::ObjectList<TL::Type> main_arg_types(2);
        main_arg_types[0] = TL::Type::get_int_type();
        main_arg_types[1] = TL::Type::get_char_type().get_pointer_to().get_pointer_to(); // char**

        TL::Symbol cluster_main = SymbolUtils::new_function_symbol(
                TL::Scope::get_global_scope(),
                "main",
                TL::Type::get_int_type(),
                main_arg_names,
                main_arg_types);
        
        // New_function_symbol chooses to make it static by default, which is
        // in general OK except for main
        symbol_entity_specs_set_is_static(cluster_main.get_internal_symbol(), 0);

        Nodecl::NodeclBase cluster_main_function_code, cluster_main_empty_stmt;
        SymbolUtils::build_empty_body_for_function(
                cluster_main,
                cluster_main_function_code,
                cluster_main_empty_stmt);
        // FIXME: what are we supposed to do in this main???

        cluster_code.append(cluster_main_function_code);

        std::string original_filename = TL::CompilationProcess::get_current_file().get_filename();
        std::string cluster_code_filename = "cluster_" + original_filename  + ".c";

        FILE* cluster_code_file = fopen(cluster_code_filename.c_str(), "w");
        if (cluster_code_file == NULL)
        {
            fatal_error("%s: error: cannot open file '%s'. %s\n",
                    original_filename.c_str(),
                    cluster_code_filename.c_str(),
                    strerror(errno));
        }

        Codegen::CodegenPhase* phase = reinterpret_cast<Codegen::CodegenPhase*>(CURRENT_CONFIGURATION->codegen_phase);
        phase->codegen_top_level(cluster_code, cluster_code_file, cluster_code_filename);
        fclose(cluster_code_file);
        cluster_code = Nodecl::List();

        TL::CompilationProcess::add_file(cluster_code_filename, "psocratescc-cluster", /* tag */ 1);
        ::mark_file_for_cleanup(cluster_code_filename.c_str());
    }

    void Lowering::compute_mmpa_architecture()
    {
        TL::Symbol erika_version_sym = TL::Scope::get_global_scope().get_symbol_from_name("erika_enterprise_version");
        ERROR_CONDITION(!erika_version_sym.is_valid(), "erika_enterprise_version not found", 0);

        Nodecl::NodeclBase erika_value = erika_version_sym.get_value();
        ERROR_CONDITION(erika_value.is_null(), "erika_enterprise_version is not initialized", 0);
        ERROR_CONDITION(!erika_value.is_constant(), "erika_enterprise_version should be initialized with a constant value", 0);

       int erika_version = const_value_cast_to_signed_int(erika_value.get_constant());
       _arch = (mppa_arch_t) erika_version;
    }

    mppa_arch_t Lowering::get_mppa_arch() const
    {
        return _arch;
    }

} }

EXPORT_PHASE(TL::MPPA::Lowering)
