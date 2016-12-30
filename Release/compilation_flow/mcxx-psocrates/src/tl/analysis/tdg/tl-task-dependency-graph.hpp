/*--------------------------------------------------------------------
 (C) Copyright 2006-2014 Barcelona Supercomputing Center             *
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

#ifndef TL_TASK_DEPENDENCY_GRAPH_HPP
#define TL_TASK_DEPENDENCY_GRAPH_HPP

#include "tl-extensible-graph.hpp"
#include "tl-nodecl-replacer.hpp"

namespace TL { 
namespace Analysis {

    #define TDG_DEBUG debug_options.tdg_verbose

    // ******************************************************************* //
    // ************ Task Dependency Graph Control Structures ************* //
    
    enum ControlStructureType {
        Implicit,
        Loop,
        IfElse
    };
    
    struct ControlStructure {
        int _id;
        ControlStructureType _type;
        NBase _condition;
        Node* _pcfg_node;
        ControlStructure* _enclosing;
        
        // *** Constructor *** //
        ControlStructure(int id, ControlStructureType type, 
                         const NBase& condition, Node* pcfg_node);
        
        // *** Getters and setters *** //
        int get_id() const;
        ControlStructureType get_type() const;
        std::string get_type_as_string() const;
        Nodecl::NodeclBase get_condition() const;
        Node* get_pcfg_node() const;
        ControlStructure* get_enclosing_cs() const;
        void set_enclosing_cs(ControlStructure* cs);
    };
    
    typedef std::vector<std::pair<ControlStructure*, std::string> > ControlStList;
    
    // ************ Task Dependency Graph Control Structures ************* //
    // ******************************************************************* //
    
    
    // ******************************************************************* //
    // ************** Task Dependency Graph Edges and Nodes ************** //
    
    struct TDG_Edge;
    
    enum TDGNodeType {
        Task,
        Taskpart,
        Target,
        Taskwait,
        Barrier,
        Unknown
    };
    
    struct TDG_Node {
        unsigned int _id;
        Node* _pcfg_node;
        TDGNodeType _type;
        Node* _parent;
        ObjectList<TDG_Edge*> _entries;
        ObjectList<TDG_Edge*> _exits;
        ControlStList _control_structures;

        Node* _init_taskpart;

        // *** Constructor *** //
        TDG_Node(Node* n, TDGNodeType type, Node* parent, Node* init_tp=NULL);
        
        // *** Getters and setters *** //
        unsigned int get_id() const;
        Node* get_pcfg_node() const;
        void add_control_structure(ControlStructure* cs, std::string taken_branch);
        ControlStList get_control_structures() const;

        friend struct TDG_Edge;
        friend class TaskDependencyGraph;
    };

    struct TDG_Edge {
        TDG_Node* _source;
        TDG_Node* _target;
        SyncKind _kind;
        NBase _condition;

        // *** Constructor *** //
        TDG_Edge(TDG_Node* source, TDG_Node* target, SyncKind kind, const NBase& condition);

        // *** Getters and setters *** //
        TDG_Node* get_source() const;
        TDG_Node* get_target() const;

        friend struct TDG_Node;
        friend class TaskDependencyGraph;
    };
    
    // ************** Task Dependency Graph Edges and Nodes ************** //
    // ******************************************************************* //
    
    
    // ******************************************************************* //
    // ********************** Task Dependency Graph ********************** //
    
    typedef std::multimap<std::pair<Node*, Node*>, ControlStructure*> PCFG_to_CS;
    
    class LIBTL_CLASS TaskDependencyGraph
    {
    private:
        // *** Class members *** //
        unsigned int _id;                               /*!< Identifier to be used at runtime */
        ExtensibleGraph* _pcfg;                         /*!< PCFG corresponding to the graph */
        std::string _json_name;                         /*!< Name of the TDG and the JSON file */
        std::map<unsigned int, TDG_Node*> _tdg_nodes;   /*!< Map of nodes in the TDG */
                                                        // We have a map where the key is the locus of the original statement
                                                        // this way we can print the nodes in order
        
        std::map<NBase, unsigned int, Nodecl::Utils::Nodecl_structural_less> _syms; /*!< Map of symbols appearing in the TDG associated to their identifier */
        PCFG_to_CS _pcfg_to_cs_map;                     /*!< Map of PCFG control structure nodes to their TDG control structure object */

        bool _taskparts_enabled;                        /*!< Enable support for taskparts analysis */
        bool _psocrates;                                /*!< The TDG is generated in the P-Socrates context (it contains tracing calls) */

        // *** Not allowed construction methods *** //
        TaskDependencyGraph(const TaskDependencyGraph& n);
        TaskDependencyGraph& operator=(const TaskDependencyGraph&);

        // *** Private methods *** //
        void create_tdg_node(
                Node* current,
                int tdg_node_id,
                TDGNodeType type,
                Node* init_tp = NULL);

        void connect_tdg_nodes(
                TDG_Node* parent, TDG_Node* child,
                SyncKind sync_type, const NBase& condition);

        TDG_Node* find_tdg_node_from_pcfg_node(Node* task);

        void create_tdg_task_nodes_from_pcfg(Node* current);
        void create_tdg_nodes_from_pcfg(Node* current);
        void create_tdg_nodes_from_taskparts(Node* current);

        enum CS_case {
            same_cs_init_before_end,    /*this is valid also for non-taskpart nodes*/
            same_cs_end_before_init,
            init_encloses_end_init_before_end,
            init_encloses_end_end_before_init,
            end_encloses_init_init_before_end,
            end_encloses_init_end_before_init
        };
        void create_control_structure_rec(
                CS_case cs_case,
                Node* init,     /*NULL for non-taskpart nodes*/
                Node* n,
                Node* control_structure,
                TDG_Node* tdg_node,
                ControlStructure* last_cs);
        void create_control_structures(
                TDG_Node* tdg_node,
                ControlStructure* last_cs);
        void set_tdg_nodes_control_structures();

        /*!This method forces tasks to be connected to
         * their immediate previous static synchronization (taskwait or barrier)
         * The method traverses the graph from node \p sync until it exits or
         * finds a taskwait or barrier in the same context.
         * All tasks found during the traversal are connected to \p sync
         * with a Static synchronization edge
         *
         * \param sync a taskwait or barrier nodes
         */
        void connect_tasks_to_previous_synchronization(Node* sync);
        void connect_dependent_nodes(Node* n);
        void connect_taskparts();
        void connect_tdg_nodes_from_pcfg(Node* current);

        void store_condition_list_of_symbols(const NBase& condition, const NodeclMap& reach_defs);

        void create_tdg(Node* current);

        std::string print_value(const NBase& var, const NBase& value) const;
        void print_tdg_node_to_dot(TDG_Node* current, std::ofstream& dot_tdg) const;
        void print_condition(
                TDG_Edge* edge,
                ControlStructure* node_cs,
                std::ofstream& json_tdg,
                std::string indent) const;
        void print_tdg_control_structs_to_json(std::ofstream& json_tdg) const;

        void print_tdg_syms_to_json(std::ofstream& json_tdg);
        void print_dependency_variables_to_json(
            std::ofstream& json_tdg,
            const VarToNodeclMap& var_to_value_map,
            const VarToNodeclMap& var_to_id_map,
            const NBase& condition,
            std::string indent,
            bool is_source,
            bool add_final_comma) const;
        void print_tdg_nodes_to_json(std::ofstream& json_tdg);
        void print_tdg_edges_to_json(std::ofstream& json_tdg) const;
        
    public:
        // *** Constructor *** //
        TaskDependencyGraph(
                ExtensibleGraph* pcfg,
                std::string json_name,
                bool taskparts_enabled,
                bool psocrates);
        
        // *** Getters and Setters *** //
        std::string get_name() const;
        bool contains_nodes() const;
        ExtensibleGraph* get_pcfg() const;
        unsigned int get_id() const;
        
        // *** Printing methods *** //
        void print_tdg_to_dot() const;
        void print_tdg_to_json(std::ofstream& json_tdg);
        static void print_tdgs_to_json(const ObjectList<TaskDependencyGraph*>& tdgs);
    };

    // ********************** Task Dependency Graph ********************** //
    // ******************************************************************* //
}
}

#endif  // TL_TASK_DEPENDENCY_GRAPH_HPP
