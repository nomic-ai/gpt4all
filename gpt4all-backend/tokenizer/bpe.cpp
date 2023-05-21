#include "bpe.h"
#include <unicode/normalizer2.h>
#include <unicode/regex.h>
#include <unicode/schriter.h>
#include <unicode/unistr.h>

#include <regex>
#include <stdexcept>
#include <iostream>

namespace bpecpp {
const std::string_view BPE_PRETOK_REGEX =
    R"('s|'t|'re|'ve|'m|'ll|'d| ?[[:alpha:]]+| ?[[:digit:]]+| ?[^\s[:alpha:][:digit:]]+|\s+(?!\S)|\s+)";

static void get_bigrams(const std::vector<icu::UnicodeString>& input,
                        std::unordered_set<UnicodeBigram, bigram_hash>& pairs) {
    pairs.clear();
    auto i = input.begin();
    auto prev = *i++;
    for (; i != input.end(); ++i) {
        pairs.insert({prev, *i});
        prev = *i;
    }
}

BPE::BPE(const std::unordered_map<std::string_view, uint32_t>& vocab,
         const std::vector<std::pair<std::string_view, std::string_view>>& merges) {
    for (auto pair : vocab) {
        icu::UnicodeString encd = icu::UnicodeString::fromUTF8(pair.first);
        m_vocab[encd] = pair.second;
        m_reverse_vocab[pair.second] = encd;
    }
    size_t n = 0;
    for (auto merge : merges) {
        auto left = icu::UnicodeString::fromUTF8(merge.first);
        auto right = icu::UnicodeString::fromUTF8(merge.second);
        m_merges[{left, right}] = n++;
    }
}

std::vector<uint32_t> BPE::encode(const std::string& input) {
    auto normalized = normalize_nfc(input);
    auto pretokenized = pretokenize(normalized);
    std::vector<icu::UnicodeString> tokens_merged;
    for (auto &ptok : pretokenized) {
        bpe(ptok, tokens_merged);
    }
    std::vector<uint32_t> final_tokens;
    for (auto &mtok : tokens_merged) {
        final_tokens.push_back(m_vocab[mtok]);
    }
    return final_tokens;
}

std::string BPE::decode(const std::vector<uint32_t>& tokens, bool valid_utf8) {
    std::string out;
    for (uint32_t t : tokens) {
        icu::UnicodeString benc = m_reverse_vocab[t];
        icu::StringCharacterIterator schriter(benc);
        for (UChar32 c = schriter.first32(); schriter.hasNext();
             c = schriter.next32()) {
            out.push_back(m_bs_table.codepoint_to_byte((uint32_t)c));
        }
    }
    // roundtrip through ICU to replace invalid utf8 with U+FFFD
    if (valid_utf8) {
        auto tmp = icu::UnicodeString::fromUTF8(out);
        out.clear();
        tmp.toUTF8String(out);
    }
    return out;
}

// https://github.com/karpathy/minGPT/blob/37baab71b9abea1b76ab957409a1cc2fbfba8a26/mingpt/bpe.py#L95
void BPE::bpe(icu::UnicodeString token_pretoked,
              std::vector<icu::UnicodeString>& output) {
    if (token_pretoked.length() < 2) {
        output.push_back(token_pretoked);
        return;
    }
    std::vector<icu::UnicodeString> words;
    std::vector<icu::UnicodeString> words_update;
    icu::StringCharacterIterator schriter(token_pretoked);
    UChar32 c;
    for (schriter.setToStart(); schriter.hasNext();) {
        c = schriter.next32PostInc();
        icu::UnicodeString w;
        w.append(c);
        words.push_back(w);
    }
    std::unordered_set<UnicodeBigram, bigram_hash> pairs;
    get_bigrams(words, pairs);
    while (true) {
        size_t min_rank = SIZE_MAX;
        UnicodeBigram to_merge;
        for (auto &bigram : pairs) {
            auto loc = m_merges.find(bigram);
            if (loc != m_merges.end() && loc->second < min_rank) {
                min_rank = loc->second;
                to_merge = loc->first;
            }
        }
        if (min_rank == SIZE_MAX) {
            break;
        } else {
            auto i = words.begin();
            while (i < words.end()) {
                if (*i == to_merge.first) {
                    auto inext = i;
                    inext++;
                    if (inext != words.end() && *inext == to_merge.second) {
                        words_update.push_back(*i + *inext);
                        i = inext;
                    } else {
                        words_update.push_back(*i);
                    }
                } else {
                    words_update.push_back(*i);
                }
                ++i;
            }
            words.swap(words_update);
            words_update.clear();
            get_bigrams(words, pairs);
        }
    }
    output.insert(output.end(), words.begin(), words.end());
}

std::string BPE::normalize_nfc(const std::string& input) {
    UErrorCode uerror = U_ZERO_ERROR;
    auto nfcnorm = icu::Normalizer2::getNFCInstance(uerror);
    if (!U_SUCCESS(uerror))
        throw std::runtime_error("could not get ICU NFC normalizer");
    auto icu_ti = icu::UnicodeString::fromUTF8(input);
    std::string out;
    nfcnorm->normalize(icu_ti, uerror).toUTF8String(out);
    if (!U_SUCCESS(uerror))
        throw std::runtime_error("ICU string normalization failed");
    return out;
}

std::vector<icu::UnicodeString> BPE::pretokenize(const std::string& input) {
    UParseError pe;
    UErrorCode uerror = U_ZERO_ERROR;
    auto bpe_re_icustr = icu::UnicodeString::fromUTF8(BPE_PRETOK_REGEX);
    if (m_pretok_re == nullptr) {
        m_pretok_re = std::unique_ptr<icu::RegexPattern>(
            icu::RegexPattern::compile(bpe_re_icustr, pe, uerror));
        if (!U_SUCCESS(uerror))
            throw std::runtime_error("Compiling BPE pretokenizer regex failed");
    }
    auto uinput = icu::UnicodeString::fromUTF8(input);
    std::unique_ptr<icu::RegexMatcher> pretok_matcher(
        m_pretok_re->matcher(uinput, uerror));
    std::vector<icu::UnicodeString> pretoks;
    if (!U_SUCCESS(uerror))
        throw std::runtime_error("Creating BPE pretokenizer matcher failed");
    while (pretok_matcher->find()) {
        auto match = pretok_matcher->group(uerror);
        if (!U_SUCCESS(uerror))
            throw std::runtime_error(
                "Getting BPE pretokenizer regex match failed");
        std::string s;
        icu::UnicodeString out;
        match.toUTF8String(s);
        for (char c : s) {
            uint32_t codepoint = m_bs_table.byte_to_codepoint((uint8_t)c);
            out.append((UChar32)codepoint);
        }
        pretoks.push_back(out);
    }
    return pretoks;
}

static std::string regex_escape(const std::string_view inp) {
    std::string s(inp);
    static const std::regex metacharacters(R"([\.\^\$\-\+\(\)\[\]\{\}\|\?\*])");
    return std::regex_replace(s, metacharacters, "\\$&");
}

AdditionalVocabAdapter::AdditionalVocabAdapter(
    const std::vector<additional_vocab_item>& vocab) {
    std::string addedtoken_regex;
    for (const additional_vocab_item& item : vocab) {
        if (!addedtoken_regex.empty()) {
            addedtoken_regex += "|";
        }
        addedtoken_regex += regex_escape(item.content);
        m_token_to_id[item.content] = item.id;
        m_id_to_token[item.id] = item.content;
        if (item.special) {
            m_special_ids.insert(item.id);
        }
    }
    m_addedtoken_re = std::regex(addedtoken_regex);
}

std::vector<uint32_t> AdditionalVocabAdapter::encode(
    const std::string& input,
    BPE& bpemodel,
    bool encode_special_tokens) {
    if (m_token_to_id.empty()) {
        return bpemodel.encode(input);
    }
    std::vector<uint32_t> out;
    std::string work = input;
    std::smatch m;
    while (std::regex_search(work, m, m_addedtoken_re)) {
        auto tokloc = m_token_to_id.find(m.str());
        if (tokloc != m_token_to_id.end()) {
            auto tokid = tokloc->second;
            auto prefix_decoded = bpemodel.encode(m.prefix());
            out.insert(out.end(), prefix_decoded.begin(), prefix_decoded.end());
            bool special = m_special_ids.find(tokid) != m_special_ids.end();
            if (!special || encode_special_tokens) {
                out.push_back(tokid);
            }
            work = m.suffix();
        }
    }
    if (!work.empty()) {
        auto rest_decoded = bpemodel.encode(work);
        out.insert(out.end(), rest_decoded.begin(), rest_decoded.end());
    }
    return out;
}

std::string AdditionalVocabAdapter::decode(const std::vector<uint32_t>& tokens,
                                           BPE& bpemodel,
                                           bool decode_special_tokens,
                                           bool valid_utf8) {
    std::string out;
    std::vector<uint32_t> to_decode;
    for (auto tokid : tokens) {
        auto tokloc = m_id_to_token.find(tokid);
        if (tokloc != m_id_to_token.end()) {  // is an added token
            if (!to_decode.empty()) {
                out += bpemodel.decode(to_decode, valid_utf8);
                to_decode.clear();
            }
            bool special = m_special_ids.find(tokid) != m_special_ids.end();
            // only include non-special tokens unless decode_special_tokens
            if (!special || decode_special_tokens) {
                out += tokloc->second;
            }
        } else {
            // non-added, regular token.
            to_decode.push_back(tokid);
        }
    }
    if (!to_decode.empty()) {
        out += bpemodel.decode(to_decode, valid_utf8);
    }
    return out;
}
}  // namespace bpecpp
