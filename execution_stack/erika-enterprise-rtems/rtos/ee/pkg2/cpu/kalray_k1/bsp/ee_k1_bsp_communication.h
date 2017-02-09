#ifndef EE_K1_BSP_COMMUNICATION_H_
#define EE_K1_BSP_COMMUNICATION_H_

#include "ee_k1_bsp.h"

/**
 * Event types between PE and RM, and between two PE
 */
enum {
  EE_K1_AMP_EVENT             = 0U,
  EE_K1_AMP_ACK_EVENT         = 1U,
  EE_K1_COMM_EVENT            = 3U,
  EE_K1_NUM_TO_FROM_RM_EVENT  = 4U,
  EE_K1_NUM_EVENT             = 8U,
  INVALID_EVENT               = ((EE_UREG)-1)
};

typedef __k1_eventmask_t EE_k1_event_mask;

/**
 * @fn static inline void EE_k1_event_notify(EE_CORE_ID self_id,
 *     EE_CORE_ID dst_id,  uint8_t ev_id)
 * @brief Notify event line(s) based on ID
 * @param self_id self processor id (processor notifying the event)
 * @param remote_id remote processor id (processor waiting for the event)
 * @param ev_id event id
 */
static inline void EE_k1_event_notify (EE_CORE_ID self_id, EE_CORE_ID remote_id,
  uint8_t ev_id)
{
  __k1_event_notify1_proc(self_id, remote_id, ev_id);
}

/**
 * @fn static inline EE_UREG EE_k1_event_get_status(EE_CORE_ID self_id,
 *    EE_CORE_ID remote_id, uint8_t ev_id)
 * @brief get the state of an event
 * @param self_id self processor id
 * @param remote_id remote processor id
 * @param ev_id event id
 */

EE_INLINE__ EE_UREG EE_k1_event_get_status ( EE_CORE_ID self_id,
  EE_CORE_ID remote_id, uint8_t ev_id )
{
  EE_UREG event_status;

  if ( (self_id >= EE_K1_CORE_NUMBER) || (remote_id >= EE_K1_CORE_NUMBER) ) {
    event_status = INVALID_EVENT;
  } else {
    if ( self_id >= EE_K1_PE_NUMBER ) {
      if ( ev_id < EE_K1_NUM_TO_FROM_RM_EVENT ) {
        EE_UREG event_pos;
        if ( remote_id >= EE_K1_PE_NUMBER ) {
          /* RM -> RM */
          event_pos = remote_id + 16U;
        } else {
          /* RM -> PE */
          event_pos = remote_id;
        }
        event_status = __k1_event_get_num(ev_id) & (1U << event_pos);
      } else {
        event_status = INVALID_EVENT;
      }
    } else {
      if ( remote_id >= EE_K1_PE_NUMBER ) {
        if ( ev_id < EE_K1_NUM_TO_FROM_RM_EVENT ) {
          /* PE -> RM */
          event_status = __k1_event_get_num(ev_id) & (1U << remote_id);
        } else {
          event_status = INVALID_EVENT;
        }
      } else {
        if ( (ev_id < EE_K1_NUM_EVENT) &&
          _K1_EVENT_PE2PE_ISPOSSIBLE(self_id, remote_id) )
        {
          /* PE -> PE */
          /* WARNING In this case we cannot be sure that the event sender is
           * exactly "remote id". It could be anyone of the neighbor enabled to
           * send event */
          event_status = __builtin_k1_get(_K1_SFR_EV4) & (1U << ev_id);
        } else {
          event_status = INVALID_EVENT;
        }
      }
    }
  }
  return event_status;
}

/**
 * @fn static inline void EE_k1_event_wait_and_clear(EE_CORE_ID self_id,
 *   EE_CORE_ID remote_id, uint8_t ev_id)
 * @brief Wait and clear event line(s) based on ID
 * @param self_id self processor id (processor waiting for the event)
 * @param remote_id remote processor id (processor notifying the event)
 * @param ev_id event id
 */
static inline void EE_k1_event_wait_and_clear ( EE_CORE_ID self_id,
  EE_CORE_ID remote_id, uint8_t ev_id )
{
  __k1_event_waitclr1_proc(self_id, remote_id, ev_id);
}

/**
 * \fn static inline void EE_k1_event_clear(EE_CORE_ID self_id,
 *   EE_CORE_ID remote_id, uint8_t ev_id)
 * \brief Clear event line(s) based on ID
 * \param self_id self processor id
 * \param remote_id remote processor id
 * \param ev_id event id
 */
