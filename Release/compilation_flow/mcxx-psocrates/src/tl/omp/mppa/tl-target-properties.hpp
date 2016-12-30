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


#include "tl-datareference.hpp"
#include "tl-omp-mppa.hpp"
#include "tl-nodecl.hpp"
#include "tl-symbol.hpp"

namespace TL { namespace MPPA {

    struct TargetProperties
    {
        // typedef std::map<TL::Symbol, TL::Symbol> field_map_t;
        // field_map_t field_map;
        Nodecl::NodeclBase device_id;

        TL::ObjectList<TL::DataReference> map_from;
        TL::ObjectList<TL::DataReference> map_to;
        TL::ObjectList<TL::DataReference> map_tofrom;

        // TL::ObjectList<Nodecl::NodeclBase> dep_in;
        // TL::ObjectList<Nodecl::NodeclBase> dep_inout;
        // TL::ObjectList<Nodecl::NodeclBase> dep_out;

        Nodecl::NodeclBase target_body;

        Nodecl::NodeclBase priority_id;

        bool undeferred;

        Lowering* _lowering;

        private:

        TL::Symbol related_function;
        const locus_t* locus_of_target;
        TL::Type info_structure;
        TL::Symbol num_field;
        TL::Symbol params_field;

        TL::Symbol mppa_omp_datum;

        Nodecl::NodeclBase generate_target_code_function(const std::string& target_function_name, bool is_device);

        public:

        TargetProperties(Lowering* lowering) : undeferred(false), _lowering(lowering) { }

        static TargetProperties gather_target_properties(
                Lowering* lowering,
                const Nodecl::OpenMP::Target& node);

        TL::Scope compute_scope_for_environment_structure();

        TL::Symbol add_field_to_class(
                TL::Symbol new_class_symbol,
                TL::Scope class_scope,
                const std::string& name,
                TL::Type field_type,
                const locus_t* locus);

        void create_target_parameters_structure(
                /* out */
                TL::Type& data_env_struct,
                Nodecl::NodeclBase& args_size);

        Nodecl::NodeclBase capture_data(TL::Symbol capture_var);

        Nodecl::NodeclBase generate_host_function();
        Nodecl::NodeclBase generate_cluster_function();
    };

} }

