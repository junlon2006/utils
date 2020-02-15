#include "LocalAsrEngine.h"
#include "jni.h"
#include "ual_ofa.h"
#include "ofa_consts.h"
#include "uni_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <gnu/lib-names.h>
#include <string.h>
#include <pthread.h>

#define TAG           "Engine"
#define MAXPATH       (512)
#define min(a, b)     (a > b ? b : a)
#define LASR_THRESOLD (-10.0)

/* we use dlopen load shared library libengine_c.so */
typedef int (*fpUalOFAGetEngineType)(void);
typedef const char* (*fpUalOFAGetVersion)(void);
typedef int (*fpUalOFAInitialize)(const char* path_of_am, const char* path_of_grammar);
typedef int (*fpUalOFASetOptionInt)(int id, int value);
typedef int (*fpUalOFARecognize)(signed char* raw_audio, int len);
typedef int (*fpUalOFAStart)(const char* grammar_tag, int am_index);
typedef int (*fpUalOFAStop)(void);
typedef char* (*fpUalOFAGetResult)(void);
typedef void (*fpUalOFARelease)(void);

typedef struct {
  fpUalOFAGetEngineType UalOFAGetEngineType;
  fpUalOFAGetVersion    UalOFAGetVersion;
  fpUalOFAInitialize    UalOFAInitialize;
  fpUalOFASetOptionInt  UalOFASetOptionInt;
  fpUalOFARecognize     UalOFARecognize;
  fpUalOFAStart         UalOFAStart;
  fpUalOFAStop          UalOFAStop;
  fpUalOFAGetResult     UalOFAGetResult;
  fpUalOFARelease       UalOFARelease;
} AsrEngineSymbolTable;

static AsrEngineSymbolTable *g_engine_symbol_table = NULL;
static pthread_mutex_t      g_mutex = PTHREAD_MUTEX_INITIALIZER;
static int                  g_is_engine_init = false;

static void _load_engine_symbols(const char *current_path) {
  void *handle;
  char library_name[2048];
  snprintf(library_name, sizeof(library_name), "%s/%s", current_path, "lib/libengine_c.so");
  LOGT(TAG, "library=%s", library_name);

  if (NULL == (handle = dlopen(library_name, RTLD_LAZY))) {
    LOGE(TAG, "dlopen failed");
    return;
  }

  g_engine_symbol_table = (AsrEngineSymbolTable *)malloc(sizeof(AsrEngineSymbolTable));
  if (NULL == g_engine_symbol_table) {
    LOGE(TAG, "alloc memory failed");
    return;
  }

  g_engine_symbol_table->UalOFAGetEngineType = (fpUalOFAGetEngineType)dlsym(handle, "UalOFAGetEngineType");
  g_engine_symbol_table->UalOFAGetVersion    = (fpUalOFAGetVersion)dlsym(handle, "UalOFAGetVersion");
  g_engine_symbol_table->UalOFAInitialize    = (fpUalOFAInitialize)dlsym(handle, "UalOFAInitialize");
  g_engine_symbol_table->UalOFASetOptionInt  = (fpUalOFASetOptionInt)dlsym(handle, "UalOFASetOptionInt");
  g_engine_symbol_table->UalOFARecognize     = (fpUalOFARecognize)dlsym(handle, "UalOFARecognize");
  g_engine_symbol_table->UalOFAStart         = (fpUalOFAStart)dlsym(handle, "UalOFAStart");
  g_engine_symbol_table->UalOFAStop          = (fpUalOFAStop)dlsym(handle, "UalOFAStop");
  g_engine_symbol_table->UalOFAGetResult     = (fpUalOFAGetResult)dlsym(handle, "UalOFAGetResult");
  g_engine_symbol_table->UalOFARelease       = (fpUalOFARelease)dlsym(handle, "UalOFARelease");
}

static void _engine_init() {
  int status;
  char curr_path[MAXPATH];
  char am_path[MAXPATH];
  char grammar[MAXPATH];

  pthread_mutex_lock(&g_mutex);

  if (g_is_engine_init) {
    LOGT(TAG, "release engine");
    g_engine_symbol_table->UalOFARelease();
  }

  getcwd(curr_path, MAXPATH);
  LOGT(TAG, "current dir=%s", curr_path);

  _load_engine_symbols(curr_path);

  int engineType = g_engine_symbol_table->UalOFAGetEngineType();
  if (KWS_STD_ENGINE != engineType) {
    LOGE(TAG, "engine type=%d", engineType);
  }

  LOGT(TAG, "version=%s", g_engine_symbol_table->UalOFAGetVersion());
  g_engine_symbol_table->UalOFASetOptionInt(ASR_LOG_ID, 0);

  snprintf(am_path, sizeof(am_path), "%s/%s", curr_path, "models");
  snprintf(grammar, sizeof(grammar), "%s/%s", curr_path, "grammar/ivm_cmd.jsgf.dat");
  LOGT(TAG, "am=%s, grammar=%s", am_path, grammar);

  g_engine_symbol_table->UalOFAInitialize(am_path, grammar);
  status = g_engine_symbol_table->UalOFASetOptionInt(ASR_ENGINE_SET_TYPE_ID, KWS_STD_ENGINE);
  if (status == ASR_FATAL_ERROR) {
    LOGE(TAG, "init failed, rc=%d", status);
  }

  g_engine_symbol_table->UalOFASetOptionInt(ASR_SET_BEAM_ID, 1500);
  g_engine_symbol_table->UalOFASetOptionInt(ASR_ENGINE_SWITCH_LANGUAGE, MANDARIN);

  g_is_engine_init = true;
  LOGT(TAG, "engine init success");

  pthread_mutex_unlock(&g_mutex);
}

