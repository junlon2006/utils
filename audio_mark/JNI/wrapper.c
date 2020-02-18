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
#include <dirent.h>
#include <sys/stat.h>

#define TAG                 "Engine"
#define MAXPATH             (2048)
#define min(a, b)           (a > b ? b : a)
#define LASR_THRESHOLD       (-10.0)
#define WAV_HEADER_LEN      (44)
#define AM_IDX              (54)
#define GRAMMAR_ARRAY_SIZE  (1024 * 512)

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static jboolean        g_is_engine_inited = false;
static float           g_lasr_threshold = LASR_THRESHOLD;

static void _get_workspace(char *path, int len) {
  if (NULL == getcwd(path, len)) {
    LOGE(TAG, "get workspace failed");
    return;
  }

  LOGT(TAG, "workspace=%s", path);
}

static void _get_grammar_dir(char *grammar_dir, int len) {
  char workspace[MAXPATH];
  _get_workspace(workspace, sizeof(workspace));
  snprintf(grammar_dir, len, "%s/%s", workspace, "grammar");
}

static char* _get_grammar_arrays() {
  DIR *dir;
  struct dirent *ent;
  char grammar_dir[MAXPATH];
  char grammar_name[MAXPATH];
  char *grammar_arrays;
  int remain_len = GRAMMAR_ARRAY_SIZE;
  int i = 1;
  int err;
  int grammar_name_len;

  _get_grammar_dir(grammar_dir, sizeof(grammar_dir));
  LOGT(TAG, "grammar dir=%s", grammar_dir);

  grammar_arrays = (char *)malloc(GRAMMAR_ARRAY_SIZE);
  grammar_arrays[0] = '\0';

  if (NULL == (dir = opendir(grammar_dir))) {
    printf("open dir [%s] failed\n", grammar_dir);
    goto L_END;
  }

  while ((ent = readdir(dir)) != NULL) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
      continue;
    }

    snprintf(grammar_name, sizeof(grammar_name), "%s/%s;", grammar_dir, ent->d_name);
    LOGT(TAG, "grammar=%s", grammar_name);

    grammar_name_len = strlen(grammar_name);
    if (remain_len <= grammar_name_len) {
      grammar_arrays = (char *)realloc(grammar_arrays, GRAMMAR_ARRAY_SIZE * ++i);
      remain_len += GRAMMAR_ARRAY_SIZE;
      LOGT(TAG, "realloc grammar array, len=%d, remain_len=%d", GRAMMAR_ARRAY_SIZE * i, remain_len);
    }

    strcat(grammar_arrays, grammar_name);
    remain_len -= grammar_name_len;
  }

  LOGT(TAG, "grammar_arrays=%s, len=%d", grammar_arrays, strlen(grammar_arrays));
  closedir(dir);

L_END:
  return grammar_arrays;
}

static void _get_am_path(char *am_path, int len) {
  char workspace[MAXPATH];
  _get_workspace(workspace, sizeof(workspace));
  snprintf(am_path, len, "%s/%s", workspace, "models");
  LOGT(TAG, "am path=%s", am_path);
}

static int _engine_init() {
  int status;
  char am_path[MAXPATH];
  char *grammar_arrays = NULL;

  if (g_is_engine_inited) {
    LOGT(TAG, "release engine");
    UalOFARelease();
  }

  int engineType = UalOFAGetEngineType();
  if (KWS_STD_ENGINE != engineType) {
    LOGE(TAG, "engine type=%d", engineType);
    goto L_ERROR;
  }

  LOGT(TAG, "version=%s", UalOFAGetVersion());
  UalOFASetOptionInt(ASR_LOG_ID, 0);

  _get_am_path(am_path, sizeof(am_path));
  grammar_arrays = _get_grammar_arrays();
  LOGT(TAG, "am=%s, grammar=%s", am_path, grammar_arrays);

  status = UalOFAInitialize(am_path, grammar_arrays);
  free(grammar_arrays);
  if (0 != status) {
    LOGT(TAG, "init failed, rc=%d", status);
    goto L_ERROR;
  }

  status = UalOFASetOptionInt(ASR_ENGINE_SET_TYPE_ID, KWS_STD_ENGINE);
  if (status == ASR_FATAL_ERROR) {
    LOGE(TAG, "init failed, rc=%d", status);
    goto L_ERROR;
  }

  UalOFASetOptionInt(ASR_SET_BEAM_ID, 1500);
  UalOFASetOptionInt(ASR_ENGINE_SWITCH_LANGUAGE, MANDARIN);

  g_is_engine_inited = true;
  LOGT(TAG, "engine init success");
  return 0;

L_ERROR:
  g_is_engine_inited = false;
  LOGT(TAG, "engine init failed");
}

