#include <jni.h>
#include <string>
#include <stdio.h>
#include "gempyre.h"
#include "gempyre_utils.h"

static JavaVM* Androidjvm = nullptr;
static jobject AndroidActivity;

void Gempyre::setJNIENV(void* env, void* obj) {
    static_cast<JNIEnv*>(env)->GetJavaVM(&Androidjvm);
    AndroidActivity = static_cast<JNIEnv*>(env)->NewGlobalRef(static_cast<jobject>(obj));
}

JNIEXPORT jint JNI_OnLoad(JavaVM* /*vm*/, void* /*reserved*/) {
    return JNI_VERSION_1_6;
}
    
int androidLoadUi(const std::string& url) {
    if(nullptr == Androidjvm) {
        GempyreUtils::log(GempyreUtils::LogLevel::Fatal, "setJNIENV not called");
        return -98;
    }
    JNIEnv* env = nullptr;
    Androidjvm->AttachCurrentThread(&env, nullptr);
    if(!env) {
        GempyreUtils::log(GempyreUtils::LogLevel::Fatal, "Cannot AttachCurrentThread");
        return -97;
    }
    jstring urlString = env->NewStringUTF(url.c_str());
    jclass cls = env->GetObjectClass(AndroidActivity);
    jmethodID methodId = env->GetMethodID(cls, "onUiLoad", "(Ljava/lang/String;)I");
    if (methodId == 0) {
        GempyreUtils::log(GempyreUtils::LogLevel::Fatal, "onUiLoad not found");
        return -99;
    }
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "onUiLoad called", url);
    return env->CallIntMethod(AndroidActivity, methodId, urlString);
}
