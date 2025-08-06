.section .data
cur_alt_inp_msg: .asciz "Current altitude: "
air_alt_inp_msg: .asciz "Airport altitude: "
cur_spd_inp_msg: .asciz "Current Speed: "
avg_spd_inp_msg: .asciz "Average descend speed: "
wnd_spd_inp_msg: .asciz "Wind speed: "
wnd_dir_inp_msg: .asciz "Tail or head wind [1 for tail | 0 for head]: "

spacer_out:  .asciz "------------------------------"
alt_out_msg: .asciz "Altitude to lose: %d\n"
spd_out_msg: .asciz "Speed: %d\n"
wnd_out_msg: .asciz "Wind:  %d\n"
dst_out_msg: .asciz "Total distance: %d\n" # TODO: float

scanf_fmt_str: .asciz "%d"

.section .bss
inp_num: .skip 4

.section .text
.extern _stdin
.global main
main:
    # stack alignment 8 bytes, and 32 bytes because a function needs them according to Microsoft x64 calling convention
    sub $40, %rsp

    lea cur_alt_inp_msg(%rip), %rcx
    call get_input
    mov %eax, %r14d

    lea air_alt_inp_msg(%rip), %rcx
    call get_input
    mov %eax, %r15d

    # calc altitude to lose base
    # ((current_alt - airport_alt) / 100) / 3
    sub %r15d, %r14d
    mov $0, %edx
    mov %r14d, %eax
    mov $100, %r14d
    div %r14d
    mov $0, %edx
    mov $3, %r14d
    div %r14d
    mov %eax, %r12d # r12 is altitude_to_lose

    lea cur_spd_inp_msg(%rip), %rcx
    call get_input
    mov %eax, %r14d

    lea avg_spd_inp_msg(%rip), %rcx
    call get_input
    mov %eax, %r15d

    # calc speed knots
    # (current_speed - avarage_descend_speed) / 10
    sub %r15d, %r14d
    mov $0, %edx
    mov %r14d, %eax
    mov $10, %r14d
    div %r14d
    mov %eax, %r13d # r13 is speed_knots

    # calc descend distance
    # altitude_to_lose + speed_knots
    add %r12d, %eax
    mov %eax, %r14d # r14 is descend_distance

    # calc wind speed
    # wind_speed / 10
    lea wnd_spd_inp_msg(%rip), %rcx
    call get_input
    mov $0, %edx
    mov $10, %ebx
    div %ebx # wind_speed already in eax
    mov %eax, %edi # rdi is wind_speed

    lea wnd_dir_inp_msg(%rip), %rcx
    call get_input
    cmp $0, %eax
    je head_wind
tail_wind:
    add %eax, %r14d
    jmp wind_done
head_wind:
    sub %eax, %r14d
wind_done:

    lea spacer_out(%rip), %rcx
    call puts

    lea alt_out_msg(%rip), %rcx
    mov %r12d, %edx
    mov $0, %eax # no floating point args
    call printf

    lea spd_out_msg(%rip), %rcx
    mov %r13d, %edx
    mov $0, %eax # no floating point args
    call printf

    lea wnd_out_msg(%rip), %rcx
    mov %edi, %edx
    mov $0, %eax # no floating point args
    call printf

    lea dst_out_msg(%rip), %rcx
    mov %r14d, %edx
    mov $0, %eax # no floating point args
    call printf

    add $40, %rsp
    mov $0, %rax         # Return 0
    ret


# one argument, the msg
get_input:
    push %r15
    sub $32, %rsp
    mov %rcx, %r15 # save message str
    jmp get_input_scanf

get_input_getchar:
    call getchar # clear input buffer
    cmp $10, %al # check for \n
    jne get_input_getchar

get_input_scanf:
    mov %r15, %rcx # restore message str
    mov $0, %eax # no floating point args
    call printf  # str is in rcx

    lea scanf_fmt_str(%rip), %rcx
    lea inp_num(%rip), %rdx
    mov $0, %rax
    call scanf

    cmp $0, %rax
    je get_input_getchar

    mov inp_num(%rip), %eax

    add $40, %rsp
    ret
