/**
 * raknet_c.h — Plain C interface for RakNet (C++ -> C DLL wrapper)
 *
 * This is the ONLY header your C client needs.
 * No C++ required. All types are plain C.
 *
 * ──────────────────────────────────────────────────────────────────
 *  Compile your C client (Linux .so):
 *    gcc client.c -o client \
 *        -I path/to/RakNet/Source \
 *        -L path/to/build/Lib/DLL -lRakNetDLL \
 *        -Wl,-rpath,'$ORIGIN'
 *
 *  Compile your C client (Windows .dll, MinGW):
 *    x86_64-w64-mingw32-gcc client.c -o client.exe \
 *        -I path/to/RakNet/Source \
 *        path/to/build-win64/Lib/DLL/libRakNetDLL.dll.a -lws2_32
 * ──────────────────────────────────────────────────────────────────
 */

#ifndef RAKNET_C_H
#define RAKNET_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  DLL visibility macros                                               */
/* ------------------------------------------------------------------ */
#if defined(_WIN32)
#  if defined(RAKNET_C_EXPORTS)
#    define RAKNET_C_API __declspec(dllexport)
#  else
#    define RAKNET_C_API __declspec(dllimport)
#  endif
#else
#  define RAKNET_C_API __attribute__((visibility("default")))
#endif

/* ------------------------------------------------------------------ */
/*  Opaque handles — never dereference, just pass back to the API      */
/* ------------------------------------------------------------------ */
typedef struct RakPeerHandle RakPeerHandle;
typedef struct RakPacket     RakPacket;

/* ------------------------------------------------------------------ */
/*  Packet priority (pass to raknet_peer_send)                         */
/* ------------------------------------------------------------------ */
#define RN_PRIORITY_IMMEDIATE  0
#define RN_PRIORITY_HIGH       1
#define RN_PRIORITY_MEDIUM     2
#define RN_PRIORITY_LOW        3

/* ------------------------------------------------------------------ */
/*  Packet reliability (pass to raknet_peer_send)                      */
/* ------------------------------------------------------------------ */
#define RN_UNRELIABLE          0   /* Like raw UDP, may drop / reorder  */
#define RN_UNRELIABLE_SEQ      1   /* Unreliable, sequenced             */
#define RN_RELIABLE            2   /* Guaranteed delivery               */
#define RN_RELIABLE_ORDERED    3   /* Guaranteed + ordered per channel  */
#define RN_RELIABLE_SEQ        4   /* Guaranteed + sequenced            */

/* ------------------------------------------------------------------ */
/*  Built-in system message IDs (data[0] in your receive loop)         */
/*  Values < 134 are internal; 134+ are free for your application.     */
/* ------------------------------------------------------------------ */
#define RN_MSG_INTERNAL_PING                    6
#define RN_MSG_CONNECTION_REQUEST_ACCEPTED     16   /* 0x10 */
#define RN_MSG_CONNECTION_ATTEMPT_FAILED       17   /* 0x11 */
#define RN_MSG_ALREADY_CONNECTED               18   /* 0x12 */
#define RN_MSG_NEW_INCOMING_CONNECTION         19   /* 0x13 */
#define RN_MSG_NO_FREE_INCOMING_CONNECTIONS    20   /* 0x14 */
#define RN_MSG_DISCONNECTION_NOTIFICATION      21   /* 0x15 */
#define RN_MSG_CONNECTION_LOST                 22   /* 0x16 */
#define RN_MSG_INVALID_PASSWORD                32   /* 0x20 */
#define RN_MSG_USER_PACKET_ENUM               134   /* first free user ID */

/* ------------------------------------------------------------------ */
/*  Return codes from raknet_peer_startup()                            */
/* ------------------------------------------------------------------ */
#define RN_STARTED                    0
#define RN_ERR_ALREADY_STARTED        1   /* OK — idempotent            */
#define RN_ERR_INVALID_SOCKET        -1
#define RN_ERR_PORT_IN_USE           -2
#define RN_ERR_STARTUP_FAILURE       -3

/* ------------------------------------------------------------------ */
/*  Return codes from raknet_peer_connect()                            */
/* ------------------------------------------------------------------ */
#define RN_CONNECT_STARTED            0
#define RN_ERR_ALREADY_CONNECTED     -1
#define RN_ERR_CONNECT_IN_PROGRESS   -2
#define RN_ERR_CONNECT_FAILED        -3

/* ------------------------------------------------------------------ */
/*  Flat packet info struct — safe to read from C                      */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t*  data;              /**< Payload bytes; data[0] = message ID    */
    uint32_t  length;            /**< Number of bytes in data[]              */
    char      systemAddress[64]; /**< Sender IP as dotted-decimal string     */
    uint16_t  systemPort;        /**< Sender UDP port                        */
    uint32_t  systemAddressRaw;  /**< Sender IPv4 in host byte order         */
    uint64_t  guid;              /**< Sender's 64-bit RakNetGUID             */
} RakPacketInfo;

/* ================================================================== */
/*  Lifecycle                                                           */
/* ================================================================== */

/** Allocate a new RakPeer. Must be freed with raknet_peer_destroy(). */
RAKNET_C_API RakPeerHandle* raknet_peer_create(void);

/** Free a RakPeer. Automatically shuts down if still active. */
RAKNET_C_API void raknet_peer_destroy(RakPeerHandle* peer);

