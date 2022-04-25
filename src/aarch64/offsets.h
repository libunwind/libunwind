/* Linux-specific definitions: */

/* Define various structure offsets to simplify cross-compilation.  */

/* Offsets for AArch64 Linux "ucontext_t":  */

#define LINUX_UC_FLAGS_OFF      0x0
#define LINUX_UC_LINK_OFF       0x8
#define LINUX_UC_STACK_OFF      0x10
#define LINUX_UC_SIGMASK_OFF    0x28
#define LINUX_UC_MCONTEXT_OFF   0xb0

/* Offsets for AArch64 Linux "struct sigcontext":  */

#define LINUX_SC_FAULTADDRESS_OFF       0x00
#define LINUX_SC_X0_OFF         0x008
#define LINUX_SC_X1_OFF         0x010
#define LINUX_SC_X2_OFF         0x018
#define LINUX_SC_X3_OFF         0x020
#define LINUX_SC_X4_OFF         0x028
#define LINUX_SC_X5_OFF         0x030
#define LINUX_SC_X6_OFF         0x038
#define LINUX_SC_X7_OFF         0x040
#define LINUX_SC_X8_OFF         0x048
#define LINUX_SC_X9_OFF         0x050
#define LINUX_SC_X10_OFF        0x058
#define LINUX_SC_X11_OFF        0x060
#define LINUX_SC_X12_OFF        0x068
#define LINUX_SC_X13_OFF        0x070
#define LINUX_SC_X14_OFF        0x078
#define LINUX_SC_X15_OFF        0x080
#define LINUX_SC_X16_OFF        0x088
#define LINUX_SC_X17_OFF        0x090
#define LINUX_SC_X18_OFF        0x098
#define LINUX_SC_X19_OFF        0x0a0
#define LINUX_SC_X20_OFF        0x0a8
#define LINUX_SC_X21_OFF        0x0b0
#define LINUX_SC_X22_OFF        0x0b8
#define LINUX_SC_X23_OFF        0x0c0
#define LINUX_SC_X24_OFF        0x0c8
#define LINUX_SC_X25_OFF        0x0d0
#define LINUX_SC_X26_OFF        0x0d8
#define LINUX_SC_X27_OFF        0x0e0
#define LINUX_SC_X28_OFF        0x0e8
#define LINUX_SC_X29_OFF        0x0f0
#define LINUX_SC_X30_OFF        0x0f8
#define LINUX_SC_SP_OFF         0x100
#define LINUX_SC_PC_OFF         0x108
#define LINUX_SC_PSTATE_OFF     0x110
#define LINUX_SC_RESERVED_OFF   0x120
/* { u32 magic; u32 size; u32 fpsr; u32 fpcr; uint128 vregs[]; } */
#define LINUX_SC_V0_OFF         (LINUX_SC_RESERVED_OFF + 0x010)
#define LINUX_SC_V1_OFF         (LINUX_SC_RESERVED_OFF + 0x020)
#define LINUX_SC_V2_OFF         (LINUX_SC_RESERVED_OFF + 0x030)
#define LINUX_SC_V3_OFF         (LINUX_SC_RESERVED_OFF + 0x040)
#define LINUX_SC_V4_OFF         (LINUX_SC_RESERVED_OFF + 0x050)
#define LINUX_SC_V5_OFF         (LINUX_SC_RESERVED_OFF + 0x060)
#define LINUX_SC_V6_OFF         (LINUX_SC_RESERVED_OFF + 0x070)
#define LINUX_SC_V7_OFF         (LINUX_SC_RESERVED_OFF + 0x080)
#define LINUX_SC_V8_OFF         (LINUX_SC_RESERVED_OFF + 0x090)
#define LINUX_SC_V9_OFF         (LINUX_SC_RESERVED_OFF + 0x0a0)
#define LINUX_SC_V10_OFF        (LINUX_SC_RESERVED_OFF + 0x0b0)
#define LINUX_SC_V11_OFF        (LINUX_SC_RESERVED_OFF + 0x0c0)
#define LINUX_SC_V12_OFF        (LINUX_SC_RESERVED_OFF + 0x0d0)
#define LINUX_SC_V13_OFF        (LINUX_SC_RESERVED_OFF + 0x0e0)
#define LINUX_SC_V14_OFF        (LINUX_SC_RESERVED_OFF + 0x0f0)
#define LINUX_SC_V15_OFF        (LINUX_SC_RESERVED_OFF + 0x100)
#define LINUX_SC_V16_OFF        (LINUX_SC_RESERVED_OFF + 0x110)
#define LINUX_SC_V17_OFF        (LINUX_SC_RESERVED_OFF + 0x120)
#define LINUX_SC_V18_OFF        (LINUX_SC_RESERVED_OFF + 0x130)
#define LINUX_SC_V19_OFF        (LINUX_SC_RESERVED_OFF + 0x140)
#define LINUX_SC_V20_OFF        (LINUX_SC_RESERVED_OFF + 0x150)
#define LINUX_SC_V21_OFF        (LINUX_SC_RESERVED_OFF + 0x160)
#define LINUX_SC_V22_OFF        (LINUX_SC_RESERVED_OFF + 0x170)
#define LINUX_SC_V23_OFF        (LINUX_SC_RESERVED_OFF + 0x180)
#define LINUX_SC_V24_OFF        (LINUX_SC_RESERVED_OFF + 0x190)
#define LINUX_SC_V25_OFF        (LINUX_SC_RESERVED_OFF + 0x1a0)
#define LINUX_SC_V26_OFF        (LINUX_SC_RESERVED_OFF + 0x1b0)
#define LINUX_SC_V27_OFF        (LINUX_SC_RESERVED_OFF + 0x1c0)
#define LINUX_SC_V28_OFF        (LINUX_SC_RESERVED_OFF + 0x1d0)
#define LINUX_SC_V29_OFF        (LINUX_SC_RESERVED_OFF + 0x1e0)
#define LINUX_SC_V30_OFF        (LINUX_SC_RESERVED_OFF + 0x1f0)
#define LINUX_SC_V31_OFF        (LINUX_SC_RESERVED_OFF + 0x200)
/* followed then by:
 *	size		description
 *	 0x10		esr_context
 *	0x8a0		sve_context (vl <= 64) (optional)
 *	 0x20		extra_context (optional)
 *	 0x10		terminator (null _aarch64_ctx)
 * per arm64/include/uapi/asm/sigcontext.h
 */
