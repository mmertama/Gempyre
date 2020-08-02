#include <jni.h>
#include <string>

JNIEnv* Androidenv;
jobject Androidobj;


JNIEXPORT jint JNICALL Java_MainActivity_CallMain(JNIEnv* env, jobject obj) {
    Androidenv = env;
    Androidobj = obj;
    return main(0, nullptr);
}
    
int androidStart(const std::string& url) {
    jstring urlString = (*Androidenv)->NewStringUTF(Androidenv, url);
    jclass cls = (*Androidenv)->GetObjectClass(Androidenv, Androidobj);
    jmethodID methodId = (*Androidenv)->GetMethodID(Androidenv, cls, "onLoad", "([Ljava/lang/String;)I");
    if (methodId == 0) {
        return -99;
    }
    return (*Androidenv)->CallVoidMethod(Androidenv, Androidobj, methodId, urlString);
}   