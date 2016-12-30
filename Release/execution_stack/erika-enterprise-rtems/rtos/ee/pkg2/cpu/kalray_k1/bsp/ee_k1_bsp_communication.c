#include "ee_k1_bsp_internal.h"
#include "ee_k1_mppa_trace.h"
#include "ee_api_types.h"
#include "ee_hal_internal.h"
#include <string.h>

EE_k1_mailbox __attribute__ ((aligned(_K1_DCACHE_LINE_SIZE)))
  ee_k1_core_mailboxes[EE_K1_PE_NUMBER];

EE_status_type EE_k1_send_message (EE_CORE_ID remote_id, void *p_msg,
  void * p_ans, EE_mem_size ans_size)
{
  EE_status_type status_type = E_OK;
  EE_UREG  mailbox_id, msg_id;
  EE_UREG  cpu_id = EE_k1_get_cpu_id();
  EE_UREG  neightbor_link = (cpu_id ^ remote_id);

  if ( cpu_id == remote_id ) {
    status_type = E_OS_ID;
  } else if ( remote_id == EE_K1_BOOT_CORE ) {
    mailbox_id     = cpu_id;
    msg_id         = EE_K1_TO_RM_MSG;
  } else if ( cpu_id == EE_K1_BOOT_CORE ) {
    mailbox_id     = remote_id;
    msg_id         = EE_K1_FROM_RM_MSG;
  } else {
    switch ( neightbor_link ) {
      case EE_K1_FROM_NEIGHBOR_1_MSG:
      case EE_K1_FROM_NEIGHBOR_2_MSG:
      case EE_K1_FROM_NEIGHBOR_3_MSG:
        mailbox_id = remote_id;
        msg_id     = neightbor_link;
      break;
      default:
        status_type = E_OS_ID;
      break;
    }
  }

  if ( status_type == E_OK ) {
    if ( ee_k1_core_mailboxes[mailbox_id].messages[msg_id].
      msg_status != EE_K1_NO_MSG )
    {
      status_type = E_OS_STATE;
    } else {
      EE_BOOL const has_answer = (p_msg != NULL) && (ans_size > 0);
      ee_k1_core_mailboxes[mailbox_id].messages[msg_id].
       send_pmsg_or_post_size = (uintptr_t)p_msg;
      if ( has_answer ) {
        ee_k1_core_mailboxes[mailbox_id].messages[msg_id].
          msg_status = EE_K1_SND_MSG;
      } else {
        ee_k1_core_mailboxes[mailbox_id].messages[msg_id].
          msg_status = EE_K1_SND_MSG_W_OUT_ANS;
      }

      EE_k1_wmb();
      EE_k1_event_notify(cpu_id, remote_id, EE_K1_COMM_EVENT);
      EE_k1_event_idle_and_clear(cpu_id, remote_id, EE_K1_COMM_EVENT);
      if ( has_answer ) {
        /* Copy the answer to the caller */
        if ( ee_k1_core_mailboxes[mailbox_id].messages[msg_id].
          msg_status == EE_K1_POST_ANSW_MSG )
        {
          if ( ans_size <= ee_k1_core_mailboxes[mailbox_id].messages[msg_id].
            send_pmsg_or_post_size )
          {
            memcpy(p_ans, ee_k1_core_mailboxes[mailbox_id].messages[msg_id].
            message, ans_size);
          } else {
            status_type = E_OS_LIMIT;
          }
        } else {
          status_type = E_OS_VALUE;
        }
      }
      ee_k1_core_mailboxes[mailbox_id].messages[msg_id].
        msg_status = EE_K1_NO_MSG;
    }
  }
  return status_type;
}

EE_status_type EE_k1_wait_message (EE_CORE_ID remote_id, void * p_msg,
  EE_UREG msg_size)
{
  EE_status_type status_type;
  EE_UREG cpu_id = EE_k1_get_cpu_id();
  EE_UREG neightbor_link = (cpu_id ^ remote_id);
  if ( cpu_id != remote_id ) {
    if ( (remote_id == EE_K1_BOOT_CORE) || (cpu_id == EE_K1_BOOT_CORE) ||
         ((neightbor_link >= EE_K1_FROM_NEIGHBOR_1_MSG) &&
           (neightbor_link <= EE_K1_FROM_NEIGHBOR_3_MSG)) )
    {
      EE_UREG mailbox_id, msg_id;
      EE_k1_message_status msg_status;
      EE_k1_event_idle_and_clear(cpu_id, remote_id, EE_K1_COMM_EVENT);
      EE_k1_mb();
      if ( cpu_id == EE_K1_BOOT_CORE ) {
        mailbox_id = remote_id;
        msg_id     = EE_K1_TO_RM_MSG;
      } else {
        mailbox_id = cpu_id;
        msg_id     = (remote_id == EE_K1_BOOT_CORE)? EE_K1_FROM_RM_MSG:
          neightbor_link;
      }

      msg_status = ee_k1_core_mailboxes[mailbox_id].messages[msg_id].msg_status;

      if ( (msg_status ==  EE_K1_SND_MSG) ||
        (msg_status = EE_K1_SND_MSG_W_OUT_ANS) )
      {
        (*((uintptr_t *)p_msg)) = ee_k1_core_mailboxes[mailbox_id].
          messages[msg_id].send_pmsg_or_post_size;
        status_type = E_OK;
      } else if ( msg_status ==  EE_K1_POST_MSG ) {
        if ( msg_size <= ee_k1_core_mailboxes[mailbox_id].
            messages[msg_id].send_pmsg_or_post_size )
        {
          memcpy (p_msg, ee_k1_core_mailboxes[mailbox_id].
            messages[msg_id].message, msg_size );
          ee_k1_core_mailboxes[mailbox_id].messages[msg_id].
            msg_status = EE_K1_NO_MSG;
          status_type = E_OK;
        } else {
          status_type = E_OS_LIMIT;
        }
      } else {
        status_type = E_OS_STATE;
      }
    } else {
      status_type = E_OS_ID;
    }
  } else {
    status_type = E_OS_ID;
  }
  return status_type;
}

