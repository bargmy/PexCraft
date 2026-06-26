#ifndef PEXCRAFT_PROTOCOL_81_RAKNET_LOADER_H
#define PEXCRAFT_PROTOCOL_81_RAKNET_LOADER_H

#include "include/raknet_c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef RakPeerHandle* (*PexFnRaknetPeerCreate)(void);
typedef void (*PexFnRaknetPeerDestroy)(RakPeerHandle*);
typedef int (*PexFnRaknetPeerStartup)(RakPeerHandle*, unsigned int, unsigned short);
typedef void (*PexFnRaknetPeerSetMaxIncoming)(RakPeerHandle*, unsigned int);
typedef int (*PexFnRaknetPeerConnect)(RakPeerHandle*, const char*, unsigned short, const char*, int);
typedef void (*PexFnRaknetPeerShutdown)(RakPeerHandle*, unsigned int);
typedef int (*PexFnRaknetPeerSend)(RakPeerHandle*, const char*, int, int, int, int, const char*, unsigned short, int);
typedef RakPacket* (*PexFnRaknetPeerReceive)(RakPeerHandle*);
typedef void (*PexFnRaknetPacketGetInfo)(RakPacket*, RakPacketInfo*);
typedef void (*PexFnRaknetPeerDeallocatePacket)(RakPeerHandle*, RakPacket*);
typedef int (*PexFnRaknetPeerIsActive)(RakPeerHandle*);
typedef unsigned int (*PexFnRaknetPeerGetConnectionCount)(RakPeerHandle*);
typedef int (*PexFnRaknetPeerGetAveragePing)(RakPeerHandle*, const char*, unsigned short);

typedef struct PexRakNetLibrary {
    void *handle;
    char path[512];
    char error[256];
    PexFnRaknetPeerCreate peer_create;
    PexFnRaknetPeerDestroy peer_destroy;
    PexFnRaknetPeerStartup peer_startup;
    PexFnRaknetPeerSetMaxIncoming peer_set_max_incoming;
    PexFnRaknetPeerConnect peer_connect;
    PexFnRaknetPeerShutdown peer_shutdown;
    PexFnRaknetPeerSend peer_send;
    PexFnRaknetPeerReceive peer_receive;
    PexFnRaknetPacketGetInfo packet_get_info;
    PexFnRaknetPeerDeallocatePacket peer_deallocate_packet;
    PexFnRaknetPeerIsActive peer_is_active;
    PexFnRaknetPeerGetConnectionCount peer_get_connection_count;
    PexFnRaknetPeerGetAveragePing peer_get_average_ping;
} PexRakNetLibrary;

PexRakNetLibrary* pex_raknet_library_open_for_current_platform(void);
void pex_raknet_library_close(PexRakNetLibrary* lib);
const char* pex_raknet_library_expected_name(void);
const char* pex_raknet_library_expected_platform_folder(void);
const char* pex_raknet_library_last_error(const PexRakNetLibrary* lib);

#ifdef __cplusplus
}
#endif

#endif /* PEXCRAFT_PROTOCOL_81_RAKNET_LOADER_H */
