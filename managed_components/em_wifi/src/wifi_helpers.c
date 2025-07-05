/*
 * Copyright (C) 2025 EmbeddedSolutions.pl
 */

#include "em/wifi_priv.h"
#include "esp_wifi_types_generic.h"

const char *reason_to_string(wifi_err_reason_t reason)
{
  switch (reason) {
  case WIFI_REASON_UNSPECIFIED:
    return "Unspecified";
  case WIFI_REASON_AUTH_EXPIRE:
    return "Auth expire";
  case WIFI_REASON_AUTH_LEAVE:
    return "Auth leave";
  case WIFI_REASON_ASSOC_EXPIRE:
    return "Assoc expire";
  case WIFI_REASON_ASSOC_TOOMANY:
    return "Too many connections";
  case WIFI_REASON_NOT_AUTHED:
    return "Not authed";
  case WIFI_REASON_NOT_ASSOCED:
    return "Not assoced";
  case WIFI_REASON_ASSOC_LEAVE:
    return "Assoc leave";
  case WIFI_REASON_ASSOC_NOT_AUTHED:
    return "Not authenticated";
  case WIFI_REASON_DISASSOC_PWRCAP_BAD:
    return "Disassoc pwrcap bad";
  case WIFI_REASON_DISASSOC_SUPCHAN_BAD:
    return "Disassoc supchan bad";
  case WIFI_REASON_BSS_TRANSITION_DISASSOC:
    return "Bss transition disassoc";
  case WIFI_REASON_IE_INVALID:
    return "Ie invalid";
  case WIFI_REASON_MIC_FAILURE:
    return "Mic failure";
  case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
    return "Invalid password";
  case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT:
    return "Group key update timeout";
  case WIFI_REASON_IE_IN_4WAY_DIFFERS:
    return "Ie in 4way differs";
  case WIFI_REASON_GROUP_CIPHER_INVALID:
    return "Group cipher invalid";
  case WIFI_REASON_PAIRWISE_CIPHER_INVALID:
    return "Pairwise cipher invalid";
  case WIFI_REASON_AKMP_INVALID:
    return "Akmp invalid";
  case WIFI_REASON_UNSUPP_RSN_IE_VERSION:
    return "Unsupp rsn ie version";
  case WIFI_REASON_INVALID_RSN_IE_CAP:
    return "Invalid rsn ie cap";
  case WIFI_REASON_802_1X_AUTH_FAILED:
    return "802.1x auth failed";
  case WIFI_REASON_CIPHER_SUITE_REJECTED:
    return "Cipher suite rejected";
  case WIFI_REASON_TDLS_PEER_UNREACHABLE:
    return "Tdls peer unreachable";
  case WIFI_REASON_TDLS_UNSPECIFIED:
    return "Tdls unspecified";
  case WIFI_REASON_SSP_REQUESTED_DISASSOC:
    return "Ssp requested disassoc";
  case WIFI_REASON_NO_SSP_ROAMING_AGREEMENT:
    return "No ssp roaming agreement";
  case WIFI_REASON_BAD_CIPHER_OR_AKM:
    return "Bad cipher or akm";
  case WIFI_REASON_NOT_AUTHORIZED_THIS_LOCATION:
    return "Not authorized this location";
  case WIFI_REASON_SERVICE_CHANGE_PERCLUDES_TS:
    return "Service change percludes ts";
  case WIFI_REASON_UNSPECIFIED_QOS:
    return "Unspecified qos";
  case WIFI_REASON_NOT_ENOUGH_BANDWIDTH:
    return "Not enough bandwidth";
  case WIFI_REASON_MISSING_ACKS:
    return "Missing acks";
  case WIFI_REASON_EXCEEDED_TXOP:
    return "Exceeded txop";
  case WIFI_REASON_STA_LEAVING:
    return "Sta leaving";
  case WIFI_REASON_END_BA:
    return "End ba";
  case WIFI_REASON_UNKNOWN_BA:
    return "Unknown ba";
  case WIFI_REASON_TIMEOUT:
    return "Timeout";
  case WIFI_REASON_PEER_INITIATED:
    return "Peer initiated";
  case WIFI_REASON_AP_INITIATED:
    return "Ap initiated";
  case WIFI_REASON_INVALID_FT_ACTION_FRAME_COUNT:
    return "Invalid ft action frame count";
  case WIFI_REASON_INVALID_PMKID:
    return "Invalid pmkid";
  case WIFI_REASON_INVALID_MDE:
    return "Invalid mde";
  case WIFI_REASON_INVALID_FTE:
    return "Invalid fte";
  case WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED:
    return "Transmission link establish failed";
  case WIFI_REASON_ALTERATIVE_CHANNEL_OCCUPIED:
    return "Alterative channel occupied";
  case WIFI_REASON_BEACON_TIMEOUT:
    return "Beacon timeout";
  case WIFI_REASON_NO_AP_FOUND:
    return "Network not found";
  case WIFI_REASON_AUTH_FAIL:
    return "Auth fail";
  case WIFI_REASON_ASSOC_FAIL:
    return "Assoc fail";
  case WIFI_REASON_HANDSHAKE_TIMEOUT:
    return "Handshake timeout";
  case WIFI_REASON_CONNECTION_FAIL:
    return "Connection fail";
  case WIFI_REASON_AP_TSF_RESET:
    return "Ap tsf reset";
  case WIFI_REASON_ROAMING:
    return "Roaming";
  case WIFI_REASON_ASSOC_COMEBACK_TIME_TOO_LONG:
    return "Assoc comeback time too long";
  case WIFI_REASON_SA_QUERY_TIMEOUT:
    return "Sa query timeout";
  case WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY:
    return "No ap found w compatible security";
  case WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD:
    return "No ap found in authmode threshold";
  case WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD:
    return "No ap found in rssi threshold";
  default:
    return "Unknown";
  }
}
