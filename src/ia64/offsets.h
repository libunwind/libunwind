/* Define various structure offsets to simplify cross-compilation.  */

/* The first three 64-bit words in a signal frame contain the signal
   number, siginfo pointer, and sigcontext pointer passed to the
   signal handler.  We use this to locate the sigcontext pointer.  */
#define SIGFRAME_ARG2_OFF	0x10

#define SIGCONTEXT_FLAGS_OFF	0x000
#define SIGCONTEXT_NAT_OFF	0x008
#define SIGCONTEXT_STACK_OFF	0x010
#define SIGCONTEXT_IP_OFF	0x028
#define SIGCONTEXT_CFM_OFF	0x030
#define SIGCONTEXT_UM_OFF	0x038
#define SIGCONTEXT_AR_RSC_OFF	0x040
#define SIGCONTEXT_AR_BSP_OFF	0x048
#define SIGCONTEXT_AR_RNAT_OFF	0x050
#define SIGCONTEXT_AR_CCV	0x058
#define SIGCONTEXT_AR_UNAT_OFF	0x060
#define SIGCONTEXT_AR_FPSR_OFF	0x068
#define SIGCONTEXT_AR_PFS_OFF	0x070
#define SIGCONTEXT_AR_LC_OFF	0x078
#define SIGCONTEXT_PR_OFF	0x080
#define SIGCONTEXT_BR_OFF	0x088
#define SIGCONTEXT_GR_OFF	0x0c8
#define SIGCONTEXT_FR_OFF	0x1d0
#define SIGCONTEXT_RBS_BASE_OFF	0x9d0
#define SIGCONTEXT_LOADRS_OFF	0x9d8
#define SIGCONTEXT_AR_25_OFF	0x9e0
#define SIGCONTEXT_AR_26_OFF	0x9e8
#define SIGCONTEXT_MASK		0xa50
