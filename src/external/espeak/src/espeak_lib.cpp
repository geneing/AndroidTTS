//
// Created by Eugene on 6/10/2024.
//

#include "espeak_lib.h"
#include <string>
#include <mutex>

#include "espeak-ng/speak_lib.h"

void InitEspeak(const std::string &data_dir) {
    static std::once_flag init_flag;
    std::call_once(init_flag, [data_dir]() {
        int32_t result =
                espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 0, data_dir.c_str(), 0);
        if (result != 22050) {
            exit(-1);
        }
    });
}

