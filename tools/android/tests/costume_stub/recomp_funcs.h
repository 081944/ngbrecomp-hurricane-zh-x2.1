/* Guest ABI shim for executing the real costume loader against controlled jobs. */
#pragma once
#include <stdint.h>
extern uint8_t guest[];
extern uint32_t eax, ecx, edx, esp, ebx, esi, edi, g_ebp, g_seh_ebp;
typedef union { float f[4]; uint32_t u[4]; } RecompXmm;
extern RecompXmm g_xmm0, g_xmm1, g_xmm2, g_xmm3, g_xmm4, g_xmm5, g_xmm6, g_xmm7;
extern double g_fp_stack[8];
extern int g_fp_top;
#define g_eax eax
#define g_ecx ecx
#define g_edx edx
#define g_esp esp
#define g_ebx ebx
#define g_esi esi
#define g_edi edi
#define XBOX_PTR(a) (guest + (uint32_t)(a))
#define MEM8(a) (*(uint8_t *)XBOX_PTR(a))
#define MEM16(a) (*(uint16_t *)XBOX_PTR(a))
#define MEM32(a) (*(uint32_t *)XBOX_PTR(a))
#define MEMF(a) (*(float *)XBOX_PTR(a))
#define LO8(a) ((uint8_t)(a))
#define SET_LO8(a,b) ((a) = ((a) & 0xFFFFFF00u) | (uint8_t)(b))
#define PUSH32(s,v) do { (s) -= 4; MEM32(s) = (v); } while (0)
#define POP32(s,v) do { (v) = MEM32(s); (s) += 4; } while (0)
#define RECOMP_ENTRY(a) ((void)0)
#define RECOMP_ABI_CALL(a,f) f()
#define CMP_AE(a,b) ((a) >= (b))
#define CMP_EQ(a,b) ((a) == (b))
#define CMP_NE(a,b) ((a) != (b))
#define CMP_L(a,b) ((a) < (b))
void sub_002FC6F0(void);
void sub_001718E0(void);
void sub_00173880(void);
void sub_00172D60(void);
void sub_002F62C0(void);
void sub_003077E0(void);
void sub_0016C5F0(void);
void sub_0016C690(void);
void sub_0016C570(void);
void sub_002EA5E0(void);
void sub_002F6300(void);
void sub_002FE1D0(void);
void sub_001919F0(void);
void sub_00188C40(void);
void sub_00188E70(void);
void sub_0018E8B0(void);
void sub_0018EDF0(void);
void sub_00189EB0(void);
void sub_0018B110(void);
void sub_0018B380(void);
void sub_0018D290(void);
void sub_00191C30(void);
