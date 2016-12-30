
#ifndef EE_GET_CURRENT_CORE_H_
#define EE_GET_CURRENT_CORE_H_

#include "ee_k1_bsp.h"
#include "ee_hal_internal.h"
#include "ee_kernel_types.h"
#include "ee_kernel_k1.h"

EE_INLINE__ EE_CDB EE_CONST* EE_get_curr_core ( void ) {
  return &KDB_WJ.core_descriptors[EE_get_curr_core_id()];
}

EE_INLINE__ EE_CDB EE_CONST * EE_get_core ( EE_CORE_ID core_id ) {
  return &KDB_WJ.core_descriptors[core_id];
}

EE_INLINE__ EE_KDB EE_CONST * EE_get_kernel ( void ) {
  return &KDB_WJ.kdb;
}

EE_INLINE__ void EE_lock_kernel ( void ) {
  EE_k1_spin_lock( &KCB_WJ.lock );
}

EE_INLINE__ void EE_unlock_kernel ( void ) {
  EE_k1_spin_unlock( &KCB_WJ.lock );
}

EE_INLINE__ EE_CORE_ID EE_lock_and_get_curr_core_id ( void ) {
  EE_CORE_ID  const core_id = EE_get_curr_core_id();
  EE_CCB_WL * const p_CCB_wl = &KCB_WJ.core_ctrls[core_id];
  EE_k1_spin_lock(&p_CCB_wl->lock);
  return core_id;
}

EE_INLINE__ void  EE_lock_core ( EE_CORE_ID core_id ) {
  EE_k1_spin_lock(&KCB_WJ.core_ctrls[core_id].lock);
}

EE_INLINE__ EE_CDB EE_CONST * EE_lock_and_get_core ( EE_CORE_ID core_id ) {
  EE_CDB *    const p_CDB    = &KDB_WJ.core_descriptors[core_id];
  EE_CCB_WL * const p_CCB_wl = &KCB_WJ.core_ctrls[core_id];
  EE_k1_spin_lock(&p_CCB_wl->lock);
  return p_CDB;
}

EE_INLINE__ EE_CDB EE_CONST * EE_lock_and_get_curr_core ( void ) {
  return EE_lock_and_get_core( EE_get_curr_core_id() );
}

EE_INLINE__ void EE_unlock_core ( EE_CORE_ID core_id ) {
  EE_CCB_WL * const p_CCB_wl = &KCB_WJ.core_ctrls[core_id];
  EE_k1_spin_unlock(&p_CCB_wl->lock);
}

EE_INLINE__ void EE_unlock_curr_core ( void ) {
  EE_unlock_core ( EE_get_curr_core_id());
}

#endif /* EE_GET_CURRENT_CORE_H_ */
