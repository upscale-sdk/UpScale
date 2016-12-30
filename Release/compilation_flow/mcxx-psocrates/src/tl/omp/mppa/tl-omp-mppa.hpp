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


#ifndef TL_OMP_MPPA_HPP
#define TL_OMP_MPPA_HPP

#include "tl-compilerphase.hpp"
#include "tl-nodecl.hpp"
#include "tl-nodecl-utils.hpp"

namespace TL { namespace MPPA {

    enum mppa_arch_t {
        ANDEY_ARCH  = 0,
        BOSTAN_ARCH = 1,
    };

    class Lowering : public TL::CompilerPhase
    {
        private:
        public:
            Lowering();

            virtual void run(DTO& dto);
            virtual void pre_run(DTO& dto);

            void register_cluster_function(Nodecl::NodeclBase node);
            void register_cluster_declaration(Nodecl::NodeclBase node);
            void register_cluster_mapping(TL::Symbol sym, TL::Symbol new_sym);
            Nodecl::Utils::SimpleSymbolMap get_cluster_mapping();

            virtual void phase_cleanup(DTO& data_flow);
            virtual void phase_cleanup_end_of_pipeline(DTO& data_flow);

            mppa_arch_t get_mppa_arch() const;

        private:
            std::string _openmp_dry_run;
            Nodecl::List cluster_code;
            TL::ObjectList<Nodecl::NodeclBase> cluster_declarations;
            TL::ObjectList<TL::Symbol> cluster_functions;
            Nodecl::Utils::SimpleSymbolMap cluster_mapping;
            mppa_arch_t _arch;

            void emit_cluster_code();
            void compute_mmpa_architecture();
    };

} }

#endif // TL_OMP_MPPA_HPP
