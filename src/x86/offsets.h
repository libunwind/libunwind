/* Linux-specific definitions: */

/* Define various structure offsets to simplify cross-compilation.  */

/* The struct sigcontext is located at an offset of 4
   from the stack pointer in the signal frame.         */

#define LINUX_SC_ESP_OFF       0x1c
#define LINUX_SC_EBP_OFF       0x18
#define LINUX_SC_EIP_OFF       0x38

/* With SA_SIGINFO set, we believe that basically the same
   layout is used for ucontext_t, except that 20 bytes are added
   at the beginning. */
#define LINUX_UC_ESP_OFF       (LINUX_SC_ESP_OFF+20)
#define LINUX_UC_EBP_OFF       (LINUX_SC_EBP_OFF+20)
#define LINUX_UC_EIP_OFF       (LINUX_SC_EIP_OFF+20)
