/*--------------------------------------------------------------------
  (C) Copyright 2006-2012 Barcelona Supercomputing Center
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

#include "cxx-cexpr.h"
#include "codegen-psocrates.hpp"
#include "tl-nodecl-utils.hpp"

namespace {
    bool print_shared = true;
    bool emit_omp4_array_section = false;
}


namespace Codegen
{
    void Psocrates::visit(const Nodecl::Range& n)
    {
        if (!emit_omp4_array_section)
        {
            this->CxxBase::visit(n);
        }
        else
        {
            walk(n.get_lower());
            *file << ":";

            if (n.get_upper().is_null())
            {
                // done
            }
            else if (n.get_upper().is_constant())
            {
                // Generate the constant +1
                walk(
                        const_value_to_nodecl_with_basic_type(
                            const_value_add(
                                n.get_upper().get_constant(),
                                const_value_get_signed_int(1)),
                            get_ptrdiff_t_type()
                            )
                    );
            }
            else
            {
                *file << "(";
                walk(n.get_upper());
                *file << ") + 1";
            }
        }
    }

    void Psocrates::walk_data_sharing_list(const Nodecl::List& l)
    {
        bool first = true;
        for (Nodecl::List::iterator it = l.begin(); it != l.end(); ++it)
        {
            TL::Symbol sym = it->get_symbol();
            if (!sym.is_variable()
                || sym.get_type().is_const())
                continue;

            if (!first)
                *file << ", ";
            walk(*it);
            first = false;
        }
    }

    void Psocrates::visit(const Nodecl::OmpSs::Target& n)
    {
        const Nodecl::List devices = n.get_devices().as<Nodecl::List>();
        ERROR_CONDITION(devices.size() != 1,
                        "Psocrates codegen expects one unique device SMP, but %d found\n",
                        devices.size());
        const Nodecl::Text device = devices.at(0).as<Nodecl::Text>();
        ERROR_CONDITION(device.get_text() != "smp",
                        "Psocrates codegen expects target device SMP, but %s found\n",
                        device.get_text().c_str());
    }

    void Psocrates::visit(const Nodecl::OpenMP::Atomic& n)
    {
        *file << "#pragma omp atomic\n";
        walk(n.get_statements());
    }

    void Psocrates::visit(const Nodecl::OpenMP::BarrierAtEnd& n)
    {}  // Nothing to print

    void Psocrates::visit(const Nodecl::OpenMP::BarrierFull& n)
    {
        *file << "#pragma omp barrier\n";
    }

    void Psocrates::visit(const Nodecl::OpenMP::DeclareTarget& n)
    {
        // Nothing to print for the moment
    }

    void Psocrates::visit(const Nodecl::OpenMP::DepIn& n)
    {
        *file << "depend(in: ";
        walk_list(n.get_exprs().as<Nodecl::List>(), ", ");
        *file << ")";
    }

    void Psocrates::visit(const Nodecl::OpenMP::DepInout& n)
    {
        *file << "depend(inout: ";
        walk_list(n.get_exprs().as<Nodecl::List>(), ", ");
        *file << ")";
    }

    void Psocrates::visit(const Nodecl::OpenMP::DepOut& n)
    {
        *file << "depend(out: ";
        walk_list(n.get_exprs().as<Nodecl::List>(), ", ");
        *file << ")";
    }

    void Psocrates::visit(const Nodecl::OpenMP::Device& n)
    {
        *file << "device(";
        walk(n.get_device_id());
        *file << ")";

    }

    void Psocrates::visit(const Nodecl::OpenMP::Firstprivate& n)
    {
        *file << "firstprivate(";
        walk_data_sharing_list(n.get_symbols().as<Nodecl::List>());
        *file << ")";
    }

    void Psocrates::visit(const Nodecl::OpenMP::FlushAtEntry& n)
    {}      // Do nothing, internal MCC clauses

    void Psocrates::visit(const Nodecl::OpenMP::FlushAtExit& n)
    {}      // Do nothing, internal MCC clauses

    void Psocrates::visit(const Nodecl::OpenMP::For& n)
    {
        *file << "#pragma omp for ";
        const Nodecl::List environ = n.get_environment().as<Nodecl::List>();
        print_shared = false;
        walk_list(environ, " ");
        print_shared = true;
        // Special clauses
        if (!Nodecl::Utils::nodecl_contains_nodecl_of_kind<Nodecl::OpenMP::BarrierAtEnd>(environ))
            *file << " nowait";
        *file << "\n";

        // 2.- Print the statements
        walk(n.get_loop());
    }

    void Psocrates::visit(const Nodecl::OpenMP::If& n)
    {
        *file << "if(";
        walk(n.get_condition());
        *file << ")";
    }

    void Psocrates::visit(const Nodecl::OpenMP::Lastprivate& n)
    {
        *file << "lastprivate(";
        walk_data_sharing_list(n.get_symbols().as<Nodecl::List>());
        *file << ")";
    }

    void Psocrates::visit(const Nodecl::OpenMP::MapFrom& n)
    {
        *file << "map(from: ";
        walk_list(n.get_map_from().as<Nodecl::List>(), ", ");
        *file << ")";
    }

    void Psocrates::visit(const Nodecl::OpenMP::MapToFrom& n)
    {
        *file << "map(tofrom: ";
        walk_list(n.get_map_tofrom().as<Nodecl::List>(), ", ");
        *file << ")";
    }

    void Psocrates::visit(const Nodecl::OpenMP::MapTo& n)
    {
        *file << "map(to: ";
        walk_list(n.get_map_to().as<Nodecl::List>(), ", ");
        *file << ")";
    }

    void Psocrates::visit(const Nodecl::OpenMP::Master& n)
    {
        *file << "#pragma omp master\n";
        walk(n.get_statements());
    }

    void Psocrates::visit(const Nodecl::OpenMP::Parallel& n)
    {
        // 1.- Print the pragma construct
        *file << "#pragma omp parallel ";
        const Nodecl::List environ = n.get_environment().as<Nodecl::List>();
        walk_list(environ, " ");
        // Special clauses
        Nodecl::NodeclBase num_replicas = n.get_num_replicas();
        if (!num_replicas.is_null())
        {
            *file << " num_threads(";
                walk(num_replicas);
            *file << ")";
        }
        *file << "\n";

        // 2.- Print the statements
        walk(n.get_statements());
    }

    void Psocrates::visit(const Nodecl::OpenMP::Private& n)
    {
        *file << "private(";
        walk_data_sharing_list(n.get_symbols().as<Nodecl::List>());
        *file << ")";
    }

    void Psocrates::visit(const Nodecl::OpenMP::Schedule& n)
    {
        *file << "schedule(";
        *file << n.get_text();
        if (n.get_text() == "static")
        {
            if (n.get_chunk().is_constant()
                && const_value_is_zero(n.get_chunk().get_constant()))
            {}  // default chunk, do not print anything
            else
            {
                *file << ",";
                walk(n.get_chunk());
            }
        }
        else
        {
            *file << ",";
            walk(n.get_chunk());
        }
        *file << ")";
    }

    void Psocrates::visit(const Nodecl::OpenMP::Shared& n)
    {
        if (print_shared)
        {
            *file << "shared(";
            walk_data_sharing_list(n.get_symbols().as<Nodecl::List>());
            *file << ")";
        }
    }

    void Psocrates::visit(const Nodecl::OpenMP::Single& n)
    {
        // 1.- Print the pragma construct
        *file << "#pragma omp single ";
        print_shared = false;
        walk_list(n.get_environment().as<Nodecl::List>(), " ");
        print_shared = true;
        *file << "\n";

        // 2.- Print the statements
        walk(n.get_statements());
    }

    void Psocrates::visit(const Nodecl::OpenMP::Target& n)
    {
        *file << "#pragma omp target ";
        Nodecl::List l = n.get_environment().as<Nodecl::List>();
        if (l.find_first<Nodecl::OpenMP::TargetTaskUndeferred>().is_null())
        {
            *file << "nowait ";
        }
        walk_list(l, " ");
        *file << "\n";

        walk(n.get_statements());
    }

    void Psocrates::visit(const Nodecl::OpenMP::TargetTaskUndeferred& n)
    {
        // Internal marker
    }

    void Psocrates::visit_map(Nodecl::List l, const std::string& motion)
    {
        emit_omp4_array_section = true;
        *file << "map(" << motion << ": ";
        walk_list(l, ", ");
        *file << ")";
        emit_omp4_array_section = false;
    }

    void Psocrates::visit(const Nodecl::OpenMP::Task& n)
    {
        // 1.- Print the pragma construct
        *file << "#pragma omp task ";
        walk_list(n.get_environment().as<Nodecl::List>(), " ");
        *file << "\n";

        // 2.- Print the statements
        walk(n.get_statements());
    }

    void Psocrates::visit(const Nodecl::OpenMP::TaskId& n)
    {
        *file << "id(";
        walk(n.get_id());
        *file << ")";
    }

    void Psocrates::visit(const Nodecl::OpenMP::TaskwaitShallow& n)
    {
        *file << "#pragma omp taskwait\n";
    }

    void Psocrates::visit(const Nodecl::OpenMP::Untied& n)
    {
        *file << "untied";
    }

} // Codegen

EXPORT_PHASE(Codegen::Psocrates)
