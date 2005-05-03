#if defined(__ia64__)

	.global flush_cache

	.proc flush_cache
flush_cache:
	.prologue
        alloc r2=ar.pfs,2,0,0,0
	add r8=31,in1			// round up to 32 byte-boundary
        ;;
        shr.u r8=r8,5                   // we flush 32 bytes per iteration
	;;
	add r8=-1,r8
        .save ar.lc, r3
        mov r3=ar.lc                    // save ar.lc
        ;;
        .body

        mov ar.lc=r8
        ;;
.loop:  fc in0                          // issuable on M0 only
        add in0=32,in0
        br.cloop.sptk.few .loop
        ;;
        sync.i
        ;;
        srlz.i
        ;;
        mov ar.lc=r3                    // restore ar.lc
        br.ret.sptk.many rp
	.endp flush_cache

#elif defined(__i386__) || defined (__x86_64__)

	.globl flush_cache
flush_cache:
	ret

#elif defined(__hppa__)

# warning FIX ME!!

        .globl flush_cache
flush_cache:
        .proc
        .callinfo
        bv      %r0(%rp)
        .procend

#else
# error Need flush_cache code for this architecture.
#endif
