//
// Created by Eugene on 6/10/2024.
//
#include <android/log.h>
#include <jni.h>

#include <cinttypes>
#include <cstring>
#include <string>
#include "espeak_lib.h"

#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, "espeak-jni", __VA_ARGS__))

extern "C" JNIEXPORT void JNICALL
Java_com_StandaloneTTS_OfflineTts_initEspeak(JNIEnv *env, jobject thiz,
                                                     jstring tokenPath, jstring dataDir)
{
    const char *p_tokenPath = env->GetStringUTFChars(tokenPath, nullptr);
    const char *p_dataDir = env->GetStringUTFChars(dataDir, nullptr);

    LOGI("initEspeakLib tokenPath is: %s, dataDir is: %s", p_tokenPath, p_dataDir);
    initEspeakLib(std::string(p_tokenPath), std::string(p_dataDir));

    env->ReleaseStringUTFChars( dataDir, p_dataDir);
    env->ReleaseStringUTFChars(tokenPath, p_tokenPath);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_StandaloneTTS_OfflineTts_convertTextToTokenIds(JNIEnv *env, jobject thiz, jstring text,
                                                        jstring voice)
{

    const char *p_text = env->GetStringUTFChars(text, nullptr);
    const char *p_voice = env->GetStringUTFChars(voice, nullptr);
    LOGI("string is: %s, voice is: %s", p_text, p_voice);
    std::vector<std::vector<int64_t>> textTokens = convertTextToTokenIds(std::string(p_text), std::string(p_voice));
    LOGI("past tokenization");

    jclass arrayListClass = env->FindClass("java/util/ArrayList");
    LOGI("past arrayListClass");
    jmethodID arrayListConstructor = env->GetMethodID(arrayListClass, "<init>", "()V");
    LOGI("past arrayListConstructor");
    jmethodID addMethod = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");
    LOGI("past addMethod");

    // The list we're going to return:
    jobject list = env->NewObject(arrayListClass, arrayListConstructor);
    LOGI("past list");

    for(auto& tokenArray : textTokens) {
        LOGI("adding tokenArray size: %lu", tokenArray.size());
        jlongArray longArray = env->NewLongArray(tokenArray.size());
        LOGI("past longArray");
        env->SetLongArrayRegion(longArray, 0, tokenArray.size(), tokenArray.data());
        LOGI("past SetLongArrayRegion");
        // Add it to the list
        env->CallBooleanMethod(list, addMethod, longArray);
        LOGI("past CallBooleanMethod");
    }
    return list;
}
