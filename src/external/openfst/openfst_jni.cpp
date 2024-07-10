#include <android/log.h>
#include <jni.h>

#include <cinttypes>
#include <cstring>
#include <string>
#include "openfst_api.h"

#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, "openfst-jni", __VA_ARGS__))


extern "C"
JNIEXPORT jlong JNICALL
Java_com_StandaloneTTS_OfflineTts_00024Normalizer_initNormalizer(JNIEnv *env, jobject thiz,
                                                                 jstring far_list) {
    const char * p_far_list = env->GetStringUTFChars(far_list, nullptr);
    auto *pNormalizer = new Normalizer(std::string(p_far_list));
    env->ReleaseStringUTFChars( far_list, p_far_list);
    return (jlong) pNormalizer;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_StandaloneTTS_OfflineTts_00024Normalizer_normalizeImpl(JNIEnv *env, jobject thiz,
                                                                jlong ptr, jstring text) {
    const char* pText = env->GetStringUTFChars( text, nullptr );
    jclass c = env->GetObjectClass(thiz);
    jfieldID fid_handle = env->GetFieldID(c, "ptr", "J");
    auto *pNormalizer = (Normalizer*) env->GetLongField(thiz, fid_handle);
    std::string normalizedText = pNormalizer->apply(std::string(pText));
    jstring result = env->NewStringUTF( normalizedText.c_str() );

    env->ReleaseStringUTFChars( text, pText );
    return result;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_StandaloneTTS_OfflineTts_00024Normalizer_cleanupNormalizer(JNIEnv *env, jobject thiz,
                                                                    jlong ptr) {
    jclass c = env->GetObjectClass(thiz);
    jfieldID fid_handle = env->GetFieldID(c, "ptr", "J");
    auto *pNormalizer = (Normalizer*) env->GetLongField(thiz, fid_handle);
    delete pNormalizer;
}