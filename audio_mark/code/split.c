#include "ual_ofa.h"
#include "ofa_consts.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "uni_log.h"

#define AM_PATH       "models"
#define GRAMMAR_PATH  "grammar/ivm_cmd.jsgf.dat"
#define DOMAIN_ID     (54)
#define WAV_PATH      "../wav"
#define SPLIT_PATH    "../small"
#define min(a, b)     (a > b ? b : a)
#define PACKED        __attribute__ ((packed))
#define TAG           "main"

typedef struct {
  char  riff[4];
  int   file_len;
  char  wave[4];
  char  fmt[4];
  char  filter[4];
  short type;
  short channel;
  int   sample_rate;
  int   bitrate;
  short adjust;
  short bit;
  char  data[4];
  int   audio_len;
} PACKED WavHeader;

static void _get_begin_end_time(int *start_ms, int *stop_ms) {
  *start_ms = UalOFAGetOptionInt(ASR_ENGINE_UTTERANCE_START_TIME);
  *stop_ms = UalOFAGetOptionInt(ASR_ENGINE_UTTERANCE_STOP_TIME);
}

static void _save_one_pcm_data(char *raw_data, const char *file_name, int start, int end, int index) {
  char file[256];
  char name[128];
  static char silent[32000] = {0};
  strcpy(name, file_name);
  name[strlen(name) - 4] = '\0';
  int length = (end - start) * 32;
  raw_data += (start * 32);
  snprintf(file, sizeof(file), "%s/%s_%s_%d.pcm", SPLIT_PATH, name, (index % 2 == 1 ? "fast" : "normal"), index);
  LOGT(TAG, "name=%s", file);
  int fd = open(file, O_RDWR | O_CREAT | O_TRUNC, 0664);
  if (fd < 0) {
    printf("open file [%s] failed", file);
    return;
  }

  write(fd, silent, sizeof(silent));
  write(fd, raw_data, length);
  write(fd, silent, sizeof(silent));
  close(fd);
}

static int _get_asr_result(const char *file_name, char *raw_data, int *index) {
  int start_msec, stop_msec;
  char *res = UalOFAGetResult();
  if (NULL != strstr(res, "#NULL")) {
    return -1;
  }

  _get_begin_end_time(&start_msec, &stop_msec);

  *index = *index + 1;
  _save_one_pcm_data(raw_data, file_name, start_msec, stop_msec, *index);

  return 0;
}

static void _recognize(const char *file_name, char *raw_data, int len) {
  char domain[16] = "ivm";
  int fix_one_recongize_len = 160;
  int i = 0;
  int status;
  int index = 0;

  status = UalOFAStart(domain, DOMAIN_ID);
  if (status != ASR_RECOGNIZER_OK) {
    printf("start failed, errno=%d\n", status);
  }

  while (len > 0) {
    status = UalOFARecognize(raw_data + (fix_one_recongize_len * i++),
                             min(fix_one_recongize_len, len));
    if (status == ASR_RECOGNIZER_PARTIAL_RESULT) {
      _get_asr_result(file_name, raw_data, &index);
    }

    len -= fix_one_recongize_len;
  }

  status = UalOFAStop();
  if (status >= 0) {
    _get_asr_result(file_name, raw_data, &index);
  }
}

static int  _wav_2_raw_pcm(char *file_name, char **raw_data, int *len) {
  int fd;
  char header[128];

  printf("process [%s]\n", file_name);

  fd = open(file_name, O_RDWR, 0664);
  if (fd < 0) {
    printf("open file[%s] failed\n", file_name);
    return -1;
  }

  *len = lseek(fd, 0, SEEK_END) - sizeof(header);
  lseek(fd, 0, SEEK_SET);

  read(fd, header, sizeof(header));

  *raw_data = malloc(*len);
  memset(*raw_data, 0, *len);
  if (*len != read(fd, *raw_data, *len)) {
    printf("read raw data failed\n");
    free(*raw_data);
    goto L_ERROR;
  }

  close(fd);
  return 0;
L_ERROR:
  close(fd);
  return -1;
}

static void _recognize_all_one_by_one() {
  DIR *dir;
  struct dirent *ent;
  char file_name[512];
  char *raw_data;
  int len;

  system("rm -rf "SPLIT_PATH"/*");

  if (NULL == (dir = opendir(WAV_PATH))) {
    printf("open dir [%s] failed\n", WAV_PATH);
    return;
  }

  while ((ent = readdir(dir)) != NULL) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
      continue;
    }

    sprintf(file_name, "%s/%s", WAV_PATH, ent->d_name);
    if (0 != _wav_2_raw_pcm(file_name, &raw_data, &len)) {
      continue;
    }

    _recognize(ent->d_name, raw_data, len);

    free(raw_data);
  }

  closedir(dir);
}

int main(int argc, char *argv[]) {
  int status;
  int engineType = UalOFAGetEngineType();

  if (KWS_STD_ENGINE != engineType) {
    printf("engineType=%d\n", engineType);
  }

  printf("version: %s\n", UalOFAGetVersion());

  UalOFASetOptionInt(ASR_LOG_ID, 0);

  UalOFAInitialize(AM_PATH, GRAMMAR_PATH);

  status = UalOFASetOptionInt(ASR_ENGINE_SET_TYPE_ID, KWS_STD_ENGINE);
  if (status == ASR_FATAL_ERROR) {
    printf("init failed errno=%d\n", status);
  }

  UalOFASetOptionInt(ASR_SET_BEAM_ID, 1500);
  UalOFASetOptionInt(ASR_ENGINE_SWITCH_LANGUAGE, MANDARIN);

  _recognize_all_one_by_one();

  UalOFARelease();

  return 0;
}