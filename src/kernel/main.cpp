#include <cstdint>
#include <cstddef>

// Limine protocol requests
#include <limine.h>

// Set the base revision to 2, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
__attribute__((used, section(".requests")))
LIMINE_BASE_REVISION(2);

// Request a framebuffer
__attribute__((used, section(".requests")))
static volatile limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = nullptr
};

// Halt and catch fire function
static void hcf() {
    for (;;) {
        asm ("hlt");
    }
}

// The following will be our kernel's entry point.
extern "C" void _start() {
    // Ensure the bootloader actually understands our base revision
    if (!LIMINE_BASE_REVISION_SUPPORTED) {
        hcf();
    }

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == nullptr
        || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // Get the first framebuffer's information.
    limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    // Draw a simple pattern on the framebuffer
    // This will create a gradient effect
    for (size_t i = 0; i < 100; i++) {
        volatile uint32_t *fb_ptr = static_cast<uint32_t*>(framebuffer->address);
        fb_ptr[i * (framebuffer->pitch / 4) + i] = 0xFFFFFF;
    }

    // We're done, just hang...
    hcf();
}
