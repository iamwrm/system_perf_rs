        .text
        # LLVM-MCA-BEGIN dot-product-body
        movq (%rdi), %rax
        movq (%rsi), %rcx
        imulq %rcx, %rax
        addq %rax, %r8
        addq $8, %rdi
        addq $8, %rsi
        # LLVM-MCA-END
