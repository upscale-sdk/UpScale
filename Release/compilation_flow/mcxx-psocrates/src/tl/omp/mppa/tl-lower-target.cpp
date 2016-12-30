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
#include "tl-lowering-visitor.hpp"

#include "tl-target-properties.hpp"
#include "tl-counters.hpp"

#include "tl-nodecl-utils.hpp"
#include "tl-symbol-utils.hpp"

#include "cxx-cexpr.h"

namespace TL { namespace MPPA {


void LoweringVisitor::visit(const Nodecl::OpenMP::Target& construct)
{
    Nodecl::List environment = construct.get_environment().as<Nodecl::List>();

    TargetProperties target_properties =
        TargetProperties::gather_target_properties(_lowering, construct);

    TL::Type data_struct;
    Nodecl::NodeclBase arg_size;
    target_properties.create_target_parameters_structure(
            data_struct,
            arg_size);

    TL::Scope sc = ::new_block_context(construct.retrieve_context().get_decl_context());

    std::string capture_var_name;
    {
        TL::Counter &counter = TL::CounterManager::get_counter("mppa");
        std::stringstream ss;
        ss << "mppa_data_" << (int)counter;
        counter++;
        capture_var_name = ss.str();
    }

    TL::Symbol capture_var = sc.new_symbol(capture_var_name);
    capture_var.get_internal_symbol()->kind = SK_VARIABLE;
    capture_var.set_type(data_struct.get_pointer_to());
    symbol_entity_specs_set_is_user_declared(
            capture_var.get_internal_symbol(),
            1);

    Nodecl::List task_body;

    Nodecl::SourceComment data_comment =
        Nodecl::SourceComment::make("Offloaded data structure is freed in the Runtime.");
    task_body.prepend(data_comment);

    Symbol malloc_sym = Scope::get_global_scope().get_symbol_from_name("malloc");
    if (!malloc_sym.is_valid())
    {
        malloc_sym = SymbolUtils::new_function_symbol(
            /*scope*/ Scope::get_global_scope(),
            /*function name*/ "malloc",
            /*return symbol name*/ "",
            /*return type*/ TL::Type::get_void_type().get_pointer_to(),
            /*parameter names*/ ObjectList<std::string>(1, "size"),
            /*parameter types*/ ObjectList<Type>(1, Type::get_size_t_type())
        );
        // Make it external
        symbol_entity_specs_set_is_extern(malloc_sym.get_internal_symbol(), 1);
        symbol_entity_specs_set_is_static(malloc_sym.get_internal_symbol(), 0);
    }
    Nodecl::Sizeof malloc_arg =
        Nodecl::Sizeof::make(
            Nodecl::Type::make(data_struct),
            Nodecl::NodeclBase::null(),
            Type::get_size_t_type()
        );
    Nodecl::Conversion malloc_call =
        Nodecl::Conversion::make(
            Nodecl::FunctionCall::make(
                malloc_sym.make_nodecl(/* set_ref */ true),
                Nodecl::List::make(malloc_arg),
                /* alternate-name */ Nodecl::NodeclBase::null(),
                /* function-form */ Nodecl::NodeclBase::null(),
                Type::get_void_type().get_pointer_to()
            ),
            data_struct.get_pointer_to()
        );
    Nodecl::ExpressionStatement data_assignment =
        Nodecl::ExpressionStatement::make(
            Nodecl::Assignment::make(
                capture_var.make_nodecl(/* set_ref */ true),
                malloc_call,
                data_struct.get_pointer_to()
            )
        );
    task_body.append(data_assignment);

    Nodecl::NodeclBase capture_data = target_properties.capture_data(capture_var);
    task_body.append(capture_data);

    TL::Symbol gomp_target_sym = TL::Scope::get_global_scope().get_symbol_from_name("GOMP_target");
    ERROR_CONDITION(!gomp_target_sym.is_valid(), "GOMP_target not found", 0);

    Nodecl::NodeclBase device_id;
    if (target_properties.device_id.is_null())
    {
       device_id = const_value_to_nodecl(
           const_value_get_signed_int(1));
    }
    else
    {
        device_id = target_properties.device_id;
    }

    int cluster_function_id;
    {
        TL::Counter &counter = TL::CounterManager::get_counter("mppa-cluster-function-id");
        cluster_function_id = counter;
        counter++;
    }

    Nodecl::NodeclBase cluster_function_ref =
        Nodecl::Conversion::make(
                const_value_to_nodecl(
                    const_value_get_signed_int(cluster_function_id)),
                TL::Type::get_void_type().get_pointer_to());
    cluster_function_ref.set_text("C");

    Nodecl::NodeclBase data_struct_cast =
        Nodecl::Conversion::make(
                capture_var.make_nodecl(/* set_ref */ true),
                TL::Type(::get_void_type()).get_pointer_to()
        );
    data_struct_cast.set_text("C");

    Nodecl::NodeclBase host_function_code =
        target_properties.generate_host_function();

    Nodecl::Utils::prepend_to_enclosing_top_level_location(construct, host_function_code);

    // void (*)(void*)
    TL::Type host_function_param_type = TL::Type::get_void_type().get_function_returning(
            TL::ObjectList<TL::Type>(1, TL::Type::get_void_type().get_pointer_to()))
        .get_pointer_to();

    Nodecl::NodeclBase host_function_ref =
        Nodecl::Conversion::make(
                host_function_code.get_symbol().make_nodecl(/* set_ref */ true),
                host_function_param_type);
    host_function_ref.set_text("C");

    // Creating the priority_id
    Nodecl::NodeclBase priority_id = target_properties.priority_id;
    if (priority_id.is_null())
        priority_id = const_value_to_nodecl(const_value_get_signed_int(0));

    Nodecl::NodeclBase bit_mask = const_value_to_nodecl(const_value_get_unsigned_int(65535));

    Nodecl::NodeclBase offload_call =
        Nodecl::ExpressionStatement::make(
                Nodecl::FunctionCall::make(
                    gomp_target_sym.make_nodecl(/* set_lvalue */ true),
                    Nodecl::List::make(
                        device_id,
                        host_function_ref,
                        cluster_function_ref,
                        data_struct_cast,
                        priority_id,
                        bit_mask
                        ),
                    Nodecl::NodeclBase::null(),
                    Nodecl::NodeclBase::null(),
                    get_void_type()));

    task_body.append(offload_call);
    if (target_properties.undeferred)
    {
        TL::Symbol gomp_target_wait_sym = TL::Scope::get_global_scope().get_symbol_from_name("GOMP_target_wait");
        ERROR_CONDITION(!gomp_target_wait_sym.is_valid(), "GOMP_target_wait not found", 0);

        Nodecl::NodeclBase funct_call = Nodecl::FunctionCall::make(
                gomp_target_wait_sym.make_nodecl(/* set_lvalue */ true),
                Nodecl::List::make(
                    device_id.shallow_copy(),
                    data_struct_cast.shallow_copy(),
                    priority_id.shallow_copy()),
                Nodecl::NodeclBase::null(),
                Nodecl::NodeclBase::null(),
                get_void_type());

        Nodecl::NodeclBase expr_stmt = Nodecl::ExpressionStatement::make(funct_call);
        task_body.append(expr_stmt);
    }

    Nodecl::NodeclBase task_code =
        Nodecl::List::make(
                Nodecl::Context::make(
                    Nodecl::List::make(
                        Nodecl::CompoundStatement::make(
                            task_body,
                            /* finalizer */ Nodecl::NodeclBase::null(),
                            construct.get_locus())),
                    sc));

    TL::ObjectList<Nodecl::NodeclBase> filtered_environment;


    for (Nodecl::List::iterator it = environment.begin();
            it != environment.end();
            it++)
    {
        if (it->is<Nodecl::OpenMP::MapTo>()
                || it->is<Nodecl::OpenMP::MapFrom>()
                || it->is<Nodecl::OpenMP::MapToFrom>()
                || it->is<Nodecl::OpenMP::MapAlloc>()
                || it->is<Nodecl::OpenMP::TargetTaskUndeferred>()
                || it->is<Nodecl::OpenMP::Device>()
                || it->is<Nodecl::OpenMP::PriorityId>()
                )
            continue;

        filtered_environment.append(it->shallow_copy());
    }


    Nodecl::NodeclBase task_construct =
        Nodecl::OpenMP::Task::make(
                Nodecl::List::make(filtered_environment),
                task_code,
                construct.get_locus());

    construct.prepend_sibling(task_construct);

    Nodecl::NodeclBase cluster_function_code = target_properties.generate_cluster_function();
    _lowering->register_cluster_function(cluster_function_code);

    Nodecl::Utils::remove_from_enclosing_list(construct);
}

} }
