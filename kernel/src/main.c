void kernel_main(void) {
    while (1)
        __asm__ volatile("cli; hlt");
}
