/* Addendum to ReactOS NDK and helper macros on KNSoft.NDK */

#pragma once

#if defined(_X86_)
#define CONTEXT_PC Eip
#elif defined(_AMD64_)
#define CONTEXT_PC Rip
#elif defined(_ARM64_)
#define CONTEXT_PC Pc
#endif

#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))
#define PtrOffset(B,O) ((ULONG)((ULONG_PTR)(O) - (ULONG_PTR)(B)))

#define KB_TO_BYTES(x) ((x) * 1024UL)
#define MB_TO_KB(x) ((x) * 1024UL)
#define MB_TO_BYTES(x) (KB_TO_BYTES(MB_TO_KB(x)))
#define GB_TO_MB(x) ((x) * 1024UL)
#define GB_TO_BYTES(x) (MB_TO_BYTES(GB_TO_MB(x)))

#define MM_LOWEST_USER_ADDRESS ((PVOID)0x10000)

#if defined(_WIN64)

/* [0x00007FF7FFFF0000 ... 0x00007FFFFFFF0000], 32G */

#define MI_ASLR_BITMAP_SIZE 0x10000
#define MI_ASLR_HIGHEST_SYSTEM_RANGE_ADDRESS ((PVOID)0x00007FFFFFFF0000ULL)

#else

/* [0x50000000 ... 0x78000000], 640M */

#define MI_ASLR_BITMAP_SIZE 0x500
#define MI_ASLR_HIGHEST_SYSTEM_RANGE_ADDRESS ((PVOID)0x78000000UL)

#endif

#define NtGetCurrentProcessId() (NtCurrentTeb()->ClientId.UniqueProcess)
#define NtGetCurrentThreadId() (NtCurrentTeb()->ClientId.UniqueThread)
#define NtGetProcessHeap() (NtCurrentPeb()->ProcessHeap)
