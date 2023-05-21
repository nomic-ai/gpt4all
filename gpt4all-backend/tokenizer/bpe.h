#pragma once
#include <unicode/regex.h>
#include <unicode/unistr.h>

#include <cstdint>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string_view>

namespace bpecpp {
typedef std::pair<icu::UnicodeString, icu::UnicodeString> UnicodeBigram;

class bpe_char_byte_table {
   public:
    bpe_char_byte_table() {
        int n = 0;
        for (uint8_t byte = 0; m_codepoint_to_byte.size() < 256; byte++) {
            bool keep = (byte >= '!' && byte <= '~') ||
                        (byte >= 0xa1 && byte <= 0xac) ||
                        (byte >= 0xae && byte <= 0xff);
            uint32_t codepoint = byte;
            if (!keep) {
                codepoint = 256 + n;
                n++;
            }
            m_byte_to_codepoint[byte] = codepoint;
            m_codepoint_to_byte[codepoint] = byte;
        };
    }
    uint32_t byte_to_codepoint(uint8_t byte) {
        return m_byte_to_codepoint[byte];
    }

    uint8_t codepoint_to_byte(uint32_t codepoint) {
        return m_codepoint_to_byte.at(codepoint);
    }

   private:
    std::array<uint32_t, 256> m_byte_to_codepoint;
    std::unordered_map<uint32_t, uint8_t> m_codepoint_to_byte;
};

struct bigram_hash {
    std::size_t operator()(const UnicodeBigram& pair) const {
        return pair.first.hashCode() + pair.second.hashCode();
    }
};

struct icu_hash {
    std::size_t operator()(const icu::UnicodeString& us) const {
        return us.hashCode();
    }
};

class BPE {
   public:
    BPE(const std::unordered_map<std::string_view, uint32_t> &vocab,
        const std::vector<std::pair<std::string_view, std::string_view>> &merges);

    std::vector<uint32_t> encode(const std::string& input);

    std::string decode(const std::vector<uint32_t>& tokens,
                       bool valid_utf8 = true);

   private:
    std::unordered_map<icu::UnicodeString, uint32_t, icu_hash> m_vocab;
    std::unordered_map<uint32_t, icu::UnicodeString> m_reverse_vocab;
    std::unordered_map<UnicodeBigram, size_t, bigram_hash> m_merges;
    bpe_char_byte_table m_bs_table;

    void bpe(icu::UnicodeString token_pretoked,
             std::vector<icu::UnicodeString>& output);
    std::unique_ptr<icu::RegexPattern> m_pretok_re;
    std::string normalize_nfc(const std::string& input);
    std::vector<icu::UnicodeString> pretokenize(const std::string& input);
};

// for embedding tokenizer configs in the library - had initially constructed
// `string_view`s in the generated headers, *but* generating thousands actual
// references into the buffer generates thousands of *relocations* and makes
// compilation rather slow, delaying resolving the real address into a
// string_view until runtime fixes that
struct buf_ref {
    // packing these into a single u32 reduces the size of the embedded
    // configs significantly (5.0MB->1.6MB)
    uint32_t offset : 20;
    uint32_t length : 12;

    std::string_view into(const char* buf) {
        return std::string_view(&buf[offset], length);
    }
};
struct additional_vocab_item_embedded {
    uint32_t id;
    buf_ref content;
    bool special;
};
struct additional_vocab_item {
    uint32_t id;
    std::string_view content;
    bool special = false;
};
class AdditionalVocabAdapter {
   public:
    AdditionalVocabAdapter(const std::vector<additional_vocab_item> &vocab);
    std::vector<uint32_t> encode(const std::string& input,
                                 BPE& bpemodel,
                                 bool encode_special_tokens = true);
    std::string decode(const std::vector<uint32_t>& tokens,
                       BPE& bpemodel,
                       bool decode_special_tokens = true,
                       bool valid_utf8 = true);

   private:
    std::unordered_map<std::string_view, uint32_t> m_token_to_id;
    std::unordered_map<uint32_t, std::string_view> m_id_to_token;
    std::unordered_set<uint32_t> m_special_ids;
    std::regex m_addedtoken_re;
};

}  // namespace bpecpp
