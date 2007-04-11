#include <ucontext.h>
#include <stdio.h>
#include <stddef.h>

#define REG_OFFSET(reg) (offsetof(struct ucontext, uc_mcontext.gregs[REG_##reg]))


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
	printf("#ifndef UCONTEXT_I_H\n#define UCONTEXT_I_H\n");
	printf("#define REG_OFFSET_%s\t%ld\n" , regs[0], REG_OFFSET(RAX));
	printf("#define REG_OFFSET_%s\t%ld\n" , regs[1], REG_OFFSET(RBX));
	printf("#define REG_OFFSET_%s\t%ld\n" , regs[2], REG_OFFSET(RCX));
	printf("#define REG_OFFSET_%s\t%ld\n" , regs[3], REG_OFFSET(RDX));

	printf("#define REG_OFFSET_%s\t%ld\n" , regs[4], REG_OFFSET(RDI));
	printf("#define REG_OFFSET_%s\t%ld\n" , regs[5], REG_OFFSET(RSI));
	printf("#define REG_OFFSET_%s\t%ld\n" , regs[6], REG_OFFSET(RSP));
	printf("#define REG_OFFSET_%s\t%ld\n" , regs[7], REG_OFFSET(RBP));

	printf("#define REG_OFFSET_%s\t%ld\n" , regs[8], REG_OFFSET(R8));
	printf("#define REG_OFFSET_%s\t%ld\n" , regs[9], REG_OFFSET(R9));
	printf("#define REG_OFFSET_%s\t%ld\n" , regs[10], REG_OFFSET(R10));
	printf("#define REG_OFFSET_%s\t%ld\n" , regs[11], REG_OFFSET(R11));

	printf("#define REG_OFFSET_%s\t%ld\n" , regs[12], REG_OFFSET(R12));
	printf("#define REG_OFFSET_%s\t%ld\n" , regs[13], REG_OFFSET(R13));
	printf("#define REG_OFFSET_%s\t%ld\n" , regs[14], REG_OFFSET(R14));
	printf("#define REG_OFFSET_%s\t%ld\n" , regs[15], REG_OFFSET(R15));
	printf("#define REG_OFFSET_%s\t%ld\n" , regs[15], REG_OFFSET(R15));
	printf("#define REG_OFFSET_%s\t%ld\n" , regs[15], REG_OFFSET(R15));
	printf("#define REG_OFFSET_%s\t%ld\n" , regs[16], REG_OFFSET(RIP));

	printf("#define REG_OFFSET_FPREGS_PTR\t%ld\n" , offsetof(struct ucontext, uc_mcontext.fpregs));
	printf("#define FPREG_OFFSET_MXCR\t%ld\n" , offsetof(struct _libc_fpstate, mxcsr));
	printf("#endif /* UCONTEXT_I_H */\n");
	return 0;
}


