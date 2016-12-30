#ifndef EE_BASIC_DATA_STRUCTURES_H_
#define EE_BASIC_DATA_STRUCTURES_H_

#include "ee_platform_types.h"
#include "ee_utils.h"

typedef struct EE_node_vft_tag   EE_node_vft;
typedef struct EE_node_base_tag  EE_node_base;
typedef struct EE_list_vft_tag   EE_list_vft;
typedef struct EE_list_base_tag  EE_list_base;

typedef struct EE_ll_node_tag    EE_ll_node;

typedef EE_list_base EE_stack;
typedef EE_list_base EE_queue;

typedef struct EE_cmp_vft_tag EE_cmp_vft;

typedef struct EE_cmp_tag EE_cmp;
typedef struct EE_cmp_base_tag EE_cmp_base;
typedef struct EE_linear_priority_queue_tag EE_linear_priority_queue;

typedef struct EE_ll_node_pool_tag EE_ll_node_pool;

static inline void  EE_ll_node_base_c_tor ( EE_ll_node * ll_node );
EE_node_base *      EE_ll_node_get_next ( EE_node_base * p_node );
void                EE_ll_node_insert_next ( EE_node_base * p_node,
  EE_node_base * p_next_node );

static inline void  EE_list_base_c_tor ( EE_list_base * p_l );
EE_BOOL             EE_list_base_insert ( EE_list_base *p_l,
  EE_node_base * p_new_node );
EE_node_base *      EE_list_base_extract ( EE_list_base *p_l );

static inline void EE_stack_c_tor ( EE_stack * p_stack );

static inline void EE_cmp_base_ctor ( EE_cmp_base * p_cmp_base );
int  EE_cmp_base_cmp ( EE_cmp * p_this, EE_cmp * p_other );

static inline void EE_linear_priority_queue_c_tor ( EE_linear_priority_queue *
  p_queue );
EE_BOOL EE_linear_priority_queue_insert ( EE_list_base *p_ll, EE_node_base *
  p_new_node );

extern EE_node_vft const ee_ll_node_vft;
extern EE_list_vft const ee_list_base_vft;
extern EE_list_vft const ee_linear_priority_queue_vft;
extern EE_list_vft const ee_linear_priority_queue_with_pool_vft;
extern EE_cmp_vft const ee_cmp_base_vft;

typedef struct EE_node_vft_tag {
  EE_node_base * (*pf_get_next) ( EE_node_base * p_node );
  void (*pf_insert_next) ( EE_node_base * p_node, EE_node_base * p_next_node );
} EE_node_vft;

typedef struct EE_node_base_tag {
  EE_node_vft vft;
  void *      p_elem;
} EE_node_base;

typedef struct EE_ll_node_tag {
  EE_node_base parent;
  EE_ll_node * p_next;
} EE_ll_node;

/* Messa qui: ha bisogno della definizione di EE_ll_node */
static inline void EE_ll_node_pool_array_c_tor ( EE_ll_node_pool * p_pool,
  EE_ll_node node_array[] );

typedef struct EE_list_vft_tag {
  EE_BOOL (*pf_insert) (EE_list_base *p_l, EE_node_base * p_new_node);
  EE_node_base * (*pf_extract_head) (EE_list_base *p_l);
} EE_list_vft;

typedef struct EE_list_base_tag {
  EE_list_vft    vft;
  EE_node_base * p_head;
} EE_list_base;

#define EE_GREATER_THAN 1
#define EE_LESSER_THAN  -1
#define EE_EQUAL_TO     0

typedef struct EE_cmp_vft_tag {
  int (*pf_cmp) (EE_cmp * p_this, EE_cmp * p_other);
} EE_cmp_vft;

typedef struct EE_cmp_tag {
  EE_cmp_vft vft;
} EE_cmp;

typedef struct EE_cmp_base_tag {
  EE_cmp  parent;
  EE_UREG value;
} EE_cmp_base;

typedef struct EE_linear_priority_queue_tag {
  EE_queue parent;
} EE_linear_priority_queue;

typedef struct EE_ll_node_pool_tag {
  EE_ll_node * p_free_nodes;
  EE_UREG      pool_size;
} EE_ll_node_pool;

