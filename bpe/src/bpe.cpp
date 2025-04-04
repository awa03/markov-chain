#include "bpe.h"

BytePairEncoded* genBytePairEncoding(const char* file_path, size_t v_size = DEFAULT_VOCAB_SIZE){
    std::ifstream file(file_path);

    if(!file.is_open()){
        std::cerr << "Error opening file path" << "\n";
        return nullptr;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    std::string content = buffer.str();

    if(content.size() <= 1){
        std::cerr << "Filesize error" << "\n";
        return nullptr;
    }

    file.close();

    BytePairEncoded* data = new BytePairEncoded(v_size);

    data->populateDict(content);
    bool stop_flag = false;

    // need to use while to prevent recursion depth from being reached
    while(!stop_flag){
        Pair most_used_pair = {' ', ' ', 0}; // the pair that has been used the most
        // encode -- set these two vars
        data->encode(content, most_used_pair);
        if(data->vocab.size() >= v_size){
            break;
        }
        if(most_used_pair.freq == 1){
            break;
        }
    }

    cout << content << "\n";
    std::cout << "\n\nVocab\n-------------\n";


    data->j_data["encoded"] = content;
    data->j_data["vocab"] = data->dumpVocab();

    return data;
}

void BytePairEncoded::dumpToFile(const char* file_path){
    std::ofstream outFile(file_path);
    outFile << j_data.dump();
    outFile.close();
}

json BytePairEncoded::dumpVocab(){
    json j_array = json::array();
    for(auto& term : vocab){
        json entry;
        entry["pair"] = term.first.first;
        entry["string"] = term.second;
        entry["freq"] = term.first.freq;
        j_array.push_back(entry);
    }
    return j_array;
}

void BytePairEncoded::populateDict(string& content){
    for(auto& c: content){
        dict.insert(c);
    }
}

void BytePairEncoded::encode(std::string& content, Pair& most_used_pair) {
    // Reset the most frequent pair
    most_used_pair.freq = 0;

    // Create frequency map of adjacent character pairs
    std::unordered_map<Pair, int, PairHash> pair_freq;

    // Count frequencies of all pairs in the content
    for(size_t i = 0; i < content.size() - 1; i++) {
        Pair tmp = {content[i], content[i + 1], 1};
        auto find = pair_freq.find(tmp);

        // Update frequency
        if(find != pair_freq.end()) {
            pair_freq[tmp]++;
            auto find_vocab = vocab.find(tmp);
            if(find_vocab != vocab.end()){
                std::string re= find_vocab->second;
                vocab.erase(find_vocab);
                vocab[tmp] = re;
            }
        } else {
            pair_freq[tmp] = 1;
        }
    }

    // Find the most frequent pair
    for(const auto& pair : pair_freq) {
        if(pair.second > most_used_pair.freq) {
            most_used_pair = pair.first;
            most_used_pair.freq = pair.second;
        }
    }

    // Only replace if the most frequent pair appears more than once
    if(most_used_pair.freq > 1) {
        std::string replacement;
        auto find_vocab = vocab.find(most_used_pair);

        if(find_vocab != vocab.end()) {
            replacement = find_vocab->second;
        } else {
            replacement = getUnusedChar();
            char replace_char = replacement[0];
            dict.insert(replace_char);
            vocab[most_used_pair] = replacement;
        }

        // Replace all occurrences of the pair with the replacement character
        std::string pair_str = std::string(1, most_used_pair.first) + std::string(1, most_used_pair.second);
        size_t pos = 0;
        while((pos = content.find(pair_str, pos)) != std::string::npos) {
            content.replace(pos, 2, replacement);
            pos += replacement.length();
        }
    }
}

bool BytePairEncoded::stopEncoding(int most_used_freq){
    if(most_used_freq == 1){
        return true;
    }

    return true;
}

void BytePairEncoded::insertPairToVocab(char first, char second, const string& replacement){
    Pair p = {first, second, 0};
    vocab[p] = replacement;
}

char BytePairEncoded::getUnusedChar() {
    for (unsigned char c = 0; c < 255; c++) {
         if (dict.find(c) == dict.end()) {
             return c;
         }
     }

    for (unsigned char c = 33; c < 127; c++) {
        if (c != '"' && c != '\'' && c != '\\' && c != '/' &&
            c != '{' && c != '}' && c != '[' && c != ']' &&
            dict.find(c) == dict.end()) {
            return c;
        }
    }

    throw std::runtime_error("No unused characters available for replacement");
}
