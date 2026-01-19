// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <httpseeds.h>

#include <logging.h>
#include <util/system.h>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/dns.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <event2/util.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <sstream>

// Simple HTTP GET using libevent (already a dependency)
static std::string SimpleHTTPGet(const std::string& url, int timeout_seconds = 10)
{
    std::string result;

    // Parse URL
    struct evhttp_uri* uri = evhttp_uri_parse(url.c_str());
    if (!uri) {
        LogPrintf("HTTPSeeds: Failed to parse URL: %s\n", url);
        return result;
    }

    const char* host = evhttp_uri_get_host(uri);
    int port = evhttp_uri_get_port(uri);
    const char* path = evhttp_uri_get_path(uri);
    const char* query = evhttp_uri_get_query(uri);

    if (!host) {
        evhttp_uri_free(uri);
        return result;
    }

    if (port == -1) {
        const char* scheme = evhttp_uri_get_scheme(uri);
        if (scheme && strcmp(scheme, "https") == 0) {
            // HTTPS not directly supported by libevent, skip
            LogPrintf("HTTPSeeds: HTTPS not supported, skipping: %s\n", url);
            evhttp_uri_free(uri);
            return result;
        }
        port = 80;
    }

    std::string fullpath = path ? path : "/";
    if (query) {
        fullpath += "?";
        fullpath += query;
    }

    // Create event base
    struct event_base* base = event_base_new();
    if (!base) {
        evhttp_uri_free(uri);
        return result;
    }

    // Create connection
    struct evhttp_connection* conn = evhttp_connection_base_new(base, nullptr, host, port);
    if (!conn) {
        event_base_free(base);
        evhttp_uri_free(uri);
        return result;
    }

    evhttp_connection_set_timeout(conn, timeout_seconds);

    // Create request
    struct evhttp_request* req = evhttp_request_new(
        [](struct evhttp_request* req, void* arg) {
            std::string* result = static_cast<std::string*>(arg);
            if (req && evhttp_request_get_response_code(req) == 200) {
                struct evbuffer* buf = evhttp_request_get_input_buffer(req);
                size_t len = evbuffer_get_length(buf);
                if (len > 0) {
                    char* data = new char[len + 1];
                    evbuffer_copyout(buf, data, len);
                    data[len] = '\0';
                    *result = data;
                    delete[] data;
                }
            }
        },
        &result
    );

    if (!req) {
        evhttp_connection_free(conn);
        event_base_free(base);
        evhttp_uri_free(uri);
        return result;
    }

    // Set headers
    struct evkeyvalq* headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(headers, "Host", host);
    evhttp_add_header(headers, "User-Agent", "Coral-Core/1.0");
    evhttp_add_header(headers, "Connection", "close");

    // Make request
    if (evhttp_make_request(conn, req, EVHTTP_REQ_GET, fullpath.c_str()) != 0) {
        evhttp_connection_free(conn);
        event_base_free(base);
        evhttp_uri_free(uri);
        return result;
    }

    // Run event loop with timeout
    struct timeval tv;
    tv.tv_sec = timeout_seconds;
    tv.tv_usec = 0;
    event_base_loopexit(base, &tv);
    event_base_dispatch(base);

    evhttp_connection_free(conn);
    event_base_free(base);
    evhttp_uri_free(uri);

    return result;
}

std::vector<std::string> ParseSeedFile(const std::string& content)
{
    std::vector<std::string> seeds;
    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        // Trim whitespace
        auto start = std::find_if_not(line.begin(), line.end(), [](unsigned char c) { return std::isspace(c); });
        auto end = std::find_if_not(line.rbegin(), line.rend(), [](unsigned char c) { return std::isspace(c); }).base();

        if (start >= end) continue;
        line = std::string(start, end);

        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;

        // Basic validation: should contain a colon for port
        if (line.find(':') != std::string::npos) {
            seeds.push_back(line);
        }
    }

    return seeds;
}

std::vector<std::string> FetchHTTPSeeds(bool testnet)
{
    std::vector<std::string> seeds;

    // Try each URL in order
    for (const auto& url : HTTP_SEED_URLS) {
        LogPrint(BCLog::NET, "HTTPSeeds: Fetching seeds from %s\n", url);

        // Note: GitHub raw URLs use HTTPS which isn't directly supported
        // For now, this serves as infrastructure for when HTTP sources are available
        std::string content = SimpleHTTPGet(url, 15);

        if (!content.empty()) {
            auto parsed = ParseSeedFile(content);
            if (!parsed.empty()) {
                LogPrintf("HTTPSeeds: Got %d seeds from %s\n", parsed.size(), url);
                seeds.insert(seeds.end(), parsed.begin(), parsed.end());
                break;  // Success, don't try other URLs
            }
        }
    }

    if (seeds.empty()) {
        LogPrint(BCLog::NET, "HTTPSeeds: No seeds found from HTTP sources\n");
    }

    return seeds;
}