/**
 * Bind a UDP socket and start the network thread.
 *
 * @param peer            Handle from raknet_peer_create().
 * @param maxConnections  Maximum simultaneous connections.
 *                        Use 1 for a pure outbound client.
 * @param localPort       UDP port to bind on (0 = OS-assigned random port).
 * @return RN_STARTED on success, RN_ERR_* on failure.
 */
RAKNET_C_API int raknet_peer_startup(RakPeerHandle* peer,
                                     unsigned int   maxConnections,
                                     unsigned short localPort);

/**
 * Allow up to `count` remote peers to connect TO you (server mode).
 * Call after raknet_peer_startup(). Leave at 0 for a pure client.
 */
RAKNET_C_API void raknet_peer_set_max_incoming(RakPeerHandle* peer,
                                               unsigned int   count);

/**
 * Begin an outbound connection attempt (non-blocking).
 *
 * @param peer           Handle.
 * @param host           Server IP or hostname string.
 * @param remotePort     Server UDP port.
 * @param password       Shared-secret bytes, or NULL.
 * @param passwordLen    Length of password in bytes.
 * @return RN_CONNECT_STARTED on success, RN_ERR_* on error.
 */
RAKNET_C_API int raknet_peer_connect(RakPeerHandle* peer,
                                     const char*    host,
                                     unsigned short remotePort,
                                     const char*    password,
                                     int            passwordLen);

/**
 * Close all connections and stop the network thread.
 *
 * @param peer              Handle.
 * @param blockDurationMs   Milliseconds to wait for sends to flush.
 *                          300 = graceful, 0 = immediate abort.
 */
RAKNET_C_API void raknet_peer_shutdown(RakPeerHandle* peer,
                                       unsigned int   blockDurationMs);

/* ================================================================== */
/*  Send / Receive                                                      */
/* ================================================================== */

/**
 * Send raw bytes.
 *
 * @param peer        Handle.
 * @param data        Payload. IMPORTANT: data[0] must be >= RN_MSG_USER_PACKET_ENUM (134).
 * @param length      Number of bytes to send.
 * @param priority    RN_PRIORITY_* constant.
 * @param reliability RN_RELIABLE_* constant.
 * @param channel     Ordering channel 0–31 (only matters for ordered/sequenced).
 * @param host        Target IP, or NULL for broadcast.
 * @param port        Target port (ignored if host==NULL or broadcast!=0).
 * @param broadcast   Non-zero = send to ALL connected peers.
 * @return Bytes enqueued (>= 0), or -1 on error.
 */
RAKNET_C_API int raknet_peer_send(RakPeerHandle* peer,
                                  const char*    data,
                                  int            length,
                                  int            priority,
                                  int            reliability,
                                  int            channel,
                                  const char*    host,
                                  unsigned short port,
                                  int            broadcast);

/**
 * Non-blocking poll. Returns NULL when queue is empty.
 *
 * You MUST call raknet_peer_deallocate_packet() on every non-NULL result.
 *
 *   RakPacket* pkt;
 *   while ((pkt = raknet_peer_receive(peer)) != NULL) {
 *       RakPacketInfo info;
 *       raknet_packet_get_info(pkt, &info);
 *       switch (info.data[0]) {
 *           case RN_MSG_CONNECTION_REQUEST_ACCEPTED: ...
 *           case RN_MSG_USER_PACKET_ENUM + MY_MSG:   ...
 *       }
 *       raknet_peer_deallocate_packet(peer, pkt);
 *   }
 */
RAKNET_C_API RakPacket* raknet_peer_receive(RakPeerHandle* peer);

/**
 * Copy packet data into a flat C struct.
 * Call this before raknet_peer_deallocate_packet().
 */
RAKNET_C_API void raknet_packet_get_info(RakPacket*     packet,
                                         RakPacketInfo* out);

/** Must be called exactly once per packet from raknet_peer_receive(). */
RAKNET_C_API void raknet_peer_deallocate_packet(RakPeerHandle* peer,
                                                RakPacket*     packet);

/* ================================================================== */
/*  Status / Diagnostics                                                */
/* ================================================================== */

/** Returns non-zero if the peer is started and its thread is running. */
RAKNET_C_API int raknet_peer_is_active(RakPeerHandle* peer);

/** Local UDP port bound by the peer (index 0 in most cases), or 0 if not started. */
RAKNET_C_API unsigned short raknet_peer_get_bound_port(RakPeerHandle* peer,
                                                       unsigned int   socketIndex);

/** Current number of open connections. */
RAKNET_C_API unsigned int raknet_peer_get_connection_count(RakPeerHandle* peer);

/**
 * Average round-trip time to a specific remote peer in milliseconds.
 * Returns -1 if not connected or ping unknown.
 */
RAKNET_C_API int raknet_peer_get_average_ping(RakPeerHandle* peer,
                                              const char*    host,
                                              unsigned short port);

/**
 * Get a local IP address string.
 * Writes into buf (up to bufSize bytes). Returns buf, or NULL on error.
 *
 * @param index  Interface index. 0 is usually the primary interface.
 */
RAKNET_C_API const char* raknet_peer_get_local_ip(RakPeerHandle* peer,
                                                  unsigned int   index,
                                                  char*          buf,
                                                  unsigned int   bufSize);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAKNET_C_H */
