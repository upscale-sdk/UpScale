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


#include "tl-target-properties.hpp"
#include "tl-omp-mppa.hpp"
#include "tl-nodecl-visitor.hpp"
#include "tl-nodecl-utils.hpp"
#include "tl-omp-core.hpp"
#include "tl-lowering-visitor.hpp"
#include "tl-counters.hpp"
#include "tl-symbol-utils.hpp"
#include "tl-datareference.hpp"

#include "cxx-cexpr.h"

#include <set>
#include <stdio.h>

namespace TL { namespace MPPA {

    struct TargetPropertiesVisitor : public Nodecl::ExhaustiveVisitor<void>
    {
        private:
            TargetProperties& _target_properties;

            static TL::ObjectList<TL::DataReference> to_symbols(Nodecl::NodeclBase l)
            {
                ERROR_CONDITION(!l.is_null() && !l.is<Nodecl::List>(), "Invalid node", 0);
                TL::ObjectList<Nodecl::NodeclBase> obj_list = l.as<Nodecl::List>().to_object_list();

                TL::ObjectList<TL::DataReference> result;
                for (TL::ObjectList<Nodecl::NodeclBase>::iterator it = obj_list.begin();
                        it != obj_list.end();
                        it++)
                {
                    TL::DataReference dr(*it);
                    ERROR_CONDITION(!dr.is_valid(), "Invalid data reference '%s'\n", it->prettyprint().c_str());
                    result.append(dr);
                }
                return result;
            }

        public:
            TargetPropertiesVisitor(TargetProperties& target_properties)
                : _target_properties(target_properties) { }

            virtual void visit(const Nodecl::OpenMP::MapFrom& n)
            {
                _target_properties.map_from = to_symbols(n.get_map_from());
            }

            virtual void visit(const Nodecl::OpenMP::MapTo& n)
            {
                _target_properties.map_to = to_symbols(n.get_map_to());
            }

            virtual void visit(const Nodecl::OpenMP::MapToFrom& n)
            {
                _target_properties.map_tofrom = to_symbols(n.get_map_tofrom());
            }

            virtual void visit(const Nodecl::OpenMP::Device &n)
            {
                _target_properties.device_id = n.get_device_id();
            }

            virtual void visit(const Nodecl::OpenMP::TargetTaskUndeferred& n)
            {
                _target_properties.undeferred = true;
            }

            virtual void visit(const Nodecl::OpenMP::PriorityId& n)
            {
                _target_properties.priority_id = n.get_priority_id();
            }

            // virtual void visit(const Nodecl::OpenMP::DepIn& n)
            // {
            //     _target_properties.dep_in = n.get_in_deps().as<Nodecl::List>().to_object_list();
            // }

            // virtual void visit(const Nodecl::OpenMP::DepInout& n)
            // {
            //     _target_properties.dep_inout = n.get_inout_deps().as<Nodecl::List>().to_object_list();
            // }

            // virtual void visit(const Nodecl::OpenMP::DepOut& n)
            // {
            //     _target_properties.dep_out = n.get_out_deps().as<Nodecl::List>().to_object_list();
            // }

    };

    TargetProperties TargetProperties::gather_target_properties(
            Lowering* lowering,
            const Nodecl::OpenMP::Target& node)
    {
        TargetProperties target_properties(lowering);
        TargetPropertiesVisitor tv(target_properties);
        tv.walk(node.get_environment());

        target_properties.related_function = Nodecl::Utils::get_enclosing_function(node);
        target_properties.locus_of_target = node.get_locus();
        target_properties.target_body = node.get_statements();

        return target_properties;
    }

    TL::Symbol TargetProperties::add_field_to_class(
            TL::Symbol new_class_symbol,
            TL::Scope class_scope,
            const std::string& name,
            TL::Type field_type,
            const locus_t* locus)
    {
        TL::Type new_class_type = new_class_symbol.get_user_defined_type();

        std::string orig_field_name = name;

        TL::Symbol field = class_scope.new_symbol(orig_field_name);
        field.get_internal_symbol()->kind = SK_VARIABLE;
        symbol_entity_specs_set_is_user_declared(field.get_internal_symbol(), 1);

        field.set_type( field_type );
        field.get_internal_symbol()->locus = locus;

        symbol_entity_specs_set_is_member(field.get_internal_symbol(), 1);
        symbol_entity_specs_set_class_type(field.get_internal_symbol(),
                new_class_type.get_internal_type());
        symbol_entity_specs_set_access(field.get_internal_symbol(), AS_PUBLIC);
        class_type_add_member(
                new_class_type.get_internal_type(),
                field.get_internal_symbol(),
                class_scope.get_decl_context(),
                /* is_definition */ 1);

        return field;
    }

