	.globl func_add1, func_add1_end
	.proc func_add1
func_add1:
{.mib	add r8 = 1, r32
	nop.i 0
	br.ret.sptk.many rp
}
func_add1_end:
	.endp func_add1

	.globl func_add3, func_add3_end
	.proc func_add3
func_add3:
{.mmi	alloc loc1 = ar.pfs, 2, 2, 2, 0
	ld8 r8 = [in1], 8		// load the function pointer
	mov loc0 = rp
}	;;
{.mmi
	ld8 r9 = [r8], 8		// load the entry-point
	mov out0 = in0
	mov out1 = in1
}	;;
{.mib	ld8 gp = [r8]			// load the gp
	mov b6 = r9
	br.call.sptk rp = b6
}
{.mii
	add r8 = 2, r8
	mov ar.pfs = loc1
	mov rp = loc0
}
{.mib
	mov loc0 = 1		// trash loc0
	mov loc1 = 3		// trash loc1
	br.ret.sptk.many rp
}
func_add3_end:
	.endp func_add3
