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

#include "tl-target-properties.hpp"
#include "tl-counters.hpp"

#include "tl-nodecl-utils.hpp"

#include "cxx-cexpr.h"

namespace TL { namespace MPPA {

DeclareTargetVisitor::DeclareTargetVisitor(Lowering* lowering)
    : _lowering(lowering)
{
}

DeclareTargetVisitor::~DeclareTargetVisitor()
{
}

void DeclareTargetVisitor::visit(const Nodecl::OpenMP::DeclareTarget& decl)
{
    Nodecl::List declarations = decl.get_declarations().as<Nodecl::List>();
    for (Nodecl::List::iterator it = declarations.begin();
            it != declarations.end();
            it++)
    {
        TL::Symbol sym = it->get_symbol();
        ERROR_CONDITION(!sym.is_valid(), "Invalid symbol", 0);

        if (sym.is_function())
        {
            if (sym.get_function_code().is_null())
                continue;

            std::stringstream ss;
            TL::Counter &counter = TL::CounterManager::get_counter("cluster-function-name");
            ss << "_cluster_fn_" << (int)counter << "_" << sym.get_name();
            counter++;

            TL::Symbol new_function = sym.get_scope().new_symbol(ss.str());
            _lowering->register_cluster_mapping(sym, new_function);

            _declare_target_functions.append(sym);

        }
        else
        {
            internal_error("declare target not implemented yet for %s\n",
                    symbol_kind_descriptive_name(sym.get_internal_symbol()->kind));
        }
    }
}

} }
