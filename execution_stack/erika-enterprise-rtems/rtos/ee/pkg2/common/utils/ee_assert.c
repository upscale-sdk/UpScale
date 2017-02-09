/* ###*B*###
 * ERIKA Enterprise - a tiny RTOS for small microcontrollers
 *
 * Copyright (C) 2002-2008  Evidence Srl
 *
 * This file is part of ERIKA Enterprise.
 *
 * ERIKA Enterprise is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation,
 * (with a special exception described below).
 *
 * Linking this code statically or dynamically with other modules is
 * making a combined work based on this code.  Thus, the terms and
 * conditions of the GNU General Public License cover the whole
 * combination.
 *
 * As a special exception, the copyright holders of this library give you
 * permission to link this code with independent modules to produce an
 * executable, regardless of the license terms of these independent
 * modules, and to copy and distribute the resulting executable under
 * terms of your choice, provided that you also meet, for each linked
 * independent module, the terms and conditions of the license of that
 * module.  An independent module is a module which is not derived from
 * or based on this library.  If you modify this code, you may extend
 * this exception to your version of the code, but you are not
 * obligated to do so.  If you do not wish to do so, delete this
 * exception statement from your version.
 *
 * ERIKA Enterprise is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with ERIKA Enterprise; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 * ###*E*### */

/*
 * Author: 2003-2004 Paolo Gai
 * CVS: $Id: ee_assert.c,v 1.1.1.1 2004/11/05 16:03:03 pj Exp $
 */

#include "ee_assert.h"

#if (!defined(ASSERT_LENGTH))
#define ASSERT_LENGTH 1U
#endif

EE_TYPEASSERTVALUE __attribute__((weak)) EE_assertions[ASSERT_LENGTH];

#ifndef __PRIVATE_ASSERT__
EE_TYPEASSERTVALUE EE_assert(EE_TYPEASSERT id,
           int test,
           EE_TYPEASSERT prev)
{
  /* I can use id into an assertion only once */
  if (EE_assertions[id] != EE_ASSERT_INITVALUE) {
    EE_assertions[id] = EE_ASSERT_ALREADYUSED;
    return EE_ASSERT_ALREADYUSED;
  }

  if (test) {
    if (prev != EE_ASSERT_NIL) {
      if (EE_assertions[prev] == EE_ASSERT_YES) {
  /* test is true and the prev assertion is YES */
  EE_assertions[id] = EE_ASSERT_YES;
  return EE_ASSERT_YES;
      }
      else {
  /* test is true but the prev assertion is != YES */
  EE_assertions[id] = EE_ASSERT_NO;
  return EE_ASSERT_NO;
      }
    } else {
      /* test is true and prev is EE_ASSERT_NIL */
      EE_assertions[id] = EE_ASSERT_YES;
      return EE_ASSERT_YES;
    }
  } else {
    /* test is false */
    EE_assertions[id] = EE_ASSERT_NO;
    return EE_ASSERT_NO;
  }
}
#endif

#ifndef __PRIVATE_ASSERT_OR__
EE_TYPEASSERTVALUE EE_assert_or(EE_TYPEASSERT id,
        EE_TYPEASSERT prev1,
        EE_TYPEASSERT prev2)
{
  /* I can use id into an assertion only once */
  if (EE_assertions[id] != EE_ASSERT_INITVALUE) {
    EE_assertions[id] = EE_ASSERT_ALREADYUSED;
    return EE_ASSERT_ALREADYUSED;
  }

  if ((EE_assertions[prev1] == EE_ASSERT_YES) ||
      (EE_assertions[prev2] == EE_ASSERT_YES)) {
    EE_assertions[id] = EE_ASSERT_YES;
    return EE_ASSERT_YES;
  }
  else {
    EE_assertions[id] = EE_ASSERT_NO;
    return EE_ASSERT_NO;
  }
}
#endif

#ifndef __PRIVATE_ASSERT_AND__
EE_TYPEASSERTVALUE EE_assert_and(EE_TYPEASSERT id,
         EE_TYPEASSERT prev1,
         EE_TYPEASSERT prev2)
{
  /* I can use id into an assertion only once */
  if (EE_assertions[id] != EE_ASSERT_INITVALUE) {
    EE_assertions[id] = EE_ASSERT_ALREADYUSED;
    return EE_ASSERT_ALREADYUSED;
  }

  if ((EE_assertions[prev1] == EE_ASSERT_YES) &&
      (EE_assertions[prev2] == EE_ASSERT_YES)) {
    EE_assertions[id] = EE_ASSERT_YES;
    return EE_ASSERT_YES;
  }
  else {
    EE_assertions[id] = EE_ASSERT_NO;
    return EE_ASSERT_NO;
  }
}
#endif

#ifndef __PRIVATE_ASSERT_RANGE__
EE_TYPEASSERTVALUE EE_assert_range(EE_TYPEASSERT id,
           EE_TYPEASSERT begin,
           EE_TYPEASSERT end)
{
  EE_TYPEASSERT i;

  for (i=begin; i<=end; i++) {
    if (EE_assertions[i] != EE_ASSERT_YES) {
      EE_assertions[id] = EE_ASSERT_NO;
      return EE_ASSERT_NO;
    }
  }

  EE_assertions[id] = EE_ASSERT_YES;
  return EE_ASSERT_YES;
}
#endif

#ifndef __PRIVATE_ASSERT_ALL__
EE_TYPEASSERTVALUE EE_assert_last(void)
{
  return EE_assertions[0];
}
#endif