JNIEXPORT jint JNICALL Java_com_unisound_aios_audiocheck_JNI_LocalAsrEngine_AsrEngineInit
  (JNIEnv * env, jobject obj, jstring am, jstring grammar) {
  int ret;

  pthread_mutex_lock(&g_mutex);
  LOGT(TAG, "engine init");
  ret = _engine_init();
  pthread_mutex_unlock(&g_mutex);

  fflush(stdout);
  return ret;
}

static void _lasr_result_parse(char *content, char *command, float *score) {
  int i = 0;
  char *p = content;
  char c = 0;
  p = strstr(content, "<s>");
  if (NULL == p) {
    LOGE(TAG, "content: %s", command);
    return;
  }

  p += 3;
  while ((c = *p++) != '<') {
    if ((c != ' ') && (c != '\t')) {
      command[i++] = c;
    }
  }
  command[i] = '\0';

  p += 3;
  while ((' ' == *p) || ('\n' == *p) || ('\t' == *p)) p++;
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
  char *res = UalOFAGetResult();
  if (NULL != strstr(res, "#NULL")) {
    return false;
  }

  LOGT(TAG, "ARS_RESULT=%s", res);
  _lasr_result_parse(res, command, &score);

  if (0 != strcmp(command, cmd_word)) {
    return false;
  }

  return (score > g_lasr_threshold);
}

static jboolean _recognize(signed char *raw_data, int len,
                           const char *cmd_word, const char *grammar) {
  int fix_one_recongize_len = 160;
  int i = 0;
  int status;
  jboolean valid = false;

  if (!g_is_engine_inited) {
    return valid;
  }

  status = UalOFAStart(grammar, AM_IDX);
  if (status != ASR_RECOGNIZER_OK) {
    LOGE(TAG, "start failed, rc=%d", status);
    goto L_END;
  }

  while (len > 0) {
    status = UalOFARecognize(raw_data + fix_one_recongize_len * i++,
                             min(fix_one_recongize_len, len));
    if (status == ASR_RECOGNIZER_PARTIAL_RESULT) {
      valid = _get_asr_result(cmd_word);
      if (valid) break;
    }

    len -= fix_one_recongize_len;
  }

  status = UalOFAStop();
  if (!valid && status >= 0) {
    valid = _get_asr_result(cmd_word);
  }

L_END:
  return valid;
}

JNIEXPORT jboolean JNICALL Java_com_unisound_aios_audiocheck_JNI_LocalAsrEngine_AsrEngineCheck
  (JNIEnv *env, jobject obj, jbyteArray audio, jstring grammar, jstring cmd_word) {
  int wav_len = env->GetArrayLength(audio);
  LOGT(TAG, "wav_len=%d", wav_len);
  if (wav_len <= WAV_HEADER_LEN) {
    LOGW(TAG, "audio too short");
    return false;
  }

  signed char *wav = (signed char *)malloc(wav_len);
  env->GetByteArrayRegion(audio, 0, wav_len, reinterpret_cast<jbyte*>(wav));

  const char *grammar_name = env->GetStringUTFChars(grammar, 0);
  LOGT(TAG, "grammar=%s", grammar_name);

  const char *cmd = env->GetStringUTFChars(cmd_word, 0);
  LOGT(TAG, "cmd=%s", cmd);

  pthread_mutex_lock(&g_mutex);
  jboolean result = _recognize(wav + WAV_HEADER_LEN, wav_len - WAV_HEADER_LEN, cmd, grammar_name);
  pthread_mutex_unlock(&g_mutex);

  free(wav);

  env->ReleaseStringUTFChars(grammar, grammar_name);
  env->ReleaseStringUTFChars(cmd_word, cmd);

  fflush(stdout);
  return result;
}

static void _reinit_engine_when_update_grammar() {
  LOGT(TAG, "restart engine start");
  _engine_init();
  LOGT(TAG, "restart engine end");
}

static void _grammar_file_process(const char *name) {
  char workspace[MAXPATH];
  char file_name[MAXPATH];
  char out_name[MAXPATH];

  _get_workspace(workspace, sizeof(workspace));

  snprintf(file_name, sizeof(file_name), "%s/grammar/%s", workspace, name);
  snprintf(out_name, sizeof(file_name), "%s/result/grammar.dat", workspace);
  LOGT(TAG, "out=%s, new=%s", out_name, file_name);

  if (0 != rename(out_name, file_name)) {
    LOGE(TAG, "build grammar[%s] failed", file_name);
    return;
  }

  _reinit_engine_when_update_grammar();

  LOGT(TAG, "build grammar[%s] success", file_name);
}

