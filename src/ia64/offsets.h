/* Linux-specific definitions: */

/* Define various structure offsets to simplify cross-compilation.  */

/* The first three 64-bit words in a signal frame contain the signal
   number, siginfo pointer, and sigcontext pointer passed to the
   signal handler.  We use this to locate the sigcontext pointer.  */

#define LINUX_SIGFRAME_ARG2_OFF	0x10

#define LINUX_SC_FLAGS_OFF	0x000
#define LINUX_SC_NAT_OFF	0x008
#define LINUX_SC_STACK_OFF	0x010
#define LINUX_SC_IP_OFF		0x028
#define LINUX_SC_CFM_OFF	0x030
#define LINUX_SC_UM_OFF		0x038
#define LINUX_SC_AR_RSC_OFF	0x040
#define LINUX_SC_AR_BSP_OFF	0x048
#define LINUX_SC_AR_RNAT_OFF	0x050
#define LINUX_SC_AR_CCV		0x058
#define LINUX_SC_AR_UNAT_OFF	0x060
#define LINUX_SC_AR_FPSR_OFF	0x068
#define LINUX_SC_AR_PFS_OFF	0x070
#define LINUX_SC_AR_LC_OFF	0x078
#define LINUX_SC_PR_OFF		0x080
#define LINUX_SC_BR_OFF		0x088
#define LINUX_SC_GR_OFF		0x0c8
#define LINUX_SC_FR_OFF		0x1d0
#define LINUX_SC_RBS_BASE_OFF	0x9d0
#define LINUX_SC_LOADRS_OFF	0x9d8
#define LINUX_SC_AR_CSD_OFF	0x9e0
#define LINUX_SC_AR_26_OFF	0x9e8
#define LINUX_SC_MASK		0xa50
