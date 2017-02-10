/* Copyright (C) 2010, 2017 DEI - Universita' di Bologna
   Contributed by:
   Alessandro Capotondi <alessandro.capotondi@unibo.it>
   Daniele Cesarini <daniele.cesarini@unibo.it>
   Andrea Marongiu  <a.marongiu@unibo.it>
   Giuseppe Tagliavini <giuseppe.tagliavini@unibo.it>
*/

/* This file is part of the GNU OpenMP Library (libgomp).

   Libgomp is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   Libgomp is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */
          
/*	This file includes tasking support functions for the Kalray MPPA 256 BSP */

#define MOS_VC_REG_PS 40
# unsigned int start_context(void *task, void *stack, void *ctx)
# r0: task, r1: stack, r2: ctx
    .text
    .align 8
	.globl start_context
	.type  start_context, @function
start_context:
#	stmia	r2!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14}
#	mov		sp, r1
#	ldr 	pc,	=gomp_start

   	sd        0[$r2] = $r8r9
   	;;
   	
   	make         $r8 = _scoreboard_start
   	get          $r9 = $pcr  	
   	sd        8[$r2] = $r10r11
   	;;

   	extfz    $r9, $r9, 15, 11
   	sd       16[$r2] = $r12r13
   	;;

   	sll      $r9, $r9, 8
   	sd       24[$r2] = $r14r15
   	;;

   	add      $r8, $r8, $r9
   	sd       32[$r2] = $r16r17
   	;;

   	sd       40[$r2] = $r18r19
   	;;

   	sd       48[$r2] = $r20r21
   	;;

   	sd       56[$r2] = $r22r23
   	;;

   	sd       64[$r2] = $r24r25
   	;;

   	get         $r24 = $ra
   	sd       72[$r2] = $r26r27
   	;;

   	lw          $r25 = MOS_VC_REG_PS[$r8]
   	;;
   	
   	sd       80[$r2] = $r28r29
   	;;

   	sd       88[$r2] = $r30r31
   	;;

   	sd       96[$r2] = $r24r25
   	;;
   	
   	copy        $r12 = $r1
   	;;

   	add     $r12, $r12, -16
   	goto    gomp_start
   	;;


# unsigned int swap_context(void *, void *)
# r0: ctx, r1: ctx
    .text
    .align 8
	.globl swap_context
	.type  swap_context, @function
swap_context:
#	stmia	r1!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14}
#	ldmia	r0!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14}
#	mov 	pc, lr

    sd        0[$r1] = $r8r9
   	;;
   	
   	make         $r8 = _scoreboard_start
   	get          $r9 = $pcr  	
   	sd        8[$r1] = $r10r11
   	;;

   	extfz    $r9, $r9, 15, 11
   	sd       16[$r1] = $r12r13
   	;;

   	sll      $r9, $r9, 8
   	sd       24[$r1] = $r14r15
   	;;

   	add      $r8, $r8, $r9
   	sd       32[$r1] = $r16r17
   	;;

   	sd       40[$r1] = $r18r19
   	;;

   	sd       48[$r1] = $r20r21
   	;;

   	sd       56[$r1] = $r22r23
   	;;

   	sd       64[$r1] = $r24r25
   	;;

   	get         $r24 = $ra
   	sd       72[$r1] = $r26r27
   	;;

   	lw          $r25 = MOS_VC_REG_PS[$r8]
   	;;
   	
   	sd       80[$r1] = $r28r29
   	;;

   	sd       88[$r1] = $r30r31
   	;;

   	sd       96[$r1] = $r24r25
   	;;
   	


   	make         $r8 = _scoreboard_start
   	get          $r9 = $pcr  
   	ld       $r10r11 = 96[$r0]	
   	;;     
           
   	set          $ra = $r10
   	extfz        $r9 = $r9, 15, 11
   	ld       $r30r31 = 88[$r0]
   	;;

   	sll      $r9, $r9, 8
   	ld       $r28r29 = 80[$r0]
   	;;
   	
   	add          $r8 = $r8, $r9
   	ld       $r26r27 = 72[$r0]
   	;;    

    sw    MOS_VC_REG_PS[$r8] = $r11
    ;;    
                       
   	ld       $r24r25 = 64[$r0]
   	;;                 
                       
   	ld       $r22r23 = 56[$r0]
   	;;                 
   	   	               
   	ld       $r20r21 = 48[$r0]
   	;;                 
                       
   	ld       $r18r19 = 40[$r0]
   	;;                 
                       
   	ld       $r16r17 = 32[$r0]
   	;;                 
                       
   	ld       $r14r15 = 24[$r0]
   	;;                 
                       
   	ld       $r12r13 = 16[$r0]
   	;;                 
                       
   	ld       $r10r11 =  8[$r0]
   	;;                 
   	                   
   	ld         $r8r9 = 0[$r0]
   	;;
   	
   	ret
   	;;


# unsigned int load_context(void *)
# r0: ctx
    .text
    .align 8
	.globl load_context
	.type  load_context, @function
load_context:
#	ldmia	r0!, {r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14}
#	mov 	pc, lr

   	make         $r8 = _scoreboard_start
   	get          $r9 = $pcr  
   	ld       $r10r11 = 96[$r0]	
   	;;     
           
   	set          $ra = $r10
   	extfz        $r9 = $r9, 15, 11
   	ld       $r30r31 = 88[$r0]
   	;;

   	sll      $r9, $r9, 8
   	ld       $r28r29 = 80[$r0]
   	;;
   	
   	add          $r8 = $r8, $r9
   	ld       $r26r27 = 72[$r0]
   	;;    

    sw    MOS_VC_REG_PS[$r8] = $r11
    ;;
             
                       
   	ld       $r24r25 = 64[$r0]
   	;;                 
                       
   	ld       $r22r23 = 56[$r0]
   	;;                 
   	   	               
   	ld       $r20r21 = 48[$r0]
   	;;                 
                       
   	ld       $r18r19 = 40[$r0]
   	;;                 
                       
   	ld       $r16r17 = 32[$r0]
   	;;                 
                       
   	ld       $r14r15 = 24[$r0]
   	;;                 
                       
   	ld       $r12r13 = 16[$r0]
   	;;                 
                       
   	ld       $r10r11 =  8[$r0]
   	;;                 
   	                   
   	ld         $r8r9 = 0[$r0]
   	;;
   	
   	ret
   	;;