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

#ifndef CODEGEN_SOCRATES_HPP
#define CODEGEN_SOCRATES_HPP

#include "codegen-phase.hpp"
#include "codegen-cxx.hpp"
namespace Codegen
{
    class Psocrates : public CxxBase
    {
    private:
        void walk_data_sharing_list(const Nodecl::List& l);

    public:
        using CxxBase::visit;

        Ret visit(const Nodecl::OmpSs::Target& n);
        Ret visit(const Nodecl::OpenMP::Atomic& n);
        Ret visit(const Nodecl::OpenMP::BarrierAtEnd& n);
        Ret visit(const Nodecl::OpenMP::BarrierFull& n);
        Ret visit(const Nodecl::OpenMP::DeclareTarget& n);
        Ret visit(const Nodecl::OpenMP::DepIn& n);
        Ret visit(const Nodecl::OpenMP::DepInout& n);
        Ret visit(const Nodecl::OpenMP::DepOut& n);
        Ret visit(const Nodecl::OpenMP::Device& n);
        Ret visit(const Nodecl::OpenMP::Firstprivate& n);
        Ret visit(const Nodecl::OpenMP::FlushAtEntry& n);
        Ret visit(const Nodecl::OpenMP::FlushAtExit& n);
        Ret visit(const Nodecl::OpenMP::For& n);
        Ret visit(const Nodecl::OpenMP::If& n);
        Ret visit(const Nodecl::OpenMP::Lastprivate& n);
        Ret visit(const Nodecl::OpenMP::MapFrom& n);
        Ret visit(const Nodecl::OpenMP::MapToFrom& n);
        Ret visit(const Nodecl::OpenMP::MapTo& n);
        Ret visit(const Nodecl::OpenMP::Master& n);
        Ret visit(const Nodecl::OpenMP::Parallel& n);
        Ret visit(const Nodecl::OpenMP::Private& );
        Ret visit(const Nodecl::OpenMP::Schedule& n);
        Ret visit(const Nodecl::OpenMP::Shared& n);
        Ret visit(const Nodecl::OpenMP::Single& n);
        Ret visit(const Nodecl::OpenMP::Target& n);
        Ret visit(const Nodecl::OpenMP::TargetTaskUndeferred& n);
        Ret visit(const Nodecl::OpenMP::TaskId& n);
        Ret visit(const Nodecl::OpenMP::Task& n);
        Ret visit(const Nodecl::OpenMP::TaskwaitShallow& n);
        Ret visit(const Nodecl::OpenMP::Untied& n);

        Ret visit(const Nodecl::Range& n);

        void visit_map(Nodecl::List l, const std::string& motion);
    };
}

#endif // CODEGEN_SOCRATES_HPP
