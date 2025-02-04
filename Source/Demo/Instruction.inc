IFDEF _M_IX86

SIZE_T equ <DWORD>
PRESET_RETURN_VALUE equ 012345678h

ELSE

SIZE_T equ <QWORD>
PRESET_RETURN_VALUE equ 01234567890ABCDEFh

ENDIF

$PublicFuncAddr MACRO Func
    align 4
    @CatStr(Func, <Ptr SIZE_T offset >, Func)
    PUBLIC @CatStr(Func, <Ptr>)
ENDM
