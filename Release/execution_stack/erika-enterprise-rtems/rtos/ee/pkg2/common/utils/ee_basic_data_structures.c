#include "ee_utils.h"
#include "ee_basic_data_structures.h"

EE_node_vft const ee_ll_node_vft = {
  .pf_get_next = &EE_ll_node_get_next,
  .pf_insert_next = &EE_ll_node_insert_next
};

EE_list_vft const ee_list_base_vft = {
  .pf_insert       = &EE_list_base_insert,
  .pf_extract_head = &EE_list_base_extract
};

EE_list_vft const ee_linear_priority_queue_vft = {
  .pf_insert       = &EE_linear_priority_queue_insert,
  .pf_extract_head = &EE_list_base_extract
};

EE_cmp_vft const ee_cmp_base_vft = {
  .pf_cmp = &EE_cmp_base_cmp
};

EE_node_base *  EE_ll_node_get_next ( EE_node_base * p_node ) {
  EE_ll_node * p_next_node;
  EE_ll_node * p_l_node = (EE_ll_node *)p_node;
  if ( p_l_node != NULL ) {
    p_next_node = p_l_node->p_next;
  } else {
    p_next_node = NULL;
  }
  return &p_next_node->parent;
}
void EE_ll_node_insert_next ( EE_node_base * p_node,
    EE_node_base * p_next_node )
{
  EE_ll_node * p_l_node = (EE_ll_node *)p_node;
  if ( p_l_node != NULL ) {
    p_l_node->p_next =  (EE_ll_node *)p_next_node;
  }
}


EE_BOOL EE_list_base_insert ( EE_list_base *p_ll, EE_node_base * p_new_node )
{
  EE_BOOL insert = EE_FALSE;
  if ( p_ll != NULL ) {
    EE_NODE_INSERT_NEXT(p_new_node, p_ll->p_head);
    p_ll->p_head       = p_new_node;
    insert = EE_TRUE;
  }
  return insert;
}

EE_node_base * EE_list_base_extract ( EE_list_base *p_ll ) {
  EE_node_base * p_old_head = NULL;
  if ( p_ll != NULL ) {
    p_old_head = p_ll->p_head;
    if ( p_old_head != NULL ) {
      p_ll->p_head = EE_NODE_GET_NEXT(p_old_head);
    }
  }
  return p_old_head;
}

int  EE_cmp_base_cmp ( EE_cmp * p_this, EE_cmp * p_other ) {
  int cmp_result;
  EE_cmp_base * p_this_base  = (EE_cmp_base *)p_this;
  EE_cmp_base * p_other_base = (EE_cmp_base *)p_other;
  if ( p_this_base->value > p_other_base->value ) {
    cmp_result = EE_GREATER_THAN;
  } else if ( p_this_base->value < p_other_base->value ) {
    cmp_result = EE_LESSER_THAN;
  } else {
    cmp_result = EE_EQUAL_TO;
  }
  return cmp_result;
}

EE_BOOL EE_linear_priority_queue_insert ( EE_list_base *p_l,
    EE_node_base * p_new_node )
{
  EE_BOOL head_changed           = EE_FALSE;
  EE_ll_node *p_tmp              = ((EE_ll_node *)p_l->p_head);
  EE_ll_node *p_l_new_node       = ((EE_ll_node *)p_new_node);

  if (p_tmp == NULL) {
    p_l->p_head        = p_new_node;
    p_l_new_node->p_next = NULL;
    head_changed       = EE_TRUE;
  } else {
    /* Traverse the queue until needed */
    do {
      if ( EE_CMP(p_l_new_node->parent.p_elem, p_tmp->parent.p_elem) > 0 ) {
        p_l_new_node->p_next = p_tmp;
        break;
      } else {
        p_tmp = p_tmp->p_next;
      }
    } while (p_tmp->p_next != NULL);

    if ((EE_node_base *)p_tmp == p_l->p_head) {
      p_l->p_head = p_new_node;
      head_changed = EE_TRUE;
    } else if (p_tmp->p_next == NULL) {
      /* Insert the new scheduler data at the tail of the queue */
      p_tmp->p_next = p_l_new_node;
      p_l_new_node->p_next = NULL;
    }
  }
  return head_changed;
}

