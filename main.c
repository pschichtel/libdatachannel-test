#include <stdio.h>
#include <rtc/rtc.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAX_SDP_SIZE 4096
#define MAX_CANDIDATES 50
#define MAX_CANDIDATE_LENGTH 200

struct rtc_state {
    char local_description[MAX_SDP_SIZE];
    char local_candidates[MAX_CANDIDATES][MAX_CANDIDATE_LENGTH];
    int32_t local_candidate_count;
    bool channel_open;
};

void RTC_API logger(rtcLogLevel level, const char *message) {
    printf("libdatachannel: %s\n", message);
}

void RTC_API on_ice_state_changed(int pc, rtcIceState state, void *ptr) {
    printf("New ICE rtc_state: %d\n", state);
}

void RTC_API on_ice_gathering_state_changed(int pc, rtcGatheringState state, void *ptr) {
    printf("New ICE gathering rtc_state: %d\n", state);
}

void replace_newlines(char *str) {
    char *src = str, *dst = str;
    while (*src) {
        if (src[0] == '\r' && src[1] == '\n') {
            *dst++ = '\n';
            src += 2;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

void render_current_sdp(struct rtc_state* state) {
    char sdp[MAX_SDP_SIZE];
    size_t current_sdp_len = strlen(state->local_description);
    memcpy(sdp, state->local_description, current_sdp_len);

    size_t candidate_size;
    char* base = sdp + current_sdp_len;
    for (int i = 0; i < state->local_candidate_count; i++) {
        *(base++) = 'a';
        *(base++) = '=';
        candidate_size = strlen(state->local_candidates[i]);
        memcpy(base, state->local_candidates[i], candidate_size);
        base += candidate_size;
        *(base++) = '\r';
        *(base++) = '\n';
    }
    *base = 0;

    replace_newlines(sdp);

    printf("SDP:\n\n%s\n", sdp);
}

void RTC_API on_local_description(int pc, const char *sdp, const char *type, void *ptr) {
    printf("SDP:\n %s\n\n", sdp);
    struct rtc_state* state = (struct rtc_state*)ptr;
    strcpy(state->local_description, sdp);
    render_current_sdp(state);
}

void RTC_API on_local_candidate(int pc, const char *cand, const char *mid, void *ptr) {
    printf("Received candidate: %s\n", cand);
    struct rtc_state* state = (struct rtc_state*)ptr;
    if (state->local_candidate_count >= MAX_CANDIDATES) {
        return;
    }
    strncpy(state->local_candidates[state->local_candidate_count], cand, MAX_CANDIDATE_LENGTH);
    state->local_candidate_count++;
    render_current_sdp(state);
}

void RTC_API on_data_channel_error(int id, const char *error, void *ptr) {
    printf("Error on Data Channel: %s\n", error);
}

void RTC_API on_data_channel_open(int id, void *ptr) {
    printf("Data channel open!\n");
    struct rtc_state* state = (struct rtc_state*)ptr;
    state->channel_open = true;
}

void RTC_API on_data_channel_close(int id, void *ptr) {
    printf("Data channel closed\n");
    struct rtc_state* state = (struct rtc_state*)ptr;
    state->channel_open = false;
}

void RTC_API on_data_channel_message(int id, const char *message, int size, void *ptr) {
    printf("Received message: (%d) %s\n", size, message);
}

int main(void) {
    const char *stun_server = getenv("STUN_SERVER");
    if (stun_server == NULL) {
        printf("A STUN server is required!\n");
        return 1;
    }
    struct rtc_state state = {0};

    rtcInitLogger(RTC_LOG_WARNING, &logger);

    rtcConfiguration config = {
            .iceServersCount = 1,
            .iceServers = &stun_server,
            .disableAutoNegotiation = false,
            .forceMediaTransport = false,
    };

    int pc = rtcCreatePeerConnection(&config);
    rtcSetUserPointer(pc, &state);
    int result;
    result = rtcSetLocalDescriptionCallback(pc, on_local_description);
    if (result != 0) {
        printf("Failed: %d\n", result);
        return 1;
    }
    result = rtcSetLocalCandidateCallback(pc, on_local_candidate);
    if (result != 0) {
        printf("Failed: %d\n", result);
        return 1;
    }
    result = rtcSetIceStateChangeCallback(pc, on_ice_state_changed);
    if (result != 0) {
        printf("Failed: %d\n", result);
        return 1;
    }
    result = rtcSetGatheringStateChangeCallback(pc, on_ice_gathering_state_changed);
    if (result != 0) {
        printf("Failed: %d\n", result);
        return 1;
    }

    int dc = rtcCreateDataChannel(pc, "test");
    result = rtcSetErrorCallback(dc, on_data_channel_error);
    if (result) {
        printf("Failed: %d", result);
        return 1;
    }
    result = rtcSetOpenCallback(dc, on_data_channel_open);
    if (result) {
        printf("Failed: %d", result);
        return 1;
    }
    result = rtcSetClosedCallback(dc, on_data_channel_close);
    if (result) {
        printf("Failed: %d", result);
        return 1;
    }
    result = rtcSetMessageCallback(dc, on_data_channel_message);
    if (result) {
        printf("Failed: %d", result);
        return 1;
    }

    printf("Input remote description:\n");
    size_t remaining_space = MAX_SDP_SIZE;
    char remote_description[remaining_space];
    char* s = remote_description;
    while (fgets(s, (int)remaining_space, stdin)) {
        size_t len = strlen(s);
        if (len == 1) {
            break;
        }
        s += len;
        remaining_space -= len;
    }

    result = rtcSetRemoteDescription(pc, remote_description, "answer");
    if (result) {
        printf("rtcSetRemoteDescription failed: %d\n", result);
        return 1;
    }

    while (!state.channel_open) {
        printf("Channel not connected yet!\n");
        sleep(1);
    }

    while (state.channel_open) {
        printf("Channel connected!\n");
        sleep(1);
    }

    return 0;
}
