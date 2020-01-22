%include "asm.inc"

; Public-domain/CC0 - see https://creativecommons.org/publicdomain/zero/1.0/

global FUNC(d_round)
global FUNC(d_sqrt)

%ifdef JPR_ALIAS_STDLIB
global FUNC(round)
global FUNC(sqrt)
%endif

section .text align=16

FUNC(d_round):
%if JPR_BITS == 64
    cvtsd2si rax, xmm0
    cvtsi2sd xmm0, rax
%else
    fld qword [esp+4]
    sub esp,8
    fistp qword [esp]
    fild qword [esp]
    add esp,8
%endif
    ret

FUNC(d_sqrt):
%if JPR_BITS == 64
    sqrtsd xmm0,xmm0
%else
%ifdef USE_SSE2
    movsd xmm0,[esp+4]
    sqrtsd xmm0,xmm0
    sub esp,8
    movsd [esp],xmm0
    fld qword[esp]
    add esp,8
%else
    fld qword [esp+4]
    fsqrt
%endif
%endif
    ret

%ifdef JPR_ALIAS_STDLIB
FUNC(round) EQU FUNC(d_round)
FUNC(sqrt) EQU FUNC(d_sqrt)
%endif
