// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_HTTPSEEDS_H
#define CORAL_HTTPSEEDS_H

#include <string>
#include <vector>

/**
 * HTTP-based seed node discovery.
 * Fetches seed node IPs from a URL (e.g., GitHub raw file).
 *
 * Seed file format (one per line):
 *   # Comment lines start with #
 *   192.168.1.100:8334
 *   seed1.example.com:8334
 */

// URLs to fetch seed nodes from (tried in order)
const std::vector<std::string> HTTP_SEED_URLS = {
    "https://coral.directory/seeds.txt",
    "https://raw.githubusercontent.com/j-c-b-s-t-w-s-k/coral/main/seeds.txt",
};

/**
 * Fetch seed nodes from HTTP sources.
 * @param testnet If true, fetch testnet seeds instead
 * @return Vector of "host:port" strings
 */
std::vector<std::string> FetchHTTPSeeds(bool testnet = false);

/**
 * Parse a seed file content into host:port pairs.
 * @param content Raw file content
 * @return Vector of "host:port" strings
 */
std::vector<std::string> ParseSeedFile(const std::string& content);

#endif // CORAL_HTTPSEEDS_H