    TL::Scope TargetProperties::compute_scope_for_environment_structure()
    {
        TL::Scope sc = related_function.get_scope();
        return sc;
    }

    void TargetProperties::create_target_parameters_structure(
            /* out */
            TL::Type& data_env_struct,
            Nodecl::NodeclBase& args_size)
    {
        TL::Scope sc = compute_scope_for_environment_structure();

        TL::Counter &counter = TL::CounterManager::get_counter("mppa");
        std::stringstream ss;
        ss << "mppa_target_param_" << (int)counter;
        counter++;

        std::string structure_name;
        if (IS_C_LANGUAGE
                || IS_FORTRAN_LANGUAGE)
        {
            // We need an extra 'struct '
            structure_name = "struct " + ss.str();
        }
        else
        {
            structure_name = ss.str();
        }

        // Create the class symbol
        TL::Symbol new_class_symbol = sc.new_symbol(structure_name);
        new_class_symbol.get_internal_symbol()->kind = SK_CLASS;
        type_t* new_class_type = get_new_class_type(sc.get_decl_context(), TT_STRUCT);

        symbol_entity_specs_set_is_user_declared(new_class_symbol.get_internal_symbol(), 1);

        const decl_context_t* class_context = new_class_context(new_class_symbol.get_scope().get_decl_context(),
                new_class_symbol.get_internal_symbol());

        TL::Scope class_scope(class_context);

        class_type_set_inner_context(new_class_type, class_context);

        new_class_symbol.get_internal_symbol()->type_information = new_class_type;

        mppa_omp_datum = TL::Scope::get_global_scope().get_symbol_from_name("omp_param_t");
        ERROR_CONDITION(!mppa_omp_datum.is_valid(), "Symbol omp_param_t not found", 0);

        // Add n
        num_field = add_field_to_class(new_class_symbol,
                class_scope,
                "n",
                TL::Type::get_int_type(),
                locus_of_target);

        int s = map_to.size() + map_from.size() + map_tofrom.size();
        if (s > 0)
        {

            TL::Type array_of_mppa_omp_datum = mppa_omp_datum
                .get_user_defined_type()
                .get_array_to(
                        const_value_to_nodecl(const_value_get_signed_int(s)),
                        TL::Scope::get_global_scope());

            params_field = add_field_to_class(new_class_symbol,
                    class_scope,
                    "params",
                    array_of_mppa_omp_datum,
                    locus_of_target);
        }

        nodecl_t nodecl_output = nodecl_null();
        finish_class_type(new_class_type,
                ::get_user_defined_type(new_class_symbol.get_internal_symbol()),
                sc.get_decl_context(),
                locus_of_target,
                &nodecl_output);
        set_is_complete_type(new_class_type, /* is_complete */ 1);
        set_is_complete_type(get_actual_class_type(new_class_type), /* is_complete */ 1);

        info_structure
            = data_env_struct
            = new_class_symbol.get_user_defined_type();

        // FIXME - VLA
        args_size = const_value_to_nodecl_with_basic_type(
                const_value_get_integer(
                    info_structure.get_size(),
                    /* bytes */ type_get_size(get_size_t_type()),
                    /* sign */ 0),
                get_size_t_type());
    }

    namespace {

        TL::Symbol get_field_by_name(
                const TL::ObjectList<TL::Symbol>& fields,
                const std::string& name)
        {
            std::function<std::string(const TL::Symbol&)> get_name = &TL::Symbol::get_name;
            TL::ObjectList<TL::Symbol> sym_list = fields.find(get_name, name);
            ERROR_CONDITION(sym_list.empty(), "Field '%s' not found", name.c_str());
            return sym_list[0];
        }

    }