JNIEXPORT jint JNICALL Java_com_unisound_aios_audiocheck_JNI_LocalAsrEngine_AsrEngineInit
(JNIEnv * env, jobject obj, jstring am, jstring grammar) {
  LOGT(TAG, "engine init");

  _engine_init();

  fflush(stdout);
  return 0;
}

static void _lasr_result_parse(char *content, char *command, float *score) {
  int i = 0;
  char *p = content;
  char c = 0;
  /* lasr result string format: "%d <s> %s </s> \n%f\n" */
  /* skip <s> */
  p = strstr(content, "<s>");
  if (NULL == p) {
    LOGE(TAG, "content: %s", command);
    return;
  }
  p += 3;   // skip <s>
  /* get context with not space */
  while ((c = *p++) != '<') {       // p point to /
    if ((c != ' ') && (c != '\t')) {
      command[i++] = c;
    }
  }
  command[i] = '\0';
  /* get score */
  p += 3;   // skip </s>
  while ((' ' == *p) || ('\n' == *p) || ('\t' == *p)) {
    p++;
  }
  if ('\0' == *p) {
    LOGE(TAG, "content: %s", command);
    return;
  }
  *score = strtof(p, NULL);
  LOGT(TAG, "command=%s, score=%f", command, *score);
}

static jboolean _get_asr_result(const char *cmd_word) {
  char command[128];
  float score = -100;
  char *res = g_engine_symbol_table->UalOFAGetResult();
  if (NULL != strstr(res, "#NULL")) {
    return false;
  }

  LOGT("ARS_RESULT=%s", res);
  _lasr_result_parse(res, command, &score);

  if (0 != strcmp(command, cmd_word)) {
    return false;
  }

  return (score > LASR_THRESOLD);
}

static jboolean _recognize(signed char *raw_data, int len,
                           const char *cmd_word, const char *grammar) {
  char domain[16] = "ivm";
  int fix_one_recongize_len = 160;
  int i = 0;
  int status;
  int index = 0;
  jboolean invalid = false;

  pthread_mutex_lock(&g_mutex);

  status = g_engine_symbol_table->UalOFAStart(domain, 54);
  if (status != ASR_RECOGNIZER_OK) {
    LOGT(TAG, "start failed, rc=%d", status);
  }

  while (len > 0) {
    status = g_engine_symbol_table->UalOFARecognize(raw_data + (fix_one_recongize_len * i++),
                                                    min(fix_one_recongize_len, len));
    if (status == ASR_RECOGNIZER_PARTIAL_RESULT) {
      invalid = _get_asr_result(cmd_word);
    }

    len -= fix_one_recongize_len;
  }

  status = g_engine_symbol_table->UalOFAStop();
  if (status >= 0 && !invalid) {
    invalid = _get_asr_result(cmd_word);
  }

  pthread_mutex_unlock(&g_mutex);

  return invalid;
}

JNIEXPORT jboolean JNICALL Java_com_unisound_aios_audiocheck_JNI_LocalAsrEngine_AsrEngineCheck
(JNIEnv *env, jobject obj, jbyteArray audio, jstring grammar, jstring cmd_word) {
  const char *grammar_name = env->GetStringUTFChars(grammar, 0);
  LOGT(TAG, "grammar=%s", grammar_name);

  const char *cmd = env->GetStringUTFChars(cmd_word, 0);
  LOGT(TAG, "cmd=%s", cmd);

  int wav_len = env->GetArrayLength(audio);
  signed char *wav = (signed char *)malloc(wav_len);

  env->GetByteArrayRegion(audio, 0, wav_len, reinterpret_cast<jbyte*>(wav));
  LOGT(TAG, "wav_len=%d", wav_len);
  jboolean result = _recognize(wav + 44, wav_len - 44, cmd, grammar_name);

  free(wav);

  env->ReleaseStringUTFChars(grammar, grammar_name);
  env->ReleaseStringUTFChars(cmd_word, cmd);
  fflush(stdout);

  return result;
}

