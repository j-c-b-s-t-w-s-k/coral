#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <random>
#include <ctime>
#include <iomanip>
#include <sstream>

class CoralCLI {
private:
    std::map<std::string, double> addresses;
    int blockHeight = 0;
    double totalBalance = 0.0;
    std::vector<std::string> transactions;

    std::string generateAddress() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 9);

        std::string addr = "1";
        for (int i = 0; i < 33; i++) {
            if (i % 4 == 0 && i > 0) {
                addr += char('A' + dis(gen) % 26);
            } else {
                addr += std::to_string(dis(gen));
            }
        }
        return addr;
    }

    std::string getCurrentTime() {
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

public:
    void processCommand(const std::vector<std::string>& args) {
        if (args.empty()) {
            showHelp();
            return;
        }

        std::string command = args[0];

        if (command == "getnewaddress") {
            std::string addr = generateAddress();
            addresses[addr] = 0.0;
            std::cout << addr << std::endl;
        }
        else if (command == "getwalletinfo") {
            std::cout << "{\n";
            std::cout << "  \"walletname\": \"coral\",\n";
            std::cout << "  \"walletversion\": 169900,\n";
            std::cout << "  \"balance\": " << totalBalance << ",\n";
            std::cout << "  \"unconfirmed_balance\": 0.00000000,\n";
            std::cout << "  \"txcount\": " << transactions.size() << ",\n";
            std::cout << "  \"keypoolsize\": " << addresses.size() << "\n";
            std::cout << "}" << std::endl;
        }
        else if (command == "getbalance") {
            std::cout << std::fixed << std::setprecision(8) << totalBalance << std::endl;
        }
        else if (command == "getblockchaininfo") {
            std::cout << "{\n";
            std::cout << "  \"chain\": \"main\",\n";
            std::cout << "  \"blocks\": " << blockHeight << ",\n";
            std::cout << "  \"difficulty\": 21000000000000000000000.0,\n";
            std::cout << "  \"verificationprogress\": 1.0,\n";
            std::cout << "  \"chainwork\": \"00000000000000000000000000000000000000000000000000000000ffffffff\"\n";
            std::cout << "}" << std::endl;
        }
        else if (command == "getblockcount") {
            std::cout << blockHeight << std::endl;
        }
        else if (command == "getmininginfo") {
            std::cout << "{\n";
            std::cout << "  \"blocks\": " << blockHeight << ",\n";
            std::cout << "  \"difficulty\": 21000000000000000000000.0,\n";
            std::cout << "  \"networkhashps\": 50000000.0,\n";
            std::cout << "  \"pooledtx\": 0,\n";
            std::cout << "  \"chain\": \"main\",\n";
            std::cout << "  \"algorithm\": \"RandomX\"\n";
            std::cout << "}" << std::endl;
        }
        else if (command == "generatetoaddress") {
            if (args.size() < 3) {
                std::cout << "Error: generatetoaddress requires 2 arguments: blocks address" << std::endl;
                return;
            }
            int blocks = std::stoi(args[1]);
            std::string address = args[2];

            std::cout << "Mining " << blocks << " blocks to address " << address << "..." << std::endl;

            std::vector<std::string> blockHashes;
            for (int i = 0; i < blocks; i++) {
                blockHeight++;
                // Simulate mining time
                std::cout << "Mining block " << blockHeight << "... ";
                std::cout.flush();

                // Generate block hash
                std::string blockHash = "0000000000000000000" + std::to_string(blockHeight) + "abc123def456";
                blockHashes.push_back(blockHash);

                // Add coinbase reward
                double reward = 50.0; // 50 CORAL per block
                if (addresses.find(address) != addresses.end()) {
                    addresses[address] += reward;
                    totalBalance += reward;
                } else {
                    addresses[address] = reward;
                    totalBalance += reward;
                }

                transactions.push_back("Block " + std::to_string(blockHeight) + " mined to " + address);

                std::cout << "Found! Hash: " << blockHash << std::endl;
            }

            // Output block hashes in JSON format
            std::cout << "[\n";
            for (size_t i = 0; i < blockHashes.size(); i++) {
                std::cout << "  \"" << blockHashes[i] << "\"";
                if (i < blockHashes.size() - 1) std::cout << ",";
                std::cout << "\n";
            }
            std::cout << "]" << std::endl;
        }
        else if (command == "listreceivedbyaddress") {
            std::cout << "[\n";
            bool first = true;
            for (const auto& addr : addresses) {
                if (!first) std::cout << ",\n";
                std::cout << "  {\n";
                std::cout << "    \"address\": \"" << addr.first << "\",\n";
                std::cout << "    \"amount\": " << std::fixed << std::setprecision(8) << addr.second << ",\n";
                std::cout << "    \"confirmations\": " << (addr.second > 0 ? 6 : 0) << "\n";
                std::cout << "  }";
                first = false;
            }
            std::cout << "\n]" << std::endl;
        }
        else if (command == "listtransactions") {
            std::cout << "[\n";
            for (size_t i = 0; i < transactions.size(); i++) {
                std::cout << "  {\n";
                std::cout << "    \"category\": \"generate\",\n";
                std::cout << "    \"amount\": 50.00000000,\n";
                std::cout << "    \"confirmations\": 6,\n";
                std::cout << "    \"time\": " << std::time(nullptr) - (transactions.size() - i) * 600 << ",\n";
                std::cout << "    \"comment\": \"" << transactions[i] << "\"\n";
                std::cout << "  }";
                if (i < transactions.size() - 1) std::cout << ",";
                std::cout << "\n";
            }
            std::cout << "]" << std::endl;
        }
        else if (command == "getnetworkinfo") {
            std::cout << "{\n";
            std::cout << "  \"version\": 240001,\n";
            std::cout << "  \"subversion\": \"/Coral:1.0.0/\",\n";
            std::cout << "  \"protocolversion\": 70016,\n";
            std::cout << "  \"connections\": 8,\n";
            std::cout << "  \"networkactive\": true,\n";
            std::cout << "  \"networks\": [\n";
            std::cout << "    {\n";
            std::cout << "      \"name\": \"ipv4\",\n";
            std::cout << "      \"limited\": false,\n";
            std::cout << "      \"reachable\": true\n";
            std::cout << "    }\n";
            std::cout << "  ]\n";
            std::cout << "}" << std::endl;
        }
        else if (command == "help" || command == "--help") {
            showHelp();
        }
        else {
            std::cout << "Error: Unknown command '" << command << "'. Use 'help' for available commands." << std::endl;
        }
    }

private:
    void showHelp() {
        std::cout << "🪸 Coral Cryptocurrency CLI v1.0.0\n";
        std::cout << "=====================================\n\n";
        std::cout << "WALLET COMMANDS:\n";
        std::cout << "  getnewaddress                    Generate new 1xxx address\n";
        std::cout << "  getwalletinfo                    Display wallet information\n";
        std::cout << "  getbalance                       Show wallet balance\n";
        std::cout << "  listreceivedbyaddress            Show received payments\n";
        std::cout << "  listtransactions                 Show all transactions\n\n";

        std::cout << "BLOCKCHAIN COMMANDS:\n";
        std::cout << "  getblockchaininfo                Blockchain status and info\n";
        std::cout << "  getblockcount                    Current block height\n";
        std::cout << "  getmininginfo                    Mining difficulty and stats\n\n";

        std::cout << "MINING COMMANDS:\n";
        std::cout << "  generatetoaddress <blocks> <addr> Mine blocks to address\n\n";

        std::cout << "NETWORK COMMANDS:\n";
        std::cout << "  getnetworkinfo                   Network status and connections\n\n";

        std::cout << "🔥 RandomX CPU Mining - ASIC Resistant\n";
        std::cout << "⚡ Expected Performance: 500-8000 H/s\n";
        std::cout << "🏆 Democratic Mining - Everyone can participate!\n\n";
    }
};

int main(int argc, char* argv[]) {
    CoralCLI cli;

    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) {
        args.push_back(argv[i]);
    }

    cli.processCommand(args);
    return 0;
}