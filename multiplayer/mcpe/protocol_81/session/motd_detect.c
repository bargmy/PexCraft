#include "motd_detect.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int pex_mcpe_protocol_81_parse_motd(const char *motd, PexMcpeMotdInfo *out_info) {
    if (out_info) memset(out_info, 0, sizeof(*out_info));
    if (!motd || strncmp(motd, "MCPE;", 5) != 0) return 0;

    char copy[256];
    snprintf(copy, sizeof(copy), "%s", motd);

    char *fields[8];
    int field_count = 0;
    char *p = copy;
    while (field_count < (int)(sizeof(fields) / sizeof(fields[0]))) {
        fields[field_count++] = p;
        char *semi = strchr(p, ';');
        if (!semi) break;
        *semi = '\0';
        p = semi + 1;
    }

    if (out_info) {
        out_info->is_mcpe = 1;
        if (field_count > 1) snprintf(out_info->server_name, sizeof(out_info->server_name), "%s", fields[1]);
        if (field_count > 2) out_info->protocol_version = atoi(fields[2]);
        if (field_count > 3) snprintf(out_info->game_version, sizeof(out_info->game_version), "%s", fields[3]);
        out_info->should_use_bedrock_join =
            (out_info->protocol_version == 82 || strcmp(out_info->game_version, "0.15.4") == 0);
    }
    return 1;
}

int pex_mcpe_protocol_81_should_join_from_motd(const char *motd) {
    PexMcpeMotdInfo info;
    if (!pex_mcpe_protocol_81_parse_motd(motd, &info)) return 0;
    return info.should_use_bedrock_join;
}
