#ifndef PEXCRAFT_MCPE_PROTOCOL_81_MOTD_DETECT_H
#define PEXCRAFT_MCPE_PROTOCOL_81_MOTD_DETECT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PexMcpeMotdInfo {
    int is_mcpe;
    int should_use_bedrock_join;
    int protocol_version;
    char game_version[24];
    char server_name[96];
} PexMcpeMotdInfo;

/* Parses RakNet MOTD strings like:
 *   MCPE;Dedicated Server;82;0.15.4;0;20;...
 * Returns 1 when the string is an MCPE MOTD. For now, protocol 82 or version
 * 0.15.4 routes into multiplayer/mcpe/protocol_81/ joining.
 */
int pex_mcpe_protocol_81_parse_motd(const char *motd, PexMcpeMotdInfo *out_info);
int pex_mcpe_protocol_81_should_join_from_motd(const char *motd);

#ifdef __cplusplus
}
#endif

#endif /* PEXCRAFT_MCPE_PROTOCOL_81_MOTD_DETECT_H */
