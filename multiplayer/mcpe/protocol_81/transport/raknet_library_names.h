#ifndef PEXCRAFT_PROTOCOL_81_RAKNET_LIBRARY_NAMES_H
#define PEXCRAFT_PROTOCOL_81_RAKNET_LIBRARY_NAMES_H

/*
 * Runtime loader names after workflow normalization.
 * Linux:   transport/bin/<arch>/libraknet.so
 * Windows: transport/bin/<arch>/raknet.dll
 */

#define PEX_RAKNET_LIB_NAME_LINUX   "libraknet.so"
#define PEX_RAKNET_LIB_NAME_WINDOWS "raknet.dll"

#define PEX_RAKNET_PLATFORM_LINUX_X64      "linux-x64"
#define PEX_RAKNET_PLATFORM_LINUX_X86      "linux-x86"
#define PEX_RAKNET_PLATFORM_LINUX_ARM64    "linux-arm64"
#define PEX_RAKNET_PLATFORM_WINDOWS_X64    "windows-x64"
#define PEX_RAKNET_PLATFORM_WINDOWS_X86    "windows-x86"
#define PEX_RAKNET_PLATFORM_WINDOWS_ARM64  "windows-arm64"

#endif /* PEXCRAFT_PROTOCOL_81_RAKNET_LIBRARY_NAMES_H */
