/* Linux-specific definitions: */

/* Define various structure offsets to simplify cross-compilation.  */

/* Offset of uc_mcontext in ucontext_t.  The kernel uses the asm-generic
   struct ucontext layout, shared by glibc and musl.  */

#define LINUX_UC_MCONTEXT_OFF   0x14

/* struct sigcontext starts with user_regs_struct: r0-r31, PC and SR.  */

#define LINUX_SC_GPR_OFF        0x00
#define LINUX_SC_PC_OFF         0x80
