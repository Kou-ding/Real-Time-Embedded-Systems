#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libwebsockets.h>

static int interrupted;
static struct lws *client_wsi;

typedef struct {
    char symbol[50];
    char api_key[100];
} context_t;

static void parse_price(const char *message) {
    const char *price_key = "\"price\":";
    char *price_start;
    char *price_end;
    double price;

    // Find the start of the price value
    price_start = strstr(message, price_key);
    if (price_start) {
        price_start += strlen(price_key);

        // Convert the value to a double
        price = strtod(price_start, &price_end);
        if (price_start != price_end) {
            printf("Apple stock price: %f\n", price);
        } else {
            printf("Price value is missing or invalid\n");
        }
    } else {
        printf("Price key not found in the message\n");
    }
}

static int callback_function(struct lws *wsi, enum lws_callback_reasons reason,
                             void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("Connection established\n");
            {
                context_t *ctx = (context_t *)user;
                char msg[256];
                sprintf(msg, "{\"type\":\"subscribe\",\"symbol\":\"%s\"}", ctx->symbol);
                lws_write(wsi, (unsigned char *)msg, strlen(msg), LWS_WRITE_TEXT);
            }
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            {
                char *received_message = (char *)in;
                printf("Received: %s\n", received_message);

                // Parse the received message to extract the price
                parse_price(received_message);
            }
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf("Connection error\n");
            interrupted = 1;
            break;
        case LWS_CALLBACK_CLIENT_CLOSED:
            printf("Connection closed\n");
            interrupted = 1;
            break;
        default:
            break;
    }
    return 0;
}

static const struct lws_protocols protocols[] = {
    {
        "example-protocol",
        callback_function,
        sizeof(context_t), // User data size is context_t
        65536,
    },
    { NULL, NULL, 0, 0 }
};

int main(void) {
    context_t ctx = {
        .symbol = "AAPL",
        .api_key = "cq6i3lhr01qlbj5047ugcq6i3lhr01qlbj5047v0"
    };

    struct lws_context_creation_info info;
    struct lws_client_connect_info ccinfo;
    struct lws_context *context;

    memset(&info, 0, sizeof info);
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

    context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "lws_create_context failed\n");
        return 1;
    }

    memset(&ccinfo, 0, sizeof(ccinfo));
    ccinfo.context = context;
    ccinfo.address = "ws.finnhub.io";
    ccinfo.port = 443;
    ccinfo.path = "/?token=cq6i3lhr01qlbj5047ugcq6i3lhr01qlbj5047v0"; // Replace with your Finnhub API key
    ccinfo.host = lws_canonical_hostname(context);
    ccinfo.origin = "origin";
    ccinfo.protocol = protocols[0].name;
    ccinfo.ssl_connection = LCCSCF_USE_SSL;
    ccinfo.pwsi = &client_wsi;
    ccinfo.userdata = &ctx; // Pass the context containing symbol and API key

    lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG | LLL_PARSER | LLL_HEADER, NULL);

    if (!lws_client_connect_via_info(&ccinfo)) {
        fprintf(stderr, "lws_client_connect_via_info failed\n");
        lws_context_destroy(context);
        return 1;
    }

    while (!interrupted) {
        lws_service(context, 1000);
    }

    lws_context_destroy(context);
    return 0;
}
