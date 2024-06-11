//
// Created by Eugene on 6/10/2024.
//

#ifndef STANDALONETTS_ESPEAK_LIB_H
#define STANDALONETTS_ESPEAK_LIB_H

#include <stdio.h>
#include "android/log.h"
#include <map>
#include <vector>
#include <string>

#define ESPEAK_LOGE(...)                                            \
  do {                                                                   \
    fprintf(stderr, "%s:%s:%d ", __FILE__, __func__,                     \
            static_cast<int>(__LINE__));                                 \
    fprintf(stderr, ##__VA_ARGS__);                                      \
    fprintf(stderr, "\n");                                               \
    __android_log_print(ANDROID_LOG_WARN, "sherpa-onnx", ##__VA_ARGS__); \
  } while (0)

typedef char32_t Phoneme;
typedef std::map<Phoneme, std::vector<Phoneme>> PhonemeMap;

struct eSpeakPhonemeConfig {
    std::string voice = "en-us";

    Phoneme period = U'.';      // CLAUSE_PERIOD
    Phoneme comma = U',';       // CLAUSE_COMMA
    Phoneme question = U'?';    // CLAUSE_QUESTION
    Phoneme exclamation = U'!'; // CLAUSE_EXCLAMATION
    Phoneme colon = U':';       // CLAUSE_COLON
    Phoneme semicolon = U';';   // CLAUSE_SEMICOLON
    Phoneme space = U' ';

    // Remove language switch flags like "(en)"
    bool keepLanguageFlags = false;

    std::shared_ptr<PhonemeMap> phonemeMap;
};

#define CLAUSE_INTONATION_FULL_STOP 0x00000000
#define CLAUSE_INTONATION_COMMA 0x00001000
#define CLAUSE_INTONATION_QUESTION 0x00002000
#define CLAUSE_INTONATION_EXCLAMATION 0x00003000

#define CLAUSE_TYPE_CLAUSE 0x00040000
#define CLAUSE_TYPE_SENTENCE 0x00080000

#define CLAUSE_PERIOD (40 | CLAUSE_INTONATION_FULL_STOP | CLAUSE_TYPE_SENTENCE)
#define CLAUSE_COMMA (20 | CLAUSE_INTONATION_COMMA | CLAUSE_TYPE_CLAUSE)
#define CLAUSE_QUESTION (40 | CLAUSE_INTONATION_QUESTION | CLAUSE_TYPE_SENTENCE)
#define CLAUSE_EXCLAMATION                                                     \
  (45 | CLAUSE_INTONATION_EXCLAMATION | CLAUSE_TYPE_SENTENCE)
#define CLAUSE_COLON (30 | CLAUSE_INTONATION_FULL_STOP | CLAUSE_TYPE_CLAUSE)
#define CLAUSE_SEMICOLON (30 | CLAUSE_INTONATION_COMMA | CLAUSE_TYPE_CLAUSE)

typedef char32_t Phoneme;
typedef std::map<Phoneme, std::vector<Phoneme>> PhonemeMap;

void InitEspeak(const std::string &data_dir);
static std::unordered_map<char32_t, int32_t> ReadTokens(std::istream &is);

// Phonemizes text using espeak-ng.
// Returns phonemes for each sentence as a separate std::vector.
//
// Assumes espeak_Initialize has already been called.
void phonemize_eSpeak(std::string text, eSpeakPhonemeConfig &config,
                 std::vector<std::vector<Phoneme>> &phonemes);

enum TextCasing {
    CASING_IGNORE = 0,
    CASING_LOWER = 1,
    CASING_UPPER = 2,
    CASING_FOLD = 3
};

// Configuration for phonemize_codepoints
struct CodepointsPhonemeConfig {
    TextCasing casing = CASING_FOLD;
    std::shared_ptr<PhonemeMap> phonemeMap;
};

// "Phonemizes" text as a series of normalized UTF-8 codepoints.
// Returns a single std::vector of "phonemes".
//
// Does not detect sentence boundaries.
void phonemize_codepoints(std::string text, CodepointsPhonemeConfig &config,
                     std::vector<std::vector<Phoneme>> &phonemes);



#endif //STANDALONETTS_ESPEAK_LIB_H
