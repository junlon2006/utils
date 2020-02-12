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

#define AM_PATH       "models"
#define GRAMMAR_PATH  "grammar/ivm_cmd.jsgf.dat"
#define DOMAIN_ID     (54)
#define WAV_PATH      "../wav"
#define TXT_PATH      "../txt"
#define min(a, b)     (a > b ? b : a)
#define PACKED        __attribute__ ((packed))

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

void get_begin_end_time(float *start_ms, float *stop_ms) {
  *start_ms = (float)UalOFAGetOptionInt(ASR_ENGINE_UTTERANCE_START_TIME) / 1000.0;
  *stop_ms = (float)UalOFAGetOptionInt(ASR_ENGINE_UTTERANCE_STOP_TIME) / 1000.0;
}

typedef struct {
  char *chinese_spell;
  char *chinese_word;
} KeyWordMapping;

static KeyWordMapping g_mapping[] = {
  {"zhinengdianti",   "智能电梯"},
  {"diantishangxing", "电梯上行"},
  {"diantixiaxing",   "电梯下行"}
};

static void _get_keyword(const char *file_name, char* keyword) {
  int cnt = sizeof(g_mapping) / sizeof(g_mapping[0]);
  for (int i = 0; i < cnt; i++) {
    if (NULL != strstr(file_name, g_mapping[i].chinese_spell)) {
      strcpy(keyword,  g_mapping[i].chinese_word);
      return;
    }
  }

  strcpy(keyword, "filename invalid");
}

static void _get_txt_file_name_by_wav_file_name(const char *wav_file_name,
                                                char *txt_name) {
  int len;
  strcpy(txt_name, wav_file_name);
  len = strlen(txt_name);
  txt_name[len - 3] = 't';
  txt_name[len - 2] = 'x';
  txt_name[len - 1] = 't';
  printf("txt_name=%s\n", txt_name);
}

static void _write_recognize_2_txt_file(const char *txt_file_name,
                                        char *txt_result) {
  int fd = open(txt_file_name, O_RDWR | O_CREAT | O_TRUNC, 0664);
  if (fd < 0) {
    printf("open file[%s] failed\n", txt_file_name);
    return;
  }

  write(fd, txt_result, strlen(txt_result));
  close(fd);
}

static void _get_asr_result(const char *file_name) {
  float start_msec, stop_msec;
  char keyword[128];
  char txt_name[256];
  char txt_result[256];
  char *res = UalOFAGetResult();
  if (NULL != strstr(res, "#NULL")) {
    return;
  }

  _get_keyword(file_name, keyword);
  get_begin_end_time(&start_msec, &stop_msec);
  printf("asr_result=%s, start=%f, stop=%f, keyword=%s\n",
         res, start_msec, stop_msec, keyword);
  _get_txt_file_name_by_wav_file_name(file_name, txt_name);
  snprintf(txt_result, sizeof(txt_result), "%f\t%f\t%s",
           start_msec, stop_msec, keyword);
  printf("txt_result=%s\n", txt_result);
  _write_recognize_2_txt_file(txt_name, txt_result);
}

static void _recognize(const char *file_name, char *raw_data, int len) {
  char domain[16] = "ivm";
  int fix_one_recongize_len = 160;
  int i = 0;
  int status;

  status = UalOFAStart(domain, DOMAIN_ID);
  if (status != ASR_RECOGNIZER_OK) {
    printf("start failed, errno=%d\n", status);
  }

  while (len > 0) {
    status = UalOFARecognize(raw_data + (fix_one_recongize_len * i++),
                             min(fix_one_recongize_len, len));
    if (status == ASR_RECOGNIZER_PARTIAL_RESULT) {
      _get_asr_result(file_name);
    }

    len -= fix_one_recongize_len;
  }

  status = UalOFAStop();
  if (status >= 0) {
    _get_asr_result(file_name);
  }
}

static int _is_wav_format(WavHeader *wavheader, int len) {
  return (wavheader->riff[0] == 'R' &&
      wavheader->riff[1] == 'I' &&
      wavheader->riff[2] == 'F' &&
      wavheader->riff[3] == 'F' &&
      wavheader->wave[0] == 'W' &&
      wavheader->wave[1] == 'A' &&
      wavheader->wave[2] == 'V' &&
      wavheader->wave[3] == 'E' &&
      wavheader->audio_len == len);
}

static void _wav_2_raw_pcm(char *file_name, char **raw_data, int *len) {
  int fd;
  WavHeader wav_header = {0};

  printf("process [%s]\n", file_name);

  fd = open(file_name, O_RDWR, 0664);
  if (fd < 0) {
    printf("open file[%s] failed\n", file_name);
    return;
  }

  *len = lseek(fd, 0, SEEK_END) - sizeof(WavHeader);
  *raw_data = malloc(*len);
  memset(*raw_data, 0, *len);

  lseek(fd, 0, SEEK_SET);

  if (sizeof(WavHeader) != read(fd, &wav_header, sizeof(WavHeader))) {
    printf("read wavheader faild\n");
    goto L_END;
  }

  if (!_is_wav_format(&wav_header, *len)) {
    printf("not wav format, invalid\n");
    goto L_END;
  }

  if (*len != read(fd, *raw_data, *len)) {
    printf("read raw data failed\n");
    goto L_END;
  }

L_END:
  close(fd);
}


static int _get_file_type() {
  DIR *dir;
  struct dirent *ent;
  int type;

  if (NULL == (dir = opendir(WAV_PATH))) {
    printf("open dir [%s] failed\n", WAV_PATH);
    return -1;
  }

  while ((ent = readdir(dir)) != NULL) {
    if (strcmp(ent->d_name, ".") == 0) {
      type = ent->d_type;
      break;
    }
  }

  closedir(dir);
  return type;
}

static void _recognize_all_one_by_one() {
  DIR *dir;
  struct dirent *ent;
  char file_name[512];
  char *raw_data;
  int len;
  int type;

  type = _get_file_type();

  system("rm -rf "TXT_PATH"/*");

  if (NULL == (dir = opendir(WAV_PATH))) {
    printf("open dir [%s] failed\n", WAV_PATH);
    return;
  }

  while ((ent = readdir(dir)) != NULL) {
    if (ent->d_type == type) {
      if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
        continue;
      }

      sprintf(file_name, "%s/%s", WAV_PATH, ent->d_name);
      _wav_2_raw_pcm(file_name, &raw_data, &len);

      sprintf(file_name, "%s/%s", TXT_PATH, ent->d_name);
      _recognize(file_name, raw_data, len);

      free(raw_data);
    }
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

