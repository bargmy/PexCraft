#include "bedrock_join.h"
#include <string.h>

int pex_mcpe_protocol_81_begin_bedrock_join(PexMcpeJoinSession *session,
                                            const char *host,
                                            uint16_t port,
                                            const char *username,
                                            const PexMcpeMotdInfo *motd_info) {
    if (!session) return 0;
    pex_mcpe_join_session_init(session, host, port, username);
    session->protocol_version = (motd_info && motd_info->protocol_version) ? motd_info->protocol_version : 82;
    if (session->protocol_version != 81 && session->protocol_version != 82) session->protocol_version = 82;
    session->state = PEX_MCPE_JOIN_RAKNET_CONNECTING;
    return 1;
}
