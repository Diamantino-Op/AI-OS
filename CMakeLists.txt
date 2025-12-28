cmake_minimum_required(VERSION 3.20)
project(GladiOS VERSION 1.0.0 LANGUAGES CXX C ASM)

# Set C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Cross-compilation settings for x86_64 freestanding environment
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Compiler flags for freestanding environment
set(KERNEL_COMPILE_FLAGS
    -ffreestanding
    -fno-stack-protector
    -fno-stack-check
    -fno-lto
    -fno-PIC
    -fno-PIE
    -m64
    -march=x86-64
    -mno-80387
    -mno-mmx
    -mno-sse
    -mno-sse2
    -mno-red-zone
    -mcmodel=kernel
    -Wall
    -Wextra
    -Werror
)

# Linker flags
set(KERNEL_LINK_FLAGS
    -nostdlib
    -static
    -z max-page-size=0x1000
    -T ${CMAKE_SOURCE_DIR}/src/kernel/linker.ld
)

# Define paths
set(LIMINE_DIR ${CMAKE_SOURCE_DIR}/deps/limine)
set(FREESTANDING_HEADERS_DIR ${CMAKE_SOURCE_DIR}/deps/freestanding-headers)

# Kernel source files
set(KERNEL_SOURCES
    src/kernel/main.cpp
)

# Create kernel executable
add_executable(Kernel.elf ${KERNEL_SOURCES})

# Set target properties
target_compile_options(Kernel.elf PRIVATE ${KERNEL_COMPILE_FLAGS})
target_link_options(Kernel.elf PRIVATE ${KERNEL_LINK_FLAGS})

# Include directories
target_include_directories(Kernel.elf PRIVATE
    ${FREESTANDING_HEADERS_DIR}/include/c
    ${FREESTANDING_HEADERS_DIR}/include/cxx
    ${LIMINE_DIR}
)

# Set the output name
set_target_properties(Kernel.elf PROPERTIES
    OUTPUT_NAME "Kernel.elf"
    SUFFIX ""
)

# ISO build directory
set(ISO_ROOT ${CMAKE_BINARY_DIR}/out/isoTemp)

# Custom target to build limine
add_custom_target(build_limine
    COMMAND ./bootstrap
    COMMAND ./configure --enable-uefi-cd --enable-bios-cd
    COMMAND make
    WORKING_DIRECTORY ${LIMINE_DIR}
    COMMENT "Building Limine bootloader..."
)

# Custom target to prepare ISO structure
add_custom_target(prepare_iso
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${ISO_ROOT}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ISO_ROOT}/boot/GladiOS
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ISO_ROOT}/boot/limine
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Kernel.elf> ${ISO_ROOT}/boot/GladiOS/Kernel.elf
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/limine-config/limine.cfg ${ISO_ROOT}/boot/limine/limine.cfg
    DEPENDS Kernel.elf
    COMMENT "Preparing ISO directory structure..."
)

# Custom target to build ISO
add_custom_target(build_iso
    COMMAND ${CMAKE_COMMAND} -E copy ${LIMINE_DIR}/limine-bios.sys ${ISO_ROOT}/boot/limine/
    COMMAND ${CMAKE_COMMAND} -E copy ${LIMINE_DIR}/limine-bios-cd.bin ${ISO_ROOT}/boot/limine/
    COMMAND ${CMAKE_COMMAND} -E copy ${LIMINE_DIR}/limine-uefi-cd.bin ${ISO_ROOT}/boot/limine/
    COMMAND ${CMAKE_COMMAND} -E copy ${LIMINE_DIR}/BOOTX64.EFI ${ISO_ROOT}/EFI/BOOT/BOOTX64.EFI
    COMMAND ${CMAKE_COMMAND} -E copy ${LIMINE_DIR}/BOOTIA32.EFI ${ISO_ROOT}/EFI/BOOT/BOOTIA32.EFI
    COMMAND xorriso -as mkisofs
        -b boot/limine/limine-bios-cd.bin
        -no-emul-boot -boot-load-size 4 -boot-info-table
        --efi-boot boot/limine/limine-uefi-cd.bin
        -efi-boot-part --efi-boot-image --protective-msdos-label
        ${ISO_ROOT} -o ${CMAKE_BINARY_DIR}/out/GladiOS.iso
    COMMAND ${LIMINE_DIR}/limine bios-install ${CMAKE_BINARY_DIR}/out/GladiOS.iso
    DEPENDS prepare_iso build_limine
    COMMENT "Building bootable ISO..."
)

# Custom target to run in QEMU with KVM
add_custom_target(run_qemu
    COMMAND qemu-system-x86_64
        -M q35
        -m 2G
        -cdrom ${CMAKE_BINARY_DIR}/out/GladiOS.iso
        -boot d
        -enable-kvm
        -cpu host
        -smp 2
    DEPENDS build_iso
    COMMENT "Running GladiOS in QEMU with KVM..."
)

# Custom target to run in QEMU without KVM (for systems where KVM is not available)
add_custom_target(run_qemu_no_kvm
    COMMAND qemu-system-x86_64
        -M q35
        -m 2G
        -cdrom ${CMAKE_BINARY_DIR}/out/GladiOS.iso
        -boot d
        -cpu qemu64
        -smp 2
    DEPENDS build_iso
    COMMENT "Running GladiOS in QEMU without KVM..."
)

# Add a clean target for ISO files
add_custom_target(clean_iso
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/out
    COMMENT "Cleaning ISO build artifacts..."
)