static void _grammar_out_path(char *out, int len) {
  char workspace[MAXPATH];
  _get_workspace(workspace, sizeof(workspace));
  snprintf(out, len, "%s/result", workspace);
  LOGT(TAG, "grammar out dir=%s", out);
}

JNIEXPORT jint JNICALL Java_com_unisound_aios_audiocheck_JNI_LocalAsrEngine_AsrGrammarBuild
  (JNIEnv *env, jobject obj, jstring jsgf, jstring jsgf_name) {
  int ret = 0, err;
  char am_path[MAXPATH];
  char grammar_out[MAXPATH];

  const char *jsgf_content = env->GetStringUTFChars(jsgf, 0);
  const char *jsgf_dat_name = env->GetStringUTFChars(jsgf_name, 0);

  LOGT(TAG, "JSGF:%s", jsgf_content);
  LOGT(TAG, "JSGF NAME=%s", jsgf_dat_name);

  _get_am_path(am_path, sizeof(am_path));
  _grammar_out_path(grammar_out, sizeof(grammar_out));

  pthread_mutex_lock(&g_mutex);

  HANDLE compiler = UalOFAInitializeUserDataCompiler(am_path);
  if (NULL == compiler) {
    ret = -1;
    LOGE(TAG, "grammar compiler init failed");
    goto L_END;
  }

  UalOFAGrammarCompilerSetOptionInt(compiler, ASR_ENGINE_SWITCH_LANGUAGE, MANDARIN);

  err = UalOFACompileGrammar(compiler, jsgf_content, "", grammar_out);
  if (err != 0) {
    ret = -1;
    LOGE(TAG, "grammar compiler failed, rc=%d", err);
    goto L_END;
  }

  _grammar_file_process(jsgf_dat_name);

  UalOFAReleaseUserDataCompiler(compiler);

L_END:
  env->ReleaseStringUTFChars(jsgf, jsgf_content);
  env->ReleaseStringUTFChars(jsgf_name, jsgf_dat_name);

  pthread_mutex_unlock(&g_mutex);

  fflush(stdout);
  return ret;
}

static jboolean _remove_local_useless_grammar_data(const char *remove_list) {
  char grammar_name[MAXPATH];
  int index = 0;
  char c;
  jboolean grammar_changed = false;
  while (c = *remove_list++) {
    if (c == ';') {
      grammar_name[index] = '\0';
      if (index != 0) {
        if (0 == remove(grammar_name)) {
          LOGT(TAG, "remove grammar=%s success", grammar_name);
          grammar_changed = true;
        } else {
          LOGE(TAG, "remove grammar=%s failed", grammar_name);
        }
      }
      index = 0;
      continue;
    }

    grammar_name[index++] = c;
  }

  return grammar_changed;
}

JNIEXPORT jint JNICALL Java_com_unisound_aios_audiocheck_JNI_LocalAsrEngine_AsrGrammarRefresh
  (JNIEnv * env, jobject obj, jstring remove) {
  const char *remove_grammar_list = env->GetStringUTFChars(remove, 0);

  pthread_mutex_lock(&g_mutex);

  LOGT(TAG, "remove list=%s", remove_grammar_list);
  jboolean grammar_changed = _remove_local_useless_grammar_data(remove_grammar_list);
  if (grammar_changed) {
    _reinit_engine_when_update_grammar();
  }

  pthread_mutex_unlock(&g_mutex);

  env->ReleaseStringUTFChars(remove, remove_grammar_list);

  fflush(stdout);
  return 0;
}

JNIEXPORT void JNICALL Java_com_unisound_aios_audiocheck_JNI_LocalAsrEngine_AsrEngineRecognizeThreshold
  (JNIEnv *env, jobject obj, jfloat thresold) {
  pthread_mutex_lock(&g_mutex);
  g_lasr_threshold = thresold;
  LOGT(TAG, "set lasr threshold=%f", g_lasr_threshold);
  pthread_mutex_unlock(&g_mutex);

  fflush(stdout);
}

JNIEXPORT void JNICALL Java_com_unisound_aios_audiocheck_JNI_LocalAsrEngine_AsrEngineLoggerLevel
  (JNIEnv *env, jobject obj, jint log_level) {
  pthread_mutex_lock(&g_mutex);
  if (0 == LogLevelSet((LogLevel)log_level)) {
    LOGT(TAG, "set log_level=%d", log_level);
  } else {
    LOGE(TAG, "log level[%d] invalid", log_level);
  }
  pthread_mutex_unlock(&g_mutex);

  fflush(stdout);
}
