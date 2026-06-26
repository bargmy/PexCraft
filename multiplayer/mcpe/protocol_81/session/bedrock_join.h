#ifndef PEXCRAFT_MCPE_PROTOCOL_81_BEDROCK_JOIN_H
#define PEXCRAFT_MCPE_PROTOCOL_81_BEDROCK_JOIN_H

#include <stdint.h>
#include "join_session.h"
#include "motd_detect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Placeholder join entry selected after MOTD detection says MCPE 0.15.4 / protocol 82. */
int pex_mcpe_protocol_81_begin_bedrock_join(PexMcpeJoinSession *session,
                                            const char *host,
                                            uint16_t port,
                                            const char *username,
                                            const PexMcpeMotdInfo *motd_info);

#ifdef __cplusplus
}
#endif

#endif /* PEXCRAFT_MCPE_PROTOCOL_81_BEDROCK_JOIN_H */
