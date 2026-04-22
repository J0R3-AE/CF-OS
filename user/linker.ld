SECTIONS
{
    . = 0x400000; /* user base */

    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) }

    /* USER HEAP */
    __heap_start = .;
    . = . + 0x100000;
    __heap_end = .;
}