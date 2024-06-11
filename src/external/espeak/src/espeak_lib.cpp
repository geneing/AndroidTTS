//
// Created by Eugene on 6/10/2024.
//
// Based on
// Copyright (c)  2022-2023  Xiaomi Corporation

#include "espeak_lib.h"
#include <codecvt>
#include <fstream>
#include <map>
#include <mutex>  // NOLINT
#include <locale>
#include <string>
#include <sstream>
#include <utility>
#include <vector>


#include "espeak-ng/speak_lib.h"
#include "uni_algo.h"

static std::unordered_map<char32_t, int32_t> token2id_;

void init_espeak_lib(
        const std::string &tokens, const std::string &data_dir) {
    {
        std::ifstream is(tokens);
        token2id_ = ReadTokens(is);
    }
    InitEspeak(data_dir);
}

void InitEspeak(const std::string &data_dir) {
    static std::once_flag init_flag;
    std::call_once(init_flag, [data_dir]() {
        int32_t result =
                espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 0, data_dir.c_str(), 0);
        if (result != 22050) {
            ESPEAK_LOGE(
                    "Failed to initialize espeak-ng with data dir: %s. Return code is: "
                    "%d",
                    data_dir.c_str(), result);
            exit(-1);
        }
    });
}


static std::unordered_map<char32_t, int32_t> ReadTokens(std::istream &is) {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
    std::unordered_map<char32_t, int32_t> token2id;

    std::string line;

    std::string sym;
    std::u32string s;
    int32_t id;
    while (std::getline(is, line)) {
        std::istringstream iss(line);
        iss >> sym;
        if (iss.eof()) {
            id = atoi(sym.c_str());
            sym = " ";
        } else {
            iss >> id;
        }

        // eat the trailing \r\n on windows
        iss >> std::ws;
        if (!iss.eof()) {
            ESPEAK_LOGE("Error when reading tokens: %s", line.c_str());
            exit(-1);
        }

        s = conv.from_bytes(sym);
        if (s.size() != 1) {
            // for tokens.txt from coqui-ai/TTS, the last token is <BLNK>
            if (s.size() == 6 && s[0] == '<' && s[1] == 'B' && s[2] == 'L' &&
                s[3] == 'N' && s[4] == 'K' && s[5] == '>') {
                continue;
            }

            ESPEAK_LOGE("Error when reading tokens at Line %s. size: %d",
                        line.c_str(), static_cast<int32_t>(s.size()));
            exit(-1);
        }

        char32_t c = s[0];

        if (token2id.count(c)) {
            ESPEAK_LOGE("Duplicated token %s. Line %s. Existing ID: %d",
                        sym.c_str(), line.c_str(), token2id.at(c));
            exit(-1);
        }

        token2id.insert({c, id});
    }
    return token2id;
}

// see the function "phonemes_to_ids" from
// https://github.com/rhasspy/piper/blob/master/notebooks/piper_inference_(ONNX).ipynb
static std::vector<int64_t> PiperPhonemesToIds(
        const std::unordered_map<char32_t, int32_t> &token2id,
        const std::vector<Phoneme> &phonemes) {
    // see
    // https://github.com/rhasspy/piper-phonemize/blob/master/src/phoneme_ids.hpp#L17
    int32_t pad = token2id.at(U'_');
    int32_t bos = token2id.at(U'^');
    int32_t eos = token2id.at(U'$');

    std::vector<int64_t> ans;
    ans.reserve(phonemes.size());

    ans.push_back(bos);
    for (auto p : phonemes) {
        if (token2id.count(p)) {
            ans.push_back(token2id.at(p));
            ans.push_back(pad);
        } else {
            ESPEAK_LOGE("Skip unknown phonemes. Unicode codepoint: \\U+%04x.",
                             static_cast<uint32_t>(p));
        }
    }
    ans.push_back(eos);

    return ans;
}

std::vector<std::vector<int64_t>> ConvertTextToTokenIds(
        const std::string &text, const std::string &voice /*= ""*/) {
    eSpeakPhonemeConfig config;

    // ./bin/espeak-ng-bin --path  ./install/share/espeak-ng-data/ --voices
    // to list available voices
    config.voice = voice;  // e.g., voice is en-us

    std::vector<std::vector<Phoneme>> phonemes;

    static std::mutex espeak_mutex;
    {
        std::lock_guard<std::mutex> lock(espeak_mutex);

        // keep multi threads from calling into phonemize_eSpeak
        phonemize_eSpeak(text, config, phonemes);
    }

    std::vector<std::vector<int64_t>> ans;
    std::vector<int64_t> phoneme_ids;

    for (const auto &p : phonemes) {
        phoneme_ids = PiperPhonemesToIds(token2id_, p);
        ans.push_back(std::move(phoneme_ids));
    }

    return ans;
}

// language -> phoneme -> [phoneme, ...]
std::map<std::string, PhonemeMap> DEFAULT_PHONEME_MAP = {
        {"pt-br", {{U'c', {U'k'}}}}};

void phonemize_eSpeak(std::string text, eSpeakPhonemeConfig &config,
                 std::vector<std::vector<Phoneme>> &phonemes) {

    auto voice = config.voice;
    int result = espeak_SetVoiceByName(voice.c_str());
    if (result != 0) {
        throw std::runtime_error("Failed to set eSpeak-ng voice");
    }

    std::shared_ptr<PhonemeMap> phonemeMap;
    if (config.phonemeMap) {
        phonemeMap = config.phonemeMap;
    } else if (DEFAULT_PHONEME_MAP.count(voice) > 0) {
        phonemeMap = std::make_shared<PhonemeMap>(DEFAULT_PHONEME_MAP[voice]);
    }

    // Modified by eSpeak
    std::string textCopy(text);

    std::vector<Phoneme> *sentencePhonemes = nullptr;
    const char *inputTextPointer = textCopy.c_str();
    int terminator = 0;

    while (inputTextPointer != NULL) {
        // Modified espeak-ng API to get access to clause terminator
        std::string clausePhonemes(espeak_TextToPhonemesWithTerminator(
                (const void **)&inputTextPointer,
                /*textmode*/ espeakCHARS_AUTO,
                /*phonememode = IPA*/ 0x02, &terminator));

        // Decompose, e.g. "ç" -> "c" + "̧"
        auto phonemesNorm = una::norm::to_nfd_utf8(clausePhonemes);
        auto phonemesRange = una::ranges::utf8_view{phonemesNorm};

        if (!sentencePhonemes) {
            // Start new sentence
            phonemes.emplace_back();
            sentencePhonemes = &phonemes[phonemes.size() - 1];
        }

        // Maybe use phoneme map
        std::vector<Phoneme> mappedSentPhonemes;
        if (phonemeMap) {
            for (auto phoneme : phonemesRange) {
                if (phonemeMap->count(phoneme) < 1) {
                    // No mapping for phoneme
                    mappedSentPhonemes.push_back(phoneme);
                } else {
                    // Mapping for phoneme
                    auto mappedPhonemes = &(phonemeMap->at(phoneme));
                    mappedSentPhonemes.insert(mappedSentPhonemes.end(),
                                              mappedPhonemes->begin(),
                                              mappedPhonemes->end());
                }
            }
        } else {
            // No phoneme map
            mappedSentPhonemes.insert(mappedSentPhonemes.end(), phonemesRange.begin(),
                                      phonemesRange.end());
        }

        auto phonemeIter = mappedSentPhonemes.begin();
        auto phonemeEnd = mappedSentPhonemes.end();

        if (config.keepLanguageFlags) {
            // No phoneme filter
            sentencePhonemes->insert(sentencePhonemes->end(), phonemeIter,
                                     phonemeEnd);
        } else {
            // Filter out (lang) switch (flags).
            // These surround words from languages other than the current voice.
            bool inLanguageFlag = false;

            while (phonemeIter != phonemeEnd) {
                if (inLanguageFlag) {
                    if (*phonemeIter == U')') {
                        // End of (lang) switch
                        inLanguageFlag = false;
                    }
                } else if (*phonemeIter == U'(') {
                    // Start of (lang) switch
                    inLanguageFlag = true;
                } else {
                    sentencePhonemes->push_back(*phonemeIter);
                }

                phonemeIter++;
            }
        }

        // Add appropriate punctuation depending on terminator type
        int punctuation = terminator & 0x000FFFFF;
        if (punctuation == CLAUSE_PERIOD) {
            sentencePhonemes->push_back(config.period);
        } else if (punctuation == CLAUSE_QUESTION) {
            sentencePhonemes->push_back(config.question);
        } else if (punctuation == CLAUSE_EXCLAMATION) {
            sentencePhonemes->push_back(config.exclamation);
        } else if (punctuation == CLAUSE_COMMA) {
            sentencePhonemes->push_back(config.comma);
            sentencePhonemes->push_back(config.space);
        } else if (punctuation == CLAUSE_COLON) {
            sentencePhonemes->push_back(config.colon);
            sentencePhonemes->push_back(config.space);
        } else if (punctuation == CLAUSE_SEMICOLON) {
            sentencePhonemes->push_back(config.semicolon);
            sentencePhonemes->push_back(config.space);
        }

        if ((terminator & CLAUSE_TYPE_SENTENCE) == CLAUSE_TYPE_SENTENCE) {
            // End of sentence
            sentencePhonemes = nullptr;
        }

    } // while inputTextPointer != NULL

} /* phonemize_eSpeak */
