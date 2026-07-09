
.section .text.start
.global _start
 
_start:
    la   sp, _stack_top

    la   t0, trap_handler
    csrw mtvec, t0
 
    /* copy .data: flash (_data_lma) -> RAM (_data_start.._data_end) */
    la   t0, _data_lma
    la   t1, _data_start
    la   t2, _data_end
copy_data:
    bge  t1, t2, copy_data_done
    lw   t3, 0(t0)
    sw   t3, 0(t1)
    addi t0, t0, 4
    addi t1, t1, 4
    j    copy_data
copy_data_done:
 
    /* zero .bss: _bss_start.._bss_end */
    la   t1, _bss_start
    la   t2, _bss_end
zero_bss:
    bge  t1, t2, zero_bss_done
    sw   zero, 0(t1)
    addi t1, t1, 4
    j    zero_bss
zero_bss_done:
 
    call main
 
hang:
    j hang
 
.align 4 /* no vectoring? */
.global trap_handler
trap_handler:
    csrr a0, mcause
    csrr a1, mepc
    call report_trap
trap_hang:
    j trap_hang