EE_status_type EE_k1_ack_message(EE_CORE_ID remote_id) {
  EE_status_type status_type;
  EE_UREG cpu_id = EE_k1_get_cpu_id();
  EE_UREG neightbor_link = (cpu_id ^ remote_id);
  if ( cpu_id != remote_id ) {
    if ( (remote_id == EE_K1_BOOT_CORE) || (cpu_id == EE_K1_BOOT_CORE) ||
         ((neightbor_link >= EE_K1_FROM_NEIGHBOR_1_MSG) &&
           (neightbor_link <= EE_K1_FROM_NEIGHBOR_3_MSG)) )
    {
      EE_UREG mailbox_id, msg_id;
      EE_k1_message_status msg_status;
      if ( cpu_id == EE_K1_BOOT_CORE ) {
        mailbox_id = remote_id;
        msg_id     = EE_K1_TO_RM_MSG;
      } else {
        mailbox_id = cpu_id;
        msg_id     = (remote_id == EE_K1_BOOT_CORE)? EE_K1_FROM_RM_MSG:
          neightbor_link;
      }
      EE_k1_mb(); /* <-- This should be better an invalidation on the
                         message cache row */
      msg_status = ee_k1_core_mailboxes[mailbox_id].messages[msg_id].msg_status;

      if ( msg_status == EE_K1_SND_MSG_W_OUT_ANS ) {
        ee_k1_core_mailboxes[mailbox_id].messages[msg_id].
          msg_status = EE_K1_SND_MSG_ACK;
        EE_k1_wmb(); /* <-- This should be better a commit on the
                            message cache row */
        EE_k1_event_notify(cpu_id, remote_id, EE_K1_COMM_EVENT);
        status_type = E_OK;
      } else {
        status_type = E_OS_STATE;
      }
    } else {
      status_type = E_OS_ID;
    }
  } else {
    status_type = E_OS_ID;
  }
  return status_type;
}

EE_status_type EE_k1_post_answer_message ( EE_CORE_ID remote_id, void *p_msg,
  EE_mem_size msg_size)
{
  EE_status_type status_type;
  EE_UREG cpu_id = EE_k1_get_cpu_id();
  EE_UREG neightbor_link = (cpu_id ^ remote_id);

  if ( (p_msg == NULL) || (msg_size == 0) ) {
    status_type = E_OS_PARAM_POINTER;
  } else if ( msg_size > EE_K1_MESSAGE_SIZE ) {
    status_type = E_OS_LIMIT;
  } else {
    if ( cpu_id != remote_id ) {
      if ( (remote_id == EE_K1_BOOT_CORE) || (cpu_id == EE_K1_BOOT_CORE) ||
           ((neightbor_link >= EE_K1_FROM_NEIGHBOR_1_MSG) &&
           (neightbor_link <= EE_K1_FROM_NEIGHBOR_3_MSG)) )
      {
        EE_UREG mailbox_id, msg_id;
        EE_k1_message_status msg_status;
        if ( cpu_id == EE_K1_BOOT_CORE ) {
          mailbox_id = remote_id;
          msg_id     = EE_K1_TO_RM_MSG;
        } else {
          mailbox_id = cpu_id;
          msg_id     = (remote_id == EE_K1_BOOT_CORE)? EE_K1_FROM_RM_MSG:
            neightbor_link;
        }

        EE_k1_mb(); /* <-- This should be better an invalidation on the
                           message cache row */
        msg_status = ee_k1_core_mailboxes[mailbox_id].messages[msg_id].
          msg_status;

        if ( msg_status == EE_K1_SND_MSG ) {
          ee_k1_core_mailboxes[mailbox_id].messages[msg_id].
            msg_status = EE_K1_POST_ANSW_MSG;
          memcpy (ee_k1_core_mailboxes[mailbox_id].messages[msg_id].message,
            p_msg, msg_size);
          EE_k1_wmb(); /* <-- This should be better a commit on the
                              message cache row */
          EE_k1_event_notify(cpu_id, remote_id, EE_K1_COMM_EVENT);
          status_type = E_OK;
        } else {
          status_type = E_OS_STATE;
        }
      } else {
        status_type = E_OS_ID;
      }
    } else {
      status_type = E_OS_ID;
    }
  }
  return status_type;
}