    Nodecl::NodeclBase TargetProperties::capture_data(TL::Symbol capture_var)
    {
        Nodecl::List l;

        int s = map_to.size() + map_from.size() + map_tofrom.size();

        Nodecl::NodeclBase expr_stmt =
            Nodecl::ExpressionStatement::make(
                    Nodecl::Assignment::make(
                        Nodecl::ClassMemberAccess::make(
                            Nodecl::Dereference::make(
                                    capture_var.make_nodecl(/* set_ref */ true),
                                    capture_var.get_type().points_to()),
                            num_field.make_nodecl(),
                            Nodecl::NodeclBase::null(),
                            num_field.get_type().get_lvalue_reference_to()),
                        const_value_to_nodecl(const_value_get_signed_int(s)),
                        num_field.get_type().get_lvalue_reference_to()));
        l.append(expr_stmt);

        if (s == 0)
            return l;

        struct {
            TL::ObjectList<TL::DataReference>* m;
            int map_kind; 
        } maps[] = {
            { &map_to, 0,  },
            { &map_from, 1, },
            { &map_tofrom, 2 }, /* check this */
            { NULL, 0 },
        };

        TL::ObjectList<TL::Symbol> fields = mppa_omp_datum.get_type().get_fields();

        TL::Symbol field_ptr = get_field_by_name(fields, "ptr");
        TL::Symbol field_size = get_field_by_name(fields, "size");
        TL::Symbol field_map_kind = get_field_by_name(fields, "type");

        int field_idx = 0;
        for (int i = 0; maps[i].m != NULL; i++)
        {
            TL::ObjectList<TL::DataReference>& current_map(*(maps[i].m));

            for (TL::ObjectList<TL::DataReference>::iterator it = current_map.begin();
                    it != current_map.end();
                    it++, field_idx++)
            {
                Nodecl::NodeclBase base_accessor =
                    Nodecl::ArraySubscript::make(
                            Nodecl::ClassMemberAccess::make(
                                Nodecl::Dereference::make(
                                    capture_var.make_nodecl(/* set_ref */ true),
                                    capture_var.get_type().points_to()),
                                params_field.make_nodecl(),
                                Nodecl::NodeclBase::null(),
                                params_field.get_type().get_lvalue_reference_to()),
                            Nodecl::List::make(
                                const_value_to_nodecl_with_basic_type(
                                    const_value_get_integer(
                                        field_idx,
                                        type_get_size(get_ptrdiff_t_type()),
                                        /* sign */ 1),
                                    get_ptrdiff_t_type())),
                            params_field.get_type().array_element().get_lvalue_reference_to());

                expr_stmt =
                    Nodecl::ExpressionStatement::make(
                            Nodecl::Assignment::make(
                                Nodecl::ClassMemberAccess::make(
                                    base_accessor.shallow_copy(),
                                    field_ptr.make_nodecl(),
                                    Nodecl::NodeclBase::null(),
                                    field_ptr.get_type().get_lvalue_reference_to()),
                                it->get_base_address().shallow_copy(),
                                field_ptr.get_type().get_lvalue_reference_to()
                                ));
                l.append(expr_stmt);

                expr_stmt =
                    Nodecl::ExpressionStatement::make(
                            Nodecl::Assignment::make(
                                Nodecl::ClassMemberAccess::make(
                                    base_accessor.shallow_copy(),
                                    field_size.make_nodecl(),
                                    Nodecl::NodeclBase::null(),
                                    field_size.get_type().get_lvalue_reference_to()),
                                it->get_sizeof().shallow_copy(),
                                field_size.get_type().get_lvalue_reference_to()));
                l.append(expr_stmt);

                expr_stmt =
                    Nodecl::ExpressionStatement::make(
                            Nodecl::Assignment::make(
                                Nodecl::ClassMemberAccess::make(
                                    base_accessor,
                                    field_map_kind.make_nodecl(),
                                    Nodecl::NodeclBase::null(),
                                    field_map_kind.get_type().get_lvalue_reference_to()),
                                const_value_to_nodecl(
                                    const_value_get_signed_int(maps[i].map_kind)
                                    ),
                                field_map_kind.get_type().get_lvalue_reference_to()));
                l.append(expr_stmt);
            }
        }

        return l;
    }

    Nodecl::NodeclBase TargetProperties::generate_cluster_function()
    {
        std::stringstream ss;
        TL::Counter &counter = TL::CounterManager::get_counter("cluster-function-name");
        ss << "_cluster_fn_" << (int)counter;
        counter++;

        return generate_target_code_function(ss.str(), /* is_device */ true);
    }

    Nodecl::NodeclBase TargetProperties::generate_host_function()
    {
        std::stringstream ss;
        TL::Counter &counter = TL::CounterManager::get_counter("host-function-name");
        ss << "_host_fn_" << (int)counter;
        counter++;

        return generate_target_code_function(ss.str(), /* is_device */ false);
    }

