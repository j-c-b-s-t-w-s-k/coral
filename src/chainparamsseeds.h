#ifndef CORAL_CHAINPARAMSSEEDS_H
#define CORAL_CHAINPARAMSSEEDS_H
/**
 * List of fixed seed nodes for the Coral network
 *
 * BOOTSTRAP OPTIONS:
 *
 * 1. Command line (easiest for testing):
 *    coral-qt -addnode=<ip>:8334
 *
 * 2. Add seed node IPs below using BIP155 format:
 *    0x01,0x04,<IP_B1>,<IP_B2>,<IP_B3>,<IP_B4>,<PORT_HI>,<PORT_LO>
 *
 *    Example for 203.0.113.50:8334 (port 8334 = 0x208e):
 *    0x01,0x04,0xcb,0x00,0x71,0x32,0x20,0x8e,
 *
 * 3. Set up DNS seeds and add to chainparams.cpp:
 *    vSeeds.emplace_back("seed.coralnetwork.org");
 */

// Coral mainnet seeds (port 8334 = 0x208e)
static const uint8_t chainparams_seed_main[] = {
    // Add your seed node IPs here
    // Example: 0x01,0x04,0xcb,0x00,0x71,0x32,0x20,0x8e,  // 203.0.113.50:8334
};

// Coral testnet seeds (port 18334 = 0x479e)
static const uint8_t chainparams_seed_test[] = {
    // Add testnet seed IPs here
};

#endif // CORAL_CHAINPARAMSSEEDS_H
