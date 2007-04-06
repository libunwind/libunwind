#include <ucontext.h>
#include <stdio.h>
#include <stddef.h>

#define REG_OFFSET(reg) (offsetof(struct ucontext, uc_mcontext.gregs[REG_##reg]))

# define REG_R8		0
# define REG_R9		1
# define REG_R10	2
# define REG_R11	3
# define REG_R12	4
# define REG_R13	5
# define REG_R14	6
# define REG_R15	7
# define REG_RDI	8
# define REG_RSI	9
# define REG_RBP	10
# define REG_RBX	11
# define REG_RDX	12
# define REG_RAX	13
# define REG_RCX	14
# define REG_RSP	15
# define REG_RIP	16

char *regs[] = { "RAX",
	"RBX",
	"RCX",
	"RDX",
	"RDI",
	"RSI",
	"RSP",
	"RBP",
	"R8",
	"R9",
	"R10",
	"R11",
	"R12",
	"R13",
	"R14",
	"R15",
	"RIP",
	};

main()
{
	printf("#define REG_OFFSET_%s\t%d\n" , regs[0], REG_OFFSET(RAX));
	printf("#define REG_OFFSET_%s\t%d\n" , regs[1], REG_OFFSET(RBX));
	printf("#define REG_OFFSET_%s\t%d\n" , regs[2], REG_OFFSET(RCX));
	printf("#define REG_OFFSET_%s\t%d\n" , regs[3], REG_OFFSET(RDX));

	printf("#define REG_OFFSET_%s\t%d\n" , regs[4], REG_OFFSET(RDI));
	printf("#define REG_OFFSET_%s\t%d\n" , regs[5], REG_OFFSET(RSI));
	printf("#define REG_OFFSET_%s\t%d\n" , regs[6], REG_OFFSET(RSP));
	printf("#define REG_OFFSET_%s\t%d\n" , regs[7], REG_OFFSET(RBP));

	printf("#define REG_OFFSET_%s\t%d\n" , regs[8], REG_OFFSET(R8));
	printf("#define REG_OFFSET_%s\t%d\n" , regs[9], REG_OFFSET(R9));
	printf("#define REG_OFFSET_%s\t%d\n" , regs[10], REG_OFFSET(R10));
	printf("#define REG_OFFSET_%s\t%d\n" , regs[11], REG_OFFSET(R11));

	printf("#define REG_OFFSET_%s\t%d\n" , regs[12], REG_OFFSET(R12));
	printf("#define REG_OFFSET_%s\t%d\n" , regs[13], REG_OFFSET(R13));
	printf("#define REG_OFFSET_%s\t%d\n" , regs[14], REG_OFFSET(R14));
	printf("#define REG_OFFSET_%s\t%d\n" , regs[15], REG_OFFSET(R15));
	printf("#define REG_OFFSET_%s\t%d\n" , regs[15], REG_OFFSET(R15));
	printf("#define REG_OFFSET_%s\t%d\n" , regs[15], REG_OFFSET(R15));
	printf("#define REG_OFFSET_%s\t%d\n" , regs[16], REG_OFFSET(RIP));

	printf("#define REG_OFFSET_FPREGS_PTR\t%d\n" , offsetof(struct ucontext, uc_mcontext.fpregs));
	printf("#define FPREG_OFFSET_MXCR\t%d\n" , offsetof(struct _libc_fpstate, mxcsr));
}


