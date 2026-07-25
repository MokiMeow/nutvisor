ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5);
    data PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = 0x100000;

    .text : ALIGN(0x1000)
    {
        *(.text*)
        *(.rodata*)
    } :text

    .data : ALIGN(0x1000)
    {
        *(.data*)
        *(.bss*)
        *(COMMON)
    } :data

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
