        .text
        # LLVM-MCA-BEGIN dot-product-body
        ldr x2, [x0]
        ldr x3, [x1]
        mul x4, x2, x3
        add x5, x5, x4
        add x0, x0, #8
        add x1, x1, #8
        # LLVM-MCA-END
