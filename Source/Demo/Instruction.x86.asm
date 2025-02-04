.686P
.XMM
.model flat, stdcall

include Instruction.inc

.code

SimpleInstructionFunc1X86 PROC

    xor     eax, eax
    mov     eax, PRESET_RETURN_VALUE
    ret

SimpleInstructionFunc1X86 ENDP

.data

$PublicFuncAddr SimpleInstructionFunc1X86

END