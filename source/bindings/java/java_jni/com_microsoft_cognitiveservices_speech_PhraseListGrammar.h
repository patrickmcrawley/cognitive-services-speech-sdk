/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_microsoft_cognitiveservices_speech_PhraseListGrammar */

#ifndef _Included_com_microsoft_cognitiveservices_speech_PhraseListGrammar
#define _Included_com_microsoft_cognitiveservices_speech_PhraseListGrammar
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_microsoft_cognitiveservices_speech_PhraseListGrammar
 * Method:    fromRecognizer
 * Signature: (Lcom/microsoft/cognitiveservices/speech/util/IntRef;Lcom/microsoft/cognitiveservices/speech/util/SafeHandle;)J
 */
JNIEXPORT jlong JNICALL Java_com_microsoft_cognitiveservices_speech_PhraseListGrammar_fromRecognizer
  (JNIEnv *, jclass, jobject, jobject);

/*
 * Class:     com_microsoft_cognitiveservices_speech_PhraseListGrammar
 * Method:    clear
 * Signature: (Lcom/microsoft/cognitiveservices/speech/util/SafeHandle;)J
 */
JNIEXPORT jlong JNICALL Java_com_microsoft_cognitiveservices_speech_PhraseListGrammar_clear
  (JNIEnv *, jobject, jobject);

/*
 * Class:     com_microsoft_cognitiveservices_speech_PhraseListGrammar
 * Method:    addPhrase
 * Signature: (Lcom/microsoft/cognitiveservices/speech/util/SafeHandle;Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_com_microsoft_cognitiveservices_speech_PhraseListGrammar_addPhrase
  (JNIEnv *, jobject, jobject, jstring);

#ifdef __cplusplus
}
#endif
#endif