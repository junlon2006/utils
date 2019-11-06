/**************************************************************************
 * Copyright (C) 2017-2017  junlon2006
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **************************************************************************
 *
 * Description : uni_mp3_player.c
 * Author      : junlon2006@163.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
#include "uni_mp3_player.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include "uni_log.h"
#include "uni_dlopen.h"
#include "uni_ringbuf.h"
#include <inttypes.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

#define MP3_PLAYER_TAG            "mp3_player"
#define AUDIO_OUT_SIZE            (8 * 1024)
#define OPEN_INPUT_TIMEOUT_S      (10)
#define READ_HEADER_TIMEOUT_S     (5)
#define READ_FRAME_TIMEOUT_S      (15)
/* must cache large than READ_HEADER_TIMEOUT_S second decode raw pcm data */
#define SOURCE_CACHE_BUF_SIZE     (1024 * 1024)   /* 24K sample_fmt * 21.8s raw pcm data */
#define DOWNLOAD_SPEED_RATIO      (2)
#define SYMBOL_NAME(symbol)       #symbol
#define FFMPEG_PATH               "lib/ffmpeg/"

#define _min(a, b)                (((a) < (b)) ? (a) : (b))

typedef enum {
  MP3_IDLE_STATE = 0,
  MP3_PREPARING_STATE,
  MP3_PREPARED_STATE,
  MP3_PAUSED_STATE,
  MP3_PLAYING_STATE
} Mp3State;

typedef enum {
  MP3_PLAY_EVENT,
  MP3_PREPARE_EVENT,
  MP3_START_EVENT,
  MP3_PAUSE_EVENT,
  MP3_RESUME_EVENT,
  MP3_STOP_EVENT
} Mp3Event;

typedef enum {
  BLOCK_NULL = 0,
  BLOCK_OPEN_INPUT,
  BLOCK_READ_HEADER,
  BLOCK_READ_FRAME,
} BlockState;

typedef struct _ConvertCtxNode{
  uint32_t               channel_layout;
  enum AVSampleFormat    sample_fmt;
  int32_t                sample_rate;
  struct SwrContext      *au_convert_ctx;
  struct _ConvertCtxNode *next;
} ConvertCtxNode;

typedef struct {
  DlOpenHandle avformat;
  DlOpenHandle avcodec;
  DlOpenHandle avutil;
  DlOpenHandle swr;
  int (*av_find_best_stream)(AVFormatContext *ic, enum AVMediaType type,
                             int wanted_stream_nb, int related_stream,
                             AVCodec **decoder_ret, int flags);
  const char *(*av_get_media_type_string)(enum AVMediaType media_type);
  AVCodec *(*avcodec_find_decoder_by_name)(const char *name);
  AVCodec *(*avcodec_find_decoder)(enum AVCodecID id);
  AVCodecContext *(*avcodec_alloc_context3)(const AVCodec *codec);
  int (*avcodec_parameters_to_context)(AVCodecContext *codec,
                                       const AVCodecParameters *par);
  int (*avcodec_open2)(AVCodecContext *avctx, const AVCodec *codec,
                       AVDictionary **options);
  void (*av_register_all)(void);
  int (*avformat_network_init)(void);
  AVFormatContext *(*avformat_alloc_context)(void);
  int (*avformat_open_input)(AVFormatContext **ps, const char *filename,
                             AVInputFormat *fmt, AVDictionary **options);
  int (*avformat_find_stream_info)(AVFormatContext *ic, AVDictionary **options);
  void (*av_dump_format)(AVFormatContext *ic, int index, const char *url,
                         int is_output);
  void (*av_init_packet)(AVPacket *pkt);
  int64_t (*av_get_default_channel_layout)(int nb_channels);
  int (*av_get_bytes_per_sample)(enum AVSampleFormat sample_fmt);
  int (*avcodec_decode_audio4)(AVCodecContext *avctx, AVFrame *frame,
                               int *got_frame_ptr, const AVPacket *avpkt);
  void (*av_packet_unref)(AVPacket *pkt);
  void (*avcodec_free_context)(AVCodecContext **pavctx);
  void (*avformat_close_input)(AVFormatContext **ps);
  void (*av_frame_free)(AVFrame **frame);
  void (*av_free)(void *ptr);
  int (*avformat_network_deinit)(void);
  int (*av_read_frame)(AVFormatContext *s, AVPacket *pkt);
  AVFrame *(*av_frame_alloc)(void);
  void *(*av_malloc)(size_t size);
  void (*av_dns_parse_init)(const char *config_path);
  struct SwrContext *(*swr_alloc_set_opts)(struct SwrContext *s,
                                           int64_t out_ch_layout,
                                           enum AVSampleFormat out_sample_fmt,
                                           int out_sample_rate,
                                           int64_t in_ch_layout,
                                           enum AVSampleFormat in_sample_fmt,
                                           int in_sample_rate,
                                           int log_offset, void *log_ctx);
  int (*swr_init)(struct SwrContext *s);
  void (*swr_free)(SwrContext **ss);
  int (*swr_get_out_samples)(struct SwrContext *s, int in_samples);
  int (*swr_convert)(struct SwrContext *s, uint8_t **out, int out_count,
                     uint8_t **in , int in_count);
  int (*av_strerror)(int errnum, char *errbuf, size_t errbuf_size);
} FFmpegSymbolTable;

static struct {
  AVFormatContext     *fmt_ctx;
  AVCodecContext      *audio_dec_ctx;
  struct SwrContext   *au_convert_ctx;
  ConvertCtxNode      *convert_ctx_list;
  AVPacket            pkt;
  AVPacket            orig_pkt;
  AVFrame             *frame;
  int32_t             audio_stream_idx;
  int64_t             out_channel_layout;
  int32_t             out_sample_rate;
  enum AVSampleFormat out_sample_fmt;
  int32_t             out_channels;
  uint8_t             *out_buffer;
  Mp3State            state;
  pthread_t           prepare_thread;
  pthread_mutex_t     mutex;
  int32_t             last_timestamp;
  int32_t             block_state;
  sem_t               sem_wait_download;
  sem_t               sem_downloading;
  sem_t               sem_exit_download;
  sem_t               sem_sync_exit_thread;
  bool                download_task_is_running;
  bool                force_stop_download;
  bool                current_download_finished;
  RingBufferHandle       source_ringbuf;
  FFmpegSymbolTable   ffmpeg_symbol_table;
} g_mp3_player;

static int64_t _get_clock_time_ms() {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
    return (ts.tv_sec * 1000L + (ts.tv_nsec / 1000000));
  }
  return 0;
}

static void _close_ffmpeg_library() {
  FFmpegSymbolTable *symbols = &g_mp3_player.ffmpeg_symbol_table;
  if (NULL != symbols->avformat) {
    DlOpenSharedLibraryClose(symbols->avformat);
    symbols->avformat = NULL;
  }
  if (NULL != symbols->avutil) {
    DlOpenSharedLibraryClose(symbols->avutil);
    symbols->avutil = NULL;
  }
  if (NULL != symbols->swr) {
    DlOpenSharedLibraryClose(symbols->swr);
    symbols->swr = NULL;
  }
  if (NULL != symbols->avcodec) {
    DlOpenSharedLibraryClose(symbols->avcodec);
    symbols->avcodec = NULL;
  }
}

static int _load_ffmpeg_library_symbol() {
  FFmpegSymbolTable *symbols = &g_mp3_player.ffmpeg_symbol_table;
  DlOpenHandle avformat = NULL;
  DlOpenHandle avcodec = NULL;
  DlOpenHandle swr = NULL;
  DlOpenHandle avutil = NULL;
  avformat = DlOpenLoadSharedLibrary(FFMPEG_PATH"libavformat.so.57");
  if (NULL == avformat) {
    LOGE(MP3_PLAYER_TAG, "load libavformat.so failed");
    goto L_ERROR;
  }
  symbols->avformat = avformat;
  avcodec = DlOpenLoadSharedLibrary(FFMPEG_PATH"libavcodec.so.57");
  if (NULL == avcodec) {
    LOGE(MP3_PLAYER_TAG, "load libavcodec.so failed");
    goto L_ERROR;
  }
  symbols->avcodec = avcodec;
  swr = DlOpenLoadSharedLibrary(FFMPEG_PATH"libswresample.so.2");
  if (NULL == swr) {
    LOGE(MP3_PLAYER_TAG, "load libswresample.so failed");
    goto L_ERROR;
  }
  symbols->swr = swr;
  avutil = DlOpenLoadSharedLibrary(FFMPEG_PATH"libavutil.so.55");
  if (NULL == avutil) {
    LOGE(MP3_PLAYER_TAG, "load libavutil.so failed");
    goto L_ERROR;
  }
  symbols->avutil = avutil;
  symbols->av_find_best_stream = DlOpenLoadSharedLibrarySymbol(avformat,
      SYMBOL_NAME(av_find_best_stream));
  if (NULL == symbols->av_find_best_stream) {
    LOGE(MP3_PLAYER_TAG, "load symbol av_find_best_stream failed");
    goto L_ERROR;
  }
  symbols->av_get_media_type_string = DlOpenLoadSharedLibrarySymbol(avutil,
      SYMBOL_NAME(av_get_media_type_string));
  if (NULL == symbols->av_get_media_type_string) {
    LOGE(MP3_PLAYER_TAG, "load symbol av_get_media_type_string failed");
    goto L_ERROR;
  }
  symbols->avcodec_find_decoder_by_name = DlOpenLoadSharedLibrarySymbol(avcodec,
      SYMBOL_NAME(avcodec_find_decoder_by_name));
  if (NULL == symbols->avcodec_find_decoder_by_name) {
    LOGE(MP3_PLAYER_TAG, "load symbol avcodec_find_decoder_by_name failed");
    goto L_ERROR;
  }
  symbols->avcodec_find_decoder = DlOpenLoadSharedLibrarySymbol(avcodec,
      SYMBOL_NAME(avcodec_find_decoder));
  if (NULL == symbols->avcodec_find_decoder) {
    LOGE(MP3_PLAYER_TAG, "load symbol avcodec_find_decoder failed");
    goto L_ERROR;
  }
  symbols->avcodec_alloc_context3 = DlOpenLoadSharedLibrarySymbol(avcodec,
      SYMBOL_NAME(avcodec_alloc_context3));
  if (NULL == symbols->avcodec_alloc_context3) {
    LOGE(MP3_PLAYER_TAG, "load symbol avcodec_alloc_context3 failed");
    goto L_ERROR;
  }
  symbols->avcodec_parameters_to_context = DlOpenLoadSharedLibrarySymbol(avcodec,
      SYMBOL_NAME(avcodec_parameters_to_context));
  if (NULL == symbols->avcodec_parameters_to_context) {
    LOGE(MP3_PLAYER_TAG, "load symbol avcodec_parameters_to_context failed");
    goto L_ERROR;
  }
  symbols->avcodec_open2 = DlOpenLoadSharedLibrarySymbol(avcodec,
      SYMBOL_NAME(avcodec_open2));
  if (NULL == symbols->avcodec_open2) {
    LOGE(MP3_PLAYER_TAG, "load symbol avcodec_open2 failed");
    goto L_ERROR;
  }
  symbols->av_register_all = DlOpenLoadSharedLibrarySymbol(avformat,
      SYMBOL_NAME(av_register_all));
  if (NULL == symbols->av_register_all) {
    LOGE(MP3_PLAYER_TAG, "load symbol av_register_all failed");
    goto L_ERROR;
  }
  symbols->avformat_network_init = DlOpenLoadSharedLibrarySymbol(avformat,
      SYMBOL_NAME(avformat_network_init));
  if (NULL == symbols->avformat_network_init) {
    LOGE(MP3_PLAYER_TAG, "load symbol avformat_network_init failed");
    goto L_ERROR;
  }
  symbols->avformat_alloc_context = DlOpenLoadSharedLibrarySymbol(avformat,
      SYMBOL_NAME(avformat_alloc_context));
  if (NULL == symbols->avformat_alloc_context) {
    LOGE(MP3_PLAYER_TAG, "load symbol avformat_alloc_context failed");
    goto L_ERROR;
  }
  symbols->avformat_open_input = DlOpenLoadSharedLibrarySymbol(avformat,
      SYMBOL_NAME(avformat_open_input));
  if (NULL == symbols->avformat_open_input) {
    LOGE(MP3_PLAYER_TAG, "load symbol avformat_open_input failed");
    goto L_ERROR;
  }
  symbols->avformat_find_stream_info = DlOpenLoadSharedLibrarySymbol(avformat,
      SYMBOL_NAME(avformat_find_stream_info));
  if (NULL == symbols->avformat_find_stream_info) {
    LOGE(MP3_PLAYER_TAG, "load symbol avformat_find_stream_info failed");
    goto L_ERROR;
  }
  symbols->av_dump_format = DlOpenLoadSharedLibrarySymbol(avformat,
      SYMBOL_NAME(av_dump_format));
  if (NULL == symbols->av_dump_format) {
    LOGE(MP3_PLAYER_TAG, "load symbol av_dump_format failed");
    goto L_ERROR;
  }
  symbols->av_init_packet = DlOpenLoadSharedLibrarySymbol(avcodec,
      SYMBOL_NAME(av_init_packet));
  if (NULL == symbols->av_init_packet) {
    LOGE(MP3_PLAYER_TAG, "load symbol av_init_packet failed");
    goto L_ERROR;
  }
  symbols->av_get_default_channel_layout = DlOpenLoadSharedLibrarySymbol(avutil,
      SYMBOL_NAME(av_get_default_channel_layout));
  if (NULL == symbols->av_get_default_channel_layout) {
    LOGE(MP3_PLAYER_TAG, "load symbol av_get_default_channel_layout failed");
    goto L_ERROR;
  }
  symbols->av_get_bytes_per_sample = DlOpenLoadSharedLibrarySymbol(avutil,
      SYMBOL_NAME(av_get_bytes_per_sample));
  if (NULL == symbols->av_get_bytes_per_sample) {
    LOGE(MP3_PLAYER_TAG, "load symbol av_get_bytes_per_sample failed");
    goto L_ERROR;
  }
  symbols->avcodec_decode_audio4 = DlOpenLoadSharedLibrarySymbol(avcodec,
      SYMBOL_NAME(avcodec_decode_audio4));
  if (NULL == symbols->avcodec_decode_audio4) {
    LOGE(MP3_PLAYER_TAG, "load symbol avcodec_decode_audio4 failed");
    goto L_ERROR;
  }
  symbols->av_packet_unref = DlOpenLoadSharedLibrarySymbol(avcodec,
      SYMBOL_NAME(av_packet_unref));
  if (NULL == symbols->av_packet_unref) {
    LOGE(MP3_PLAYER_TAG, "load symbol av_packet_unref failed");
    goto L_ERROR;
  }
  symbols->avcodec_free_context = DlOpenLoadSharedLibrarySymbol(avcodec,
      SYMBOL_NAME(avcodec_free_context));
  if (NULL == symbols->avcodec_free_context) {
    LOGE(MP3_PLAYER_TAG, "load symbol avcodec_free_context failed");
    goto L_ERROR;
  }
  symbols->avformat_close_input = DlOpenLoadSharedLibrarySymbol(avformat,
      SYMBOL_NAME(avformat_close_input));
  if (NULL == symbols->avformat_close_input) {
    LOGE(MP3_PLAYER_TAG, "load symbol avformat_close_input failed");
    goto L_ERROR;
  }
  symbols->av_frame_free = DlOpenLoadSharedLibrarySymbol(avutil,
      SYMBOL_NAME(av_frame_free));
  if (NULL == symbols->av_frame_free) {
    LOGE(MP3_PLAYER_TAG, "load symbol av_frame_free failed");
    goto L_ERROR;
  }
  symbols->av_free = DlOpenLoadSharedLibrarySymbol(avutil, SYMBOL_NAME(av_free));
  if (NULL == symbols->av_free) {
    LOGE(MP3_PLAYER_TAG, "load symbol av_free failed");
    goto L_ERROR;
  }
  symbols->avformat_network_deinit = DlOpenLoadSharedLibrarySymbol(avformat,
      SYMBOL_NAME(avformat_network_deinit));
  if (NULL == symbols->avformat_network_deinit) {
    LOGE(MP3_PLAYER_TAG, "load symbol avformat_network_deinit failed");
    goto L_ERROR;
  }
  symbols->av_read_frame = DlOpenLoadSharedLibrarySymbol(avformat,
      SYMBOL_NAME(av_read_frame));
  if (NULL == symbols->av_read_frame) {
    LOGE(MP3_PLAYER_TAG, "load symbol av_read_frame failed");
    goto L_ERROR;
  }
  symbols->av_frame_alloc = DlOpenLoadSharedLibrarySymbol(avutil,
      SYMBOL_NAME(av_frame_alloc));
  if (NULL == symbols->av_frame_alloc) {
    LOGE(MP3_PLAYER_TAG, "load symbol av_frame_alloc failed");
    goto L_ERROR;
  }
  symbols->av_malloc = DlOpenLoadSharedLibrarySymbol(avutil,
      SYMBOL_NAME(av_malloc));
  if (NULL == symbols->av_malloc) {
    LOGE(MP3_PLAYER_TAG, "load symbol av_malloc failed");
    goto L_ERROR;
  }
  symbols->av_dns_parse_init = DlOpenLoadSharedLibrarySymbol(avformat,
      SYMBOL_NAME(av_dns_parse_init));
  if (NULL == symbols->av_dns_parse_init) {
    LOGE(MP3_PLAYER_TAG, "load symbol av_dns_parse_init failed");
    goto L_ERROR;
  }
  symbols->swr_alloc_set_opts = DlOpenLoadSharedLibrarySymbol(swr,
      SYMBOL_NAME(swr_alloc_set_opts));
  if (NULL == symbols->swr_alloc_set_opts) {
    LOGE(MP3_PLAYER_TAG, "load symbol swr_alloc_set_opts failed");
    goto L_ERROR;
  }
  symbols->swr_init = DlOpenLoadSharedLibrarySymbol(swr, SYMBOL_NAME(swr_init));
  if (NULL == symbols->swr_init) {
    LOGE(MP3_PLAYER_TAG, "load symbol swr_init failed");
    goto L_ERROR;
  }
  symbols->swr_free = DlOpenLoadSharedLibrarySymbol(swr, SYMBOL_NAME(swr_free));
  if (NULL == symbols->swr_free) {
    LOGE(MP3_PLAYER_TAG, "load symbol swr_free failed");
    goto L_ERROR;
  }
  symbols->swr_get_out_samples = DlOpenLoadSharedLibrarySymbol(swr,
      SYMBOL_NAME(swr_get_out_samples));
  if (NULL == symbols->swr_get_out_samples) {
    LOGE(MP3_PLAYER_TAG, "load symbol swr_get_out_samples failed");
    goto L_ERROR;
  }
  symbols->swr_convert = DlOpenLoadSharedLibrarySymbol(swr,
      SYMBOL_NAME(swr_convert));
  if (NULL == symbols->swr_convert) {
    LOGE(MP3_PLAYER_TAG, "load symbol swr_convert failed");
    goto L_ERROR;
  }
  symbols->av_strerror = DlOpenLoadSharedLibrarySymbol(avutil,
      SYMBOL_NAME(av_strerror));
  if (NULL == symbols->av_strerror) {
    LOGE(MP3_PLAYER_TAG, "load symbol av_strerror failed");
    goto L_ERROR;
  }
  LOGT(MP3_PLAYER_TAG, "load ffmpeg symbol table success");
  return 0;
L_ERROR:
  _close_ffmpeg_library();
  return -1;
}

static const char* _block_state_2_string(BlockState state) {
  static const char* block_state[] = {
    [BLOCK_NULL]        = "BLOCK_NULL",
    [BLOCK_OPEN_INPUT]  = "BLOCK_OPEN_INPUT",
    [BLOCK_READ_HEADER] = "BLOCK_READ_HEADER",
    [BLOCK_READ_FRAME]  = "BLOCK_READ_FRAME",
  };
  return block_state[state];
}

static int32_t interrupt_cb(void *ctx) {
  int32_t seconds;
  int32_t timeout;
  seconds = time((time_t *)NULL);
  LOGD(MP3_PLAYER_TAG, "%s", _block_state_2_string(g_mp3_player.block_state));
  switch (g_mp3_player.block_state) {
    case BLOCK_NULL:
      return 0;
    case BLOCK_OPEN_INPUT:
      timeout = OPEN_INPUT_TIMEOUT_S;
      break;
    case BLOCK_READ_HEADER:
      timeout = READ_HEADER_TIMEOUT_S;
      break;
    case BLOCK_READ_FRAME:
      timeout = READ_FRAME_TIMEOUT_S;
      break;
    default:
      LOGW(MP3_PLAYER_TAG, "invalid block state [%d]!!!!!!",
           g_mp3_player.block_state);
      return 0;
  }
  if (seconds - g_mp3_player.last_timestamp >= timeout) {
    LOGE(MP3_PLAYER_TAG, "ffmpeg hit timeout at state [%d, %s], "
         "curr_sec=%d, last_sec=%d", g_mp3_player.block_state,
         _block_state_2_string(g_mp3_player.block_state),
         seconds, g_mp3_player.last_timestamp);
    return 1;
  }
  return 0;
}

static const AVIOInterruptCB int_cb = {interrupt_cb, NULL};

static char* _event2string(Mp3Event event) {
  switch (event) {
  case MP3_PLAY_EVENT:
    return "MP3_PLAY_EVENT";
  case MP3_PREPARE_EVENT:
    return "MP3_PREPARE_EVENT";
  case MP3_START_EVENT:
    return "MP3_START_EVENT";
  case MP3_PAUSE_EVENT:
    return "MP3_PAUSE_EVENT";
  case MP3_RESUME_EVENT:
    return "MP3_RESUME_EVENT";
  case MP3_STOP_EVENT:
    return "MP3_STOP_EVENT";
  default:
    break;
  }
  return "N/A";
}

static char* _state2string(Mp3State state) {
  switch (state) {
    case MP3_IDLE_STATE:
      return "MP3_IDLE_STATE";
    case MP3_PREPARING_STATE:
      return "MP3_PREPARING_STATE";
    case MP3_PREPARED_STATE:
      return "MP3_PREPARED_STATE";
    case MP3_PAUSED_STATE:
      return "MP3_PAUSED_STATE";
    case MP3_PLAYING_STATE:
      return "MP3_PLAYING_STATE";
    default:
      break;
  }
  return "N/A";
}

static void _mp3_set_state(Mp3State state) {
  g_mp3_player.state = state;
  LOGT(MP3_PLAYER_TAG, "mp3 state is set to %d", state);
}

static ConvertCtxNode* _create_convert_ctx_node(uint32_t channel_layout,
                                                enum AVSampleFormat sample_fmt,
                                                int32_t sample_rate) {
  FFmpegSymbolTable *symbols = &g_mp3_player.ffmpeg_symbol_table;
  ConvertCtxNode *node, *head = g_mp3_player.convert_ctx_list;
  node = malloc(sizeof(ConvertCtxNode));
  node->channel_layout = channel_layout;
  node->sample_fmt = sample_fmt;
  node->sample_rate = sample_rate;
  node->au_convert_ctx =
    symbols->swr_alloc_set_opts(NULL,
                                g_mp3_player.out_channel_layout,
                                g_mp3_player.out_sample_fmt,
                                g_mp3_player.out_sample_rate,
                                node->channel_layout,
                                node->sample_fmt,
                                node->sample_rate,
                                0,
                                NULL);
  node->next = NULL;
  symbols->swr_init(node->au_convert_ctx);
  if (NULL == head) {
    g_mp3_player.convert_ctx_list = node;
    return node;
  }
  while (NULL != head->next) {
    head = head->next;
  }
  head->next = node;
  return node;
}

static void _destroy_convert_ctx_node(ConvertCtxNode *node) {
  FFmpegSymbolTable *symbols = &g_mp3_player.ffmpeg_symbol_table;
  if (NULL != node) {
    symbols->swr_free(&node->au_convert_ctx);
    free(node);
  }
}

static void _choose_au_convert_ctx(uint32_t channel_layout,
                                   enum AVSampleFormat sample_fmt,
                                   int32_t sample_rate) {
  ConvertCtxNode *node = g_mp3_player.convert_ctx_list;
  FFmpegSymbolTable *symbols = &g_mp3_player.ffmpeg_symbol_table;
  while (NULL != node) {
    if (node->channel_layout == channel_layout &&
        node->sample_fmt == sample_fmt &&
        node->sample_rate == sample_rate) {
      g_mp3_player.au_convert_ctx = node->au_convert_ctx;
      symbols->swr_init(node->au_convert_ctx);
      LOGW(MP3_PLAYER_TAG, "channel_layout=%d, sample_fmt=%d,"
           "sample_rate=%d", node->channel_layout, node->sample_fmt,
           node->sample_rate);
      return;
    }
    node = node->next;
  }
  node = _create_convert_ctx_node(channel_layout, sample_fmt, sample_rate);
  g_mp3_player.au_convert_ctx = node->au_convert_ctx;
  return;
}

static int32_t _open_codec_context(int32_t *stream_idx,
                                   AVCodecContext **dec_ctx,
                                   AVFormatContext *fmt_ctx,
                                   enum AVMediaType type) {
  int32_t ret, stream_index;
  AVStream *st;
  AVCodec *dec = NULL;
  AVDictionary *opts = NULL;
  FFmpegSymbolTable *symbols = &g_mp3_player.ffmpeg_symbol_table;
  ret = symbols->av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
  if (ret < 0) {
    LOGE(MP3_PLAYER_TAG, "Could not find %s stream in input file",
         symbols->av_get_media_type_string(type));
    return ret;
  }
  stream_index = ret;
  st = fmt_ctx->streams[stream_index];
  dec = symbols->avcodec_find_decoder(st->codecpar->codec_id);
  if (!dec) {
    LOGE(MP3_PLAYER_TAG, "Failed to find %s codec",
         symbols->av_get_media_type_string(type));
    return AVERROR(EINVAL);
  }
  *dec_ctx = symbols->avcodec_alloc_context3(dec);
  if (!*dec_ctx) {
    LOGE(MP3_PLAYER_TAG, "Failed to allocate the %s codec context",
         symbols->av_get_media_type_string(type));
    return AVERROR(ENOMEM);
  }
  ret = symbols->avcodec_parameters_to_context(*dec_ctx, st->codecpar);
  if (ret < 0) {
    LOGE(MP3_PLAYER_TAG, "Failed to copy %s codec parameter to decoder context",
         symbols->av_get_media_type_string(type));
    return ret;
  }
  LOGT(MP3_PLAYER_TAG, "before avcodec_open2");
  if ((ret = symbols->avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
    LOGE(MP3_PLAYER_TAG, "Failed to open %s codec",
         symbols->av_get_media_type_string(type));
    return ret;
  }
  *stream_idx = stream_index;
  return 0;
}

static void _set_block_state(BlockState state) {
  g_mp3_player.last_timestamp = time(NULL);
  g_mp3_player.block_state = state;
  LOGD(MP3_PLAYER_TAG, "timestamp=%d, state=%s", g_mp3_player.last_timestamp,
       _block_state_2_string(g_mp3_player.block_state));
}

static void _reinit_sem_exit_download() {
  sem_destroy(&g_mp3_player.sem_exit_download);
  sem_init(&g_mp3_player.sem_exit_download, 0, 1);
}

static void _mp3_source_download_start() {
  _reinit_sem_exit_download();
  sem_post(&g_mp3_player.sem_wait_download);
  sem_wait(&g_mp3_player.sem_downloading);
}

static int _mp3_prepare_internal(const char *url) {
  FFmpegSymbolTable *symbols = &g_mp3_player.ffmpeg_symbol_table;
  symbols->av_register_all();
  symbols->avformat_network_init();
  g_mp3_player.fmt_ctx = symbols->avformat_alloc_context();
  if (NULL == g_mp3_player.fmt_ctx) {
    LOGE(MP3_PLAYER_TAG, "Could not alloc context");
    return -1;
  }
  g_mp3_player.fmt_ctx->interrupt_callback = int_cb;
  _set_block_state(BLOCK_OPEN_INPUT);
  LOGT(MP3_PLAYER_TAG, "before avformat_open_input");
  if (symbols->avformat_open_input(&g_mp3_player.fmt_ctx, url,
                                   NULL, NULL) < 0) {
    LOGE(MP3_PLAYER_TAG, "Could not open source file %s", url);
    return -1;
  }
  _set_block_state(BLOCK_READ_HEADER);
  LOGT(MP3_PLAYER_TAG, "before avformat_find_stream_info");
  if (symbols->avformat_find_stream_info(g_mp3_player.fmt_ctx, NULL) < 0) {
    LOGE(MP3_PLAYER_TAG, "Could not find stream information");
    return -1;
  }
  LOGT(MP3_PLAYER_TAG, "before _open_codec_context");
  if (_open_codec_context(&g_mp3_player.audio_stream_idx,
                          &g_mp3_player.audio_dec_ctx,
                          g_mp3_player.fmt_ctx, AVMEDIA_TYPE_AUDIO) < 0) {
    LOGE(MP3_PLAYER_TAG, "Open codec context failed");
    return -1;
  }
  if (NULL == g_mp3_player.fmt_ctx->streams[g_mp3_player.audio_stream_idx]) {
    LOGE(MP3_PLAYER_TAG, "Could not find audio stream");
    return -1;
  }
  _set_block_state(BLOCK_NULL);
  LOGT(MP3_PLAYER_TAG, "before av_dump_format");
  symbols->av_dump_format(g_mp3_player.fmt_ctx, 0, url, 0);
  if (NULL == (g_mp3_player.frame = symbols->av_frame_alloc())) {
    LOGE(MP3_PLAYER_TAG, "Could not allocate frame");
    return -1;
  }
  symbols->av_init_packet(&g_mp3_player.pkt);
  g_mp3_player.pkt.data = NULL;
  g_mp3_player.pkt.size = 0;
  g_mp3_player.out_buffer = (uint8_t *)symbols->av_malloc(AUDIO_OUT_SIZE);
  LOGT(MP3_PLAYER_TAG, "before _choose_au_convert_ctx");
  _choose_au_convert_ctx(symbols->av_get_default_channel_layout( \
                         g_mp3_player.audio_dec_ctx->channels),
                         g_mp3_player.audio_dec_ctx->sample_fmt,
                         g_mp3_player.audio_dec_ctx->sample_rate);
  LOGT(MP3_PLAYER_TAG, "prepare internal success");
  _mp3_source_download_start();
  return 0;
}

static int32_t _threshold_pcm_size(int32_t time_offset) {
  int32_t sample_size;
  if (g_mp3_player.out_sample_fmt == AV_SAMPLE_FMT_S16) {
    sample_size = 2;
  } else {
    sample_size = 4;
  }
  return time_offset *
         ((g_mp3_player.out_sample_rate / 1000) * (sample_size)) *
         DOWNLOAD_SPEED_RATIO;
}

static void _decode_speed_limited(int32_t len) {
  static int32_t time_start = -1;
  int32_t time_current, time_offset;
  static int32_t total_len = 0;
  if (-1 == time_start) {
    time_start = _get_clock_time_ms();
  }
  time_current = _get_clock_time_ms();
  total_len += len;
  time_offset = time_current - time_start;
  if (time_offset >= 1000) {
    total_len = 0;
    time_start = time_current;
    return;
  }
  if (total_len >= 8192 &&
      time_offset > 10 &&
      total_len > _threshold_pcm_size(time_offset)) {
    total_len = 0;
    time_start = time_current;
    usleep(0);
  }
}

static void _write_ringbuffer(RingBufferHandle data_buffer, char *buf, int32_t len,
                              int32_t *actual_write_size) {
  int32_t free_size;
  int32_t write_size;
  free_size = RingBufferGetFreeSize(data_buffer);
  write_size = _min(free_size, len);
  if (0 < write_size) {
    *actual_write_size = write_size;
    RingBufferWrite(data_buffer, (char *)g_mp3_player.out_buffer, write_size);
  }
  _decode_speed_limited(write_size);
}

static int32_t _swr_context_cache_check_and_process(RingBufferHandle data_buffer,
                                                    int32_t *decode_byte_len) {
  int32_t cache_size = 0;
  int32_t data_len;
  FFmpegSymbolTable *symbols = &g_mp3_player.ffmpeg_symbol_table;
  cache_size = symbols->swr_get_out_samples(g_mp3_player.au_convert_ctx, 0);
  if (g_mp3_player.frame->nb_samples > 0 &&
      cache_size >= g_mp3_player.frame->nb_samples) {
    data_len = symbols->swr_convert(g_mp3_player.au_convert_ctx,
                                    &g_mp3_player.out_buffer,
                                    AUDIO_OUT_SIZE / 2, NULL, 0);
    if (data_len < 0) {
      LOGE(MP3_PLAYER_TAG, "Could not convert input samples (error '%d')",
           data_len);
      goto L_END;
    }
    data_len *= (symbols->av_get_bytes_per_sample(
                 g_mp3_player.out_sample_fmt) * g_mp3_player.out_channels);
    if (0 == data_len) {
      LOGW(MP3_PLAYER_TAG, "try read swr_cache, cannot get data, datasize=0");
      goto L_END;
    }
    _write_ringbuffer(data_buffer, (char *)g_mp3_player.out_buffer, data_len,
                      decode_byte_len);
    return 0;
  }
L_END:
  return -1;
}

static int32_t _decode_packet(RingBufferHandle data_buffer,
                              int32_t *decode_byte_len) {
  int32_t ret = 0;
  int32_t decoded = g_mp3_player.pkt.size;
  int32_t got_frame = 0;
  int32_t data_len;
  char errbuf[AV_ERROR_MAX_STRING_SIZE];
  FFmpegSymbolTable *symbols = &g_mp3_player.ffmpeg_symbol_table;
  if (g_mp3_player.pkt.stream_index == g_mp3_player.audio_stream_idx) {
    if (0 == _swr_context_cache_check_and_process(data_buffer,
                                                  decode_byte_len)) {
      return 0;
    }
    ret = symbols->avcodec_decode_audio4(g_mp3_player.audio_dec_ctx,
                                         g_mp3_player.frame,
                                         &got_frame,
                                         &g_mp3_player.pkt);
    if (ret < 0) {
      *decode_byte_len = -1;
      LOGE(MP3_PLAYER_TAG, "Error decoding audio frame (%d->%s)", ret,
           (0 == symbols->av_strerror(ret, errbuf, sizeof(errbuf))) ? errbuf :
           "NULL");
      return -1;
    }
    decoded = FFMIN(ret, g_mp3_player.pkt.size);
    if (got_frame) {
      data_len = symbols->swr_convert(g_mp3_player.au_convert_ctx,
                                      &g_mp3_player.out_buffer,
                                      AUDIO_OUT_SIZE / 2,
                                      g_mp3_player.frame->data,
                                      g_mp3_player.frame->nb_samples);
      if (data_len < 0) {
        LOGE(MP3_PLAYER_TAG, "Could not convert input samples (error '%d')",
             data_len);
        return data_len;
      }
      data_len *= (symbols->av_get_bytes_per_sample(g_mp3_player.out_sample_fmt) *
                   g_mp3_player.out_channels);
      _write_ringbuffer(data_buffer, (char *)g_mp3_player.out_buffer, data_len,
                        decode_byte_len);
    }
  }
  return decoded;
}

static int32_t _decode_callback(RingBufferHandle data_buffer) {
  int32_t ret, decode_byte_len = 0;
  FFmpegSymbolTable *symbols = &g_mp3_player.ffmpeg_symbol_table;
  char errbuf[AV_ERROR_MAX_STRING_SIZE];
  /* make sure ringbuf has more than 8K (audio player ringbuf) */
  if (RingBufferGetFreeSize(data_buffer) <= 8192) {
    /* wait until enough free size, release CPU */
    usleep(100);
    return 0;
  }
  _set_block_state(BLOCK_READ_FRAME);
  if (0 == g_mp3_player.pkt.size) {
    ret = symbols->av_read_frame(g_mp3_player.fmt_ctx, &g_mp3_player.pkt);
    if (ret < 0) {
      LOGE(MP3_PLAYER_TAG, "Demuxing succeeded[%d-->%s]", ret,
           (0 == symbols->av_strerror(ret, errbuf, sizeof(errbuf))) ?
           errbuf : "NULL");
      return -1;
    }
    g_mp3_player.orig_pkt = g_mp3_player.pkt;
  }
  do {
    if (g_mp3_player.pkt.size <= 0) {
      break;
    }
    ret = _decode_packet(data_buffer, &decode_byte_len);
    if (ret < 0) {
      break;
    }
    g_mp3_player.pkt.data += ret;
    g_mp3_player.pkt.size -= ret;
  } while (0);
  if (0 == g_mp3_player.pkt.size) {
    symbols->av_packet_unref(&g_mp3_player.orig_pkt);
    memset(&g_mp3_player.orig_pkt, 0, sizeof(g_mp3_player.orig_pkt));
  }
  return decode_byte_len;
}

static void _unset_current_download_finish_flag() {
  g_mp3_player.current_download_finished = false;
  LOGT(MP3_PLAYER_TAG, "current_download_finished=%d",
       g_mp3_player.current_download_finished);
}

static void _set_current_download_finish_flag() {
  g_mp3_player.current_download_finished = true;
  LOGT(MP3_PLAYER_TAG, "current_download_finished=%d",
       g_mp3_player.current_download_finished);
}

static void* _download_tsk(void *args) {
  int ret;
  while (g_mp3_player.download_task_is_running) {
    sem_wait(&g_mp3_player.sem_wait_download);
    _unset_current_download_finish_flag();
    RingBufferClear(g_mp3_player.source_ringbuf);
    sem_post(&g_mp3_player.sem_downloading);
    while (g_mp3_player.download_task_is_running) {
      ret = _decode_callback(g_mp3_player.source_ringbuf);
      if (-1 == ret ||
          g_mp3_player.force_stop_download) {
        _set_current_download_finish_flag();
        sem_post(&g_mp3_player.sem_exit_download);
        break;
      }
    }
  }
  sem_post(&g_mp3_player.sem_sync_exit_thread);
  return NULL;
}

static void _create_download_source_tsk() {
  pthread_t pid;
  g_mp3_player.download_task_is_running = true;
  pthread_create(&pid, NULL, _download_tsk, NULL);
  pthread_detach(pid);
}

static bool _is_current_source_feed_finished() {
  return (0 == RingBufferGetDataSize(g_mp3_player.source_ringbuf) &&
          g_mp3_player.current_download_finished);
}

static int _audio_player_callback(RingBufferHandle data_buffer) {
  static char buf[8 * 1024];
  int32_t data_size, free_size, write_size;
  if (_is_current_source_feed_finished()) {
    LOGT(MP3_PLAYER_TAG, "current source retrieve finished");
    return -1;
  }
  data_size = RingBufferGetDataSize(g_mp3_player.source_ringbuf);
  free_size = RingBufferGetFreeSize(data_buffer);
  write_size = _min(data_size, free_size);
  write_size = _min(write_size, sizeof(buf));
  if (0 == write_size) {
    usleep(0);
    return 0;
  }
  RingBufferRead(buf, write_size, g_mp3_player.source_ringbuf);
  RingBufferWrite(data_buffer, buf, write_size);
  return write_size;
}

static void _mp3_start_internal(void) {
  //AudioPlayerStart(_audio_player_callback, AUDIO_MEDIA_PLAYER);
}

static void _mp3_source_download_stop() {
  /* sync download stop needed only when download started */
  if (MP3_PAUSED_STATE == g_mp3_player.state ||
      MP3_PLAYING_STATE == g_mp3_player.state) {
    LOGT(MP3_PLAYER_TAG, "sync download stop, current state=%s",
         _state2string(g_mp3_player.state));
    g_mp3_player.force_stop_download = true;
    sem_wait(&g_mp3_player.sem_exit_download);
    g_mp3_player.force_stop_download = false;
  }
}

static int _mp3_release_internal(void) {
  FFmpegSymbolTable *symbols = &g_mp3_player.ffmpeg_symbol_table;
  _mp3_source_download_stop();
  if (0 != g_mp3_player.orig_pkt.size) {
    symbols->av_packet_unref(&g_mp3_player.orig_pkt);
    memset(&g_mp3_player.orig_pkt, 0, sizeof(g_mp3_player.orig_pkt));
  }
  if (g_mp3_player.audio_dec_ctx) {
    symbols->avcodec_free_context(&g_mp3_player.audio_dec_ctx);
    g_mp3_player.audio_dec_ctx = NULL;
  }
  if (g_mp3_player.fmt_ctx) {
    symbols->avformat_close_input(&g_mp3_player.fmt_ctx);
    g_mp3_player.fmt_ctx = NULL;
  }
  if (g_mp3_player.frame) {
    symbols->av_frame_free(&g_mp3_player.frame);
    g_mp3_player.frame = NULL;
  }
  if (g_mp3_player.out_buffer) {
    symbols->av_free(g_mp3_player.out_buffer);
    g_mp3_player.out_buffer = NULL;
  }
  symbols->avformat_network_deinit();
  g_mp3_player.au_convert_ctx = NULL;
  LOGT(MP3_PLAYER_TAG, "release internal success");
  return 0;
}

static void _mp3_stop_internal(void) {
  //AudioPlayerStop(AUDIO_MEDIA_PLAYER);
}

static void* _mp3_prepare_routine(void *arg) {
  pthread_detach(g_mp3_player.prepare_thread);
  pthread_mutex_lock(&g_mp3_player.mutex);
  if (0 != _mp3_prepare_internal((char *)arg)) {
    _mp3_set_state(MP3_IDLE_STATE);
    _mp3_release_internal();
    LOGE(MP3_PLAYER_TAG, "mp3 prepared failed");
  } else {
    _mp3_set_state(MP3_PREPARED_STATE);
    LOGT(MP3_PLAYER_TAG, "mp3 prepared succedded");
  }
  pthread_mutex_unlock(&g_mp3_player.mutex);
  return NULL;
}

static int _mp3_fsm(Mp3Event event, void *param) {
  int rc = -1;
  pthread_mutex_lock(&g_mp3_player.mutex);
  switch (g_mp3_player.state) {
    case MP3_IDLE_STATE:
      if (MP3_PLAY_EVENT == event) {
        if (0 == _mp3_prepare_internal((char *)param)) {
          _mp3_start_internal();
          _mp3_set_state(MP3_PLAYING_STATE);
          rc = 0;
          break;
        }
        _mp3_release_internal();
        break;
      }
      if (MP3_PREPARE_EVENT == event) {
        pthread_create(&g_mp3_player.prepare_thread,
                       NULL, _mp3_prepare_routine, param);
        _mp3_set_state(MP3_PREPARING_STATE);
        rc = 0;
      }
      break;
    case MP3_PREPARING_STATE:
      if (MP3_START_EVENT == event || MP3_RESUME_EVENT == event) {
        while (MP3_PREPARING_STATE == g_mp3_player.state) {
          usleep(100 * 1000);
          LOGT(MP3_PLAYER_TAG, "waiting while mp3 is preparing");
        }
        if (MP3_PREPARED_STATE == g_mp3_player.state) {
          _mp3_start_internal();
          _mp3_set_state(MP3_PLAYING_STATE);
          rc = 0;
        }
      }
      break;
    case MP3_PREPARED_STATE:
      if (MP3_START_EVENT == event || MP3_RESUME_EVENT == event) {
        _mp3_start_internal();
        _mp3_set_state(MP3_PLAYING_STATE);
        rc = 0;
      } else if (MP3_STOP_EVENT == event) {
        _mp3_release_internal();
        _mp3_set_state(MP3_IDLE_STATE);
        rc = 0;
      }
      break;
    case MP3_PLAYING_STATE:
      if (MP3_PAUSE_EVENT == event) {
        _mp3_stop_internal();
        _mp3_set_state(MP3_PAUSED_STATE);
        rc = 0;
      } else if (MP3_STOP_EVENT == event) {
        _mp3_stop_internal();
        _mp3_release_internal();
        _mp3_set_state(MP3_IDLE_STATE);
        rc = 0;
      }
      break;
    case MP3_PAUSED_STATE:
      if (MP3_RESUME_EVENT == event) {
        _mp3_start_internal();
        _mp3_set_state(MP3_PLAYING_STATE);
        rc = 0;
      } else if (MP3_STOP_EVENT == event) {
        _mp3_release_internal();
        _mp3_set_state(MP3_IDLE_STATE);
        rc = 0;
      }
      break;
    default:
      break;
  }
  pthread_mutex_unlock(&g_mp3_player.mutex);
  LOGT(MP3_PLAYER_TAG, "event %s, state %s, result %s",
       _event2string(event), _state2string(g_mp3_player.state),
       rc == 0 ? "OK" : "FAILED");
  return rc;
}

int Mp3Play(char *filename) {
  return _mp3_fsm(MP3_PLAY_EVENT, (void *)filename);
}

int Mp3Prepare(char *filename) {
  LOGT(MP3_PLAYER_TAG, "playing %s", filename);
  return _mp3_fsm(MP3_PREPARE_EVENT, (void *)filename);
}

int Mp3Start(void) {
  return _mp3_fsm(MP3_START_EVENT, NULL);
}

int Mp3Pause(void) {
  return _mp3_fsm(MP3_PAUSE_EVENT, NULL);
}

int Mp3Resume(void) {
  return _mp3_fsm(MP3_RESUME_EVENT, NULL);
}

int Mp3Stop(void) {
  return _mp3_fsm(MP3_STOP_EVENT, NULL);
}

static void _sem_init() {
  sem_init(&g_mp3_player.sem_wait_download, 0, 1);
  sem_init(&g_mp3_player.sem_downloading, 0, 1);
  sem_init(&g_mp3_player.sem_exit_download, 0, 1);
  sem_init(&g_mp3_player.sem_sync_exit_thread, 0, 1);
}

static void _sem_destroy() {
  sem_destroy(&g_mp3_player.sem_wait_download);
  sem_destroy(&g_mp3_player.sem_downloading);
  sem_destroy(&g_mp3_player.sem_exit_download);
  sem_destroy(&g_mp3_player.sem_sync_exit_thread);
}

int Mp3Init(AudioParam *param) {
  FFmpegSymbolTable *symbols = &g_mp3_player.ffmpeg_symbol_table;
  memset(&g_mp3_player, 0, sizeof(g_mp3_player));
  if (0 != _load_ffmpeg_library_symbol()) {
    LOGE(MP3_PLAYER_TAG, "load ffmpeg library symbols failed");
    return -1;
  }
  pthread_mutex_init(&g_mp3_player.mutex, NULL);
  g_mp3_player.out_channels = param->channels;
  g_mp3_player.out_sample_rate = param->rate;
  if (param->channels == 1) {
    g_mp3_player.out_channel_layout = AV_CH_LAYOUT_MONO;
  } else {
    g_mp3_player.out_channel_layout = AV_CH_LAYOUT_STEREO;
  }
  if (param->bit == 16) {
    g_mp3_player.out_sample_fmt = AV_SAMPLE_FMT_S16;
  } else {
    g_mp3_player.out_sample_fmt = AV_SAMPLE_FMT_S32;
  }
  _sem_init();
  g_mp3_player.source_ringbuf = RingBufferCreate(SOURCE_CACHE_BUF_SIZE);
  _create_download_source_tsk();
  //symbols->av_dns_parse_init(WriteableFilePathGet());
  return 0;
}

int Mp3Final(void) {
  ConvertCtxNode *head = g_mp3_player.convert_ctx_list;
  ConvertCtxNode *node = head;
  g_mp3_player.download_task_is_running = false;
  sem_post(&g_mp3_player.sem_wait_download);
  sem_wait(&g_mp3_player.sem_sync_exit_thread);
  pthread_mutex_destroy(&g_mp3_player.mutex);
  _sem_destroy();
  while (NULL != node) {
    head = node->next;
    _destroy_convert_ctx_node(node);
    node = head;
  }
  _close_ffmpeg_library();
  return 0;
}

bool Mp3CheckIsPlaying(void) {
  return (g_mp3_player.state == MP3_PLAYING_STATE);
}

bool Mp3CheckIsPause(void) {
  return (g_mp3_player.state == MP3_PAUSED_STATE);
}
