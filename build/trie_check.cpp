#include <iostream>
#include <vector>
#include <string>
#include "D:/SY/sem2/DS/cpp/ds/Trie.h"

int main() {
    Trie trie;
    trie.insert("Nashik", 1);
    trie.insert("Nagpur", 2);
    trie.insert("Pune", 3);
    trie.insert("Mumbai", 4);

    const std::vector<std::string> prefixes = {"N", "Na", "Nas", "Nag", "P", "M"};
    for (const auto& prefix : prefixes) {
        const auto results = trie.autocomplete(prefix, 10);
        std::cout << prefix << ':';
        for (std::size_t i = 0; i < results.size(); ++i) {
            if (i > 0) std::cout << ',';
            std::cout << results[i];
        }
        std::cout << '\n';
    }
    return 0;
}