#define EE_DECLARE_LL_NODE_POOL_ARRAY(pool_name, pool_size)               \
  struct EE_S_J(EE_ll_node_pool_,pool_name) {                             \
    EE_ll_node_pool pool;                                                 \
    EE_ll_node      node_pool_array[pool_size];                           \
  };                                                                      \
  extern struct EE_S_J(EE_ll_node_pool_, pool_name) pool_name;

#define EE_DEFINE_LL_NODE_POOL_ARRAY(pool_name, pool_size)                \
  struct EE_S_J(EE_ll_node_pool_, pool_name) pool_name = {                \
    .pool = { NULL, pool_size },                                          \
  }

#define EE_NODE_GET_NEXT(p_node)\
  ((EE_node_vft *)(p_node))->pf_get_next((EE_node_base *)(p_node))

#define EE_NODE_INSERT_NEXT(p_node, p_next_node)\
  ((EE_node_vft *)(p_node))->pf_insert_next((EE_node_base *)(p_node),\
    (EE_node_base *)(p_next_node))

#define EE_LIST_INSERT(p_l, new_elem)\
  ((EE_list_vft *)(p_l))->pf_insert((EE_list_base *)(p_l),\
    (EE_node_base *)(new_elem))

#define EE_LIST_EXTRACT(p_l) \
  ((EE_list_vft *)(p_l))->pf_extract_head((EE_list_base *)(p_l))

#define EE_LIST_PEEK(p_l) (((EE_list_base *)(p_l))->p_head)

#define EE_STACK_PUSH(p_stack, new_elem)\
  EE_list_base_insert(p_stack, (EE_node_base *)new_elem)
#define EE_STACK_POP(p_stack)\
  EE_list_base_extract(p_stack)

#define EE_STACK_PEEK(p_stack)              EE_LIST_PEEK(p_stack)

#define EE_QUEUE_INSERT(p_queue, new_elem)  EE_LIST_INSERT(p_queue, new_elem)
#define EE_QUEUE_EXTRACT(p_queue)           EE_LIST_EXTRACT(p_queue)
#define EE_QUEUE_PEEK(p_queue)              EE_LIST_PEEK(p_queue)

#define EE_CMP(p_this, p_other) \
  ((EE_cmp_vft *)(p_this))->pf_cmp((EE_cmp *)(p_this), (EE_cmp *)(p_other))

static inline void  EE_ll_node_base_c_tor ( EE_ll_node * ll_node ) {
  ll_node->parent.vft = ee_ll_node_vft;
}

static inline void EE_list_base_c_tor ( EE_list_base * p_l ) {
  p_l->vft    = ee_list_base_vft;
  p_l->p_head = NULL;
}

static inline void EE_stack_c_tor ( EE_stack * p_stack ) {
  EE_list_base_c_tor ( p_stack );
}

static inline void EE_cmp_base_ctor ( EE_cmp_base * p_cmp_base ) {
  p_cmp_base->parent.vft = ee_cmp_base_vft;
  p_cmp_base->value      = 0U;
}

static inline void EE_linear_priority_queue_c_tor ( EE_linear_priority_queue *
    p_queue )
{
  p_queue->parent.vft    = ee_linear_priority_queue_vft;
  p_queue->parent.p_head = NULL;
}

static inline void EE_ll_node_pool_array_c_tor ( EE_ll_node_pool * p_pool,
  EE_ll_node node_array[] )
{
  EE_array_index i;
  EE_array_index const last_elem = (p_pool->pool_size - 1U);
  for ( i = 0U; i < last_elem; ++i ) {
    node_array[i].parent.vft = ee_ll_node_vft;
    node_array[i].p_next     = &node_array[i + 1U];
  }
  node_array[last_elem].parent.vft = ee_ll_node_vft;
  node_array[last_elem].p_next     = NULL;
  p_pool->p_free_nodes = &node_array[0];
}

static inline EE_ll_node *  EE_ll_node_pool_alloc ( EE_ll_node_pool * p_pool )
{
  EE_ll_node * p_alloc = p_pool->p_free_nodes;
  p_pool->p_free_nodes = p_pool->p_free_nodes->p_next;
  return p_alloc;
}

static inline void * EE_ll_node_pool_free ( EE_ll_node_pool * p_pool,
    EE_ll_node * p_free_node )
{
  p_free_node->p_next  = p_pool->p_free_nodes;
  p_pool->p_free_nodes = p_free_node;
  return p_free_node->parent.p_elem;
}

#endif /* !EE_BASIC_DATA_STRUCTURES_H_ */