    Nodecl::NodeclBase TargetProperties::generate_target_code_function(
            const std::string& target_function_name,
            bool is_cluster)
    {
        TL::Symbol cluster_fun = SymbolUtils::new_function_symbol(
                related_function,
                target_function_name,
                TL::Type::get_void_type(),
                TL::ObjectList<std::string>(1, "_params"),
                TL::ObjectList<TL::Type>(1, mppa_omp_datum.get_user_defined_type().get_pointer_to()));

        Nodecl::NodeclBase function_code, empty_stmt;
        SymbolUtils::build_empty_body_for_function(
                cluster_fun,
                function_code,
                empty_stmt);

        struct {
            TL::ObjectList<TL::DataReference>* m;
            int map_kind; 
        } maps[] = {
            { &map_to, 0,  },
            { &map_from, 1, },
            { &map_tofrom, 2 }, /* check these magic numbers */
            { NULL, 0 },
        };

        TL::Scope scope_inside_function = empty_stmt.retrieve_context();
        Nodecl::Utils::SimpleSymbolMap symbol_map;
        if (is_cluster)
        {
            symbol_map = _lowering->get_cluster_mapping();
        }

        TL::Symbol field_ptr = get_field_by_name(
                mppa_omp_datum.get_type().get_fields(),
                "ptr");

        TL::Symbol params = scope_inside_function.get_symbol_from_name("_params");
        ERROR_CONDITION(!params.is_valid(), "Symbol '_params' not found", 0);

        int field_idx = 0;
        for (int i = 0; maps[i].m != NULL; i++)
        {
            TL::ObjectList<TL::DataReference>& current_map(*(maps[i].m));

            for (TL::ObjectList<TL::DataReference>::iterator it = current_map.begin();
                    it != current_map.end();
                    it++, field_idx++)
            {
                TL::Symbol base_symbol = it->get_base_symbol();

                Nodecl::NodeclBase base_accessor =
                    Nodecl::ArraySubscript::make(
                            params.make_nodecl(/* set_ref */ true),
                            Nodecl::List::make(
                                const_value_to_nodecl_with_basic_type(
                                    const_value_get_integer(
                                        field_idx,
                                        type_get_size(get_ptrdiff_t_type()),
                                        /* sign */ 1),
                                    get_ptrdiff_t_type())),
                            mppa_omp_datum.get_type().get_lvalue_reference_to());

                TL::Symbol new_local_sym =
                    scope_inside_function.new_symbol(base_symbol.get_name());
                new_local_sym.get_internal_symbol()->kind = SK_VARIABLE;

                TL::Type new_local_type = base_symbol.get_type().no_ref();
                if (!new_local_type.is_pointer())
                    new_local_type = new_local_type.get_lvalue_reference_to();

                new_local_sym.set_type(new_local_type);

                symbol_entity_specs_set_is_user_declared(new_local_sym.get_internal_symbol(), 1);

                symbol_map.add_map(base_symbol, new_local_sym);

                bool base_sym_is_ptr = base_symbol.get_type().no_ref().is_pointer();

                TL::Type suitable_ptr_type = base_symbol.get_type().no_ref();
                if (!base_sym_is_ptr)
                {
                    suitable_ptr_type = suitable_ptr_type.get_pointer_to();
                }

                Nodecl::NodeclBase cast =
                        Nodecl::Conversion::make(
                            Nodecl::Conversion::make(
                                Nodecl::ClassMemberAccess::make(
                                    base_accessor,
                                    field_ptr.make_nodecl(),
                                    Nodecl::NodeclBase::null(),
                                    field_ptr.get_type().get_lvalue_reference_to()),
                                field_ptr.get_type()),
                            suitable_ptr_type);
                cast.set_text("C");

                Nodecl::NodeclBase value = cast;
                if (!base_sym_is_ptr)
                {
                    value = Nodecl::Dereference::make(
                            cast,
                            suitable_ptr_type.points_to().get_lvalue_reference_to());
                }

                new_local_sym.set_value(value);

                empty_stmt.prepend_sibling(Nodecl::ObjectInit::make(new_local_sym));
            }
        }

        Nodecl::NodeclBase updated_tree =
            Nodecl::Utils::deep_copy(target_body, scope_inside_function, symbol_map);

        empty_stmt.prepend_sibling(updated_tree);

        Nodecl::Utils::remove_from_enclosing_list(empty_stmt);

        return function_code;
    }

} }
