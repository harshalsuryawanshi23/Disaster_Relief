#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct TrieNode {
    std::unordered_map<char, TrieNode*> children;
    bool is_end;
    int zone_id;
    int frequency;
    std::string full_name;

    TrieNode();
};

class Trie {
public:
    Trie();
    ~Trie();

    Trie(const Trie&) = delete;
    Trie& operator=(const Trie&) = delete;

    void insert(const std::string& name, int zone_id);
    bool search(const std::string& name);
    bool starts_with(const std::string& prefix);
    std::vector<std::string> autocomplete(const std::string& prefix, int limit = 10);
    int get_zone_id(const std::string& name);
    void increment_frequency(const std::string& name);
    std::string export_json() const;

private:
    TrieNode* root;

    void collect_all(const TrieNode* node,
                     std::vector<std::pair<int, std::string>>& result) const;
    void delete_tree(TrieNode* node);
    const TrieNode* traverse(const std::string& key) const;
    TrieNode* traverse_mutable(const std::string& key);
    void export_node_json(const TrieNode* node, char edge_label, std::string& out) const;
};