static inline void EE_k1_event_clear (EE_CORE_ID self_id,
  EE_CORE_ID remote_id, uint8_t ev_id)
{
  __k1_event_clear1_proc(self_id, remote_id, ev_id);
}

/**
 * @fn static inline void EE_k1_event_wait_and_clear(EE_CORE_ID self_id,
 *   EE_CORE_ID remote_id, uint8_t ev_id)
 * @brief Wait and clear event line(s) based on ID
 * @param self_id self processor id (processor waiting for the event)
 * @param remote_id remote processor id (processor notifying the event)
 * @param ev_id event id
 */
static inline void EE_k1_event_idle_and_clear ( EE_CORE_ID self_id,
  EE_CORE_ID remote_id, uint8_t ev_id )
{
  EE_UREG event = EE_k1_event_get_status (self_id, remote_id,
    ev_id);
  while ( event == 0 ) {
    EE_k1_idle_enter();
    event = EE_k1_event_get_status(self_id, remote_id, ev_id);
  }
  EE_k1_event_clear(self_id, remote_id, ev_id);
  EE_k1_mb();
}

/** Generic Structure to handle syscalls */
//typedef struct EE_k1_message_tag {
//  EE_message_type  type;
//  EE_UREG          number;
//  long             ret;
//  int              notify;
//  unsigned int     ret_errno;
//} __attribute__ ((aligned(_K1_DCACHE_LINE_SIZE))) EE_k1_message;

typedef enum EE_k1_message_id_tag {
  EE_K1_TO_RM_MSG,
  EE_K1_FROM_RM_MSG,
  EE_K1_FROM_NEIGHBOR_1_MSG,
  EE_K1_FROM_NEIGHBOR_2_MSG,
  EE_K1_FROM_NEIGHBOR_3_MSG,
  EE_K1_SIZE_MSG
} EE_k1_maessage_id;

typedef enum EE_k1_message_status_tag {
  EE_K1_NO_MSG,
  EE_K1_SND_MSG,
  EE_K1_SND_MSG_W_OUT_ANS,
  EE_K1_SND_MSG_ACK,
  EE_K1_POST_MSG,
  EE_K1_POST_ANSW_MSG
} EE_k1_message_status;

#define EE_K1_MESSAGE_SIZE\
  (_K1_DCACHE_LINE_SIZE - 5U /* offsetof(struct EE_k1_message_tag, message) */)

typedef struct EE_k1_message_tag {
  uint8_t    message[EE_K1_MESSAGE_SIZE];
  uint8_t    msg_status;
  uintptr_t  send_pmsg_or_post_size;
} __attribute__ ((aligned(_K1_DCACHE_LINE_SIZE))) EE_k1_message;

typedef struct EE_k1_mailbox_tag {
  EE_k1_message messages[EE_K1_SIZE_MSG];
} __attribute__ ((aligned(_K1_DCACHE_LINE_SIZE))) EE_k1_mailbox;

/**
 * Send a message to the RM or a neighbor core and wait for the answer
 * @param remote_id
 * @param p_msg pointer to arguments of the command
 * @param pp_ans Poiter to receive the answer
 */
EE_status_type EE_k1_send_message (EE_CORE_ID remote_id, void *p_msg,
  void *p_ans, EE_mem_size ans_size);

/**
 * Post an answer message to the RM or a neighbor core.
 * @param remote_id
 * @param p_msg pointer to arguments of the command
 * @param msg_size of the message
 */
EE_status_type EE_k1_post_answer_message (EE_CORE_ID remote_id, void *p_msg,
  EE_mem_size msg_size);

/**
 * Ackwoledge a message sent by the RM or a neighbor core.
 * @param remote_id
 * @param msg_size of the message
 */
EE_status_type EE_k1_ack_message(EE_CORE_ID remote_id);

/**
 * Post a message to the RM or a neighbor core.
 * @param remote_id
 * @param p_msg pointer to arguments of the command
 * @param msg_size of the message
 */
EE_status_type EE_k1_post_message (EE_CORE_ID remote_id, void *p_msg,
  EE_mem_size msg_size);

/**
 * Wait a message from the RM or a neighbor core
 * @param [in]  remote_id
 * @param [out] p_msg pointer to the return message pointer
 */
EE_status_type EE_k1_wait_message(EE_CORE_ID remote_id, void * p_msg,
  EE_mem_size EE_mem_size);

#endif /* EE_K1_BSP_COMMUNICATION_H_ */
