#include "Mod/Engine/ZipImport.h"

#include "Mod/CmdLine.h"
#include "PvzpLib/PvzpDebug.h"

#if defined(__ANDROID__) && !defined(__TERMUX__)
#include <SDL3/SDL.h>
#include <jni.h>
#define PVZU_ANDROID_IMPORT 1
#endif

namespace pvzu::engine {

namespace {
ImportMailbox gMailbox;

std::optional<ImportResult> DevResult() {
	auto aValue = CmdOption("-pvzu-dev-import");
	return aValue ? DevImportResult(*aValue) : std::nullopt;
}

#ifdef PVZU_ANDROID_IMPORT
bool OpenAndroidPicker() {
	JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
	jobject activity = static_cast<jobject>(SDL_GetAndroidActivity());
	if (!env || !activity)
		return false;
	bool ok = false;
	jclass cls = env->GetObjectClass(activity);
	jmethodID open = cls ? env->GetMethodID(cls, "pvzuOpenUnnamedImport", "()Z") : nullptr;
	if (open)
		ok = env->CallBooleanMethod(activity, open);
	if (env->ExceptionCheck()) {
		env->ExceptionClear();
		ok = false;
	}
	if (cls)
		env->DeleteLocalRef(cls);
	env->DeleteLocalRef(activity);
	return ok;
}
#endif
}

bool ImportButtonWanted(bool theFromGameSelector) {
#ifdef PVZU_ANDROID_IMPORT
	constexpr bool kAndroid = true;
#else
	constexpr bool kAndroid = false;
#endif
	return ImportButtonShown(kAndroid || DevResult().has_value(), theFromGameSelector);
}

bool StartUnnamedZipImport() {
	if (auto aDev = DevResult()) {
		gMailbox.Post(*aDev);
		return true;
	}
#ifdef PVZU_ANDROID_IMPORT
	const bool ok = OpenAndroidPicker();
	if (!ok)
		PvzpLogLn("PVZU: could not open the Unnamed zip picker");
	return ok;
#else
	return false;
#endif
}

std::optional<ImportResult> TakeUnnamedImportResult() { return gMailbox.Take(); }

} // namespace pvzu::engine

#ifdef PVZU_ANDROID_IMPORT
// PvZPortableActivity.nativePvzuImportDone(int outcome, int copied, String error), on Android's UI thread.
extern "C" JNIEXPORT void JNICALL Java_io_github_wszqkzqk_pvzportable_PvZPortableActivity_nativePvzuImportDone(
	JNIEnv* env, jclass, jint theOutcome, jint theCopied, jstring theError) {
	pvzu::ImportResult r;
	r.outcome = pvzu::ImportOutcomeFromCode(theOutcome);
	r.copied = theCopied;
	if (theError) {
		if (const char* s = env->GetStringUTFChars(theError, nullptr)) {
			r.error = s;
			env->ReleaseStringUTFChars(theError, s);
		}
	}
	pvzu::engine::gMailbox.Post(r);
}
#endif
