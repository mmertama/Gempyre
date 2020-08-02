#include <jni.h>
#include <string>

JNIEnv* Androidenv;
jobject Androidobj;


extern int main(int, char**);

JNIEXPORT jint JNICALL Java_MainActivity_CallMain(JNIEnv* env, jobject obj) {
    Androidenv = env;
    Androidobj = obj;
    return main(0, nullptr);
}
    
int androidStart(const std::string& url) {
    jstring urlString = Androidenv->NewStringUTF(url.c_str());
    jclass cls = Androidenv->GetObjectClass(Androidobj);
    jmethodID methodId = Androidenv->GetMethodID(cls, "onLoad", "([Ljava/lang/String;)I");
    if (methodId == 0) {
        return -99;
    }
    return Androidenv->CallIntMethod(Androidobj, methodId, urlString);
}
