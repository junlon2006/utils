/**
 * @file ofa_consts.h
 * @copyright 2019, Unisound. All rights reserved.
 * @brief KWS Engine Constants
 *
 * @note including:
 * @note    return value definition
 * @note    option id definition
 */
#ifndef PUBLIC_OFA_CONSTS_H_
#define PUBLIC_OFA_CONSTS_H_

/**
 * @brief 识别引擎状态码
 */
enum EngineStatusCode {
  /**错误调用*/
  ASR_RECOGNIZER_WRONG_OPS = -9,
  /**错误*/
  ASR_FATAL_ERROR = -1,
  /**正常工作*/
  ASR_RECOGNIZER_OK = 0,
  /**没有识别结果*/
  ASR_RECOGNIZER_NO_RESULT = 1,
  /**有识别结果*/
  ASR_RECOGNIZER_PARTIAL_RESULT = 2,
};

/**
 * @brief 引擎属性ID
 */
enum EngineOptionId {
  /** Log层级属性ID
   * @note 0 silence, 1 verbose, 2 debug, 3 info, 4 warn, 5 error, 6 fatal
   */
  ASR_LOG_ID = 12,
  /**CallStack属性ID @deprecated*/
  ASR_CALLSTACK_ID = 13,
  /**使用历史缓存 @deprecated */
  ASR_SET_USE_LAST_RESULT = 19,
  /**设置解码时最大token数量*/
  ASR_SET_BEAM_ID = 21,
  /**（Oneshot模式）设置VAD检查最大延迟，单位：毫秒*/
  ASR_VAD_WAV_CHECK_MAX_LATENCY = 22,

  /**声学模型个数*/
  ASR_ENGINE_AUDIO_MODEL_COUNT_ID = 100,
  /**当前识别结果的起始时间*/
  ASR_ENGINE_UTTERANCE_START_TIME = 101,
  /**当前识别结果的结束时间*/
  ASR_ENGINE_UTTERANCE_STOP_TIME = 102,
  /**引擎授权状态*/
  ASR_ENGINE_AUTHORIZED_STATUS = 103,
  /**引擎授权过期时间*/
  ASR_ENGINE_AUTHORIZED_EXPIRY_TIME = 104,
  /**引擎授权包名*/
  ASR_ENGINE_AUTHORIZED_PACKAGE_NAME = 105,
  /**引擎个数*/
  ASR_ENGINE_COUNT_ID = 106,
  /**当前被激活的引擎的类型*/
  ASR_ENGINE_ACTIVE_ENGINE_ID = 107,
  /**切换引擎（STD or LP）*/
  ASR_ENGINE_SET_TYPE_ID = 108,
  /**重置前端 @deprecated*/
  ASR_ENGINE_SET_RESET_FRONTEND_ID = 109,
  /**引擎内部帧数偏移 @deprecated*/
  ASR_ENGINE_GET_FRAMEOFFSET_ID = 123,
  /**VAD状态*/
  ASR_RECORD_WAV_ID = 124,
  /**置信度允许头部语音（用于抑制离线引擎抢分）*/
  ASR_ENGINE_ALLOWHEADSPEECH_ID = 125,
  /**置信度允许尾部语音（用于抑制离线引擎抢分）*/
  ASR_ENGINE_ALLOWTAILSPEECH_ID = 126,
  /**设置计算NN是的bunchsize*/
  KWS_SET_BUNCH_FRAME_NUMBER = 127,
  /**设置Decoder使用的Token数量*/
  KWS_SET_TOKEN_NUMBER = 128,
  /**设置Decoder使用的LatNode数量*/
  KWS_SET_LATNODE_NUMBER = 129,
  /**设置LP引擎是否使用 RANK*/
  KWS_SET_CM_RANK = 130,
  /**设置LP引擎是否使用 出词惩罚策略*/
  KWS_SET_WORD_PEN = 131,
  /**设置LP引擎 silence 奖励的值，程序中真正使用的是这里设置 值 * 0.1 */
  KWS_SET_SIL_PEN = 132,
  /**设置LP 候选解码路径的方式*/
  KWS_SET_BEST_TOKEN = 133,
};

/**
 * @brief 引擎语种相关常量
 */
enum EngineLanguageId {
  /**语种切换ID(同时作用于引擎和动态语法编译)*/
  ASR_ENGINE_SWITCH_LANGUAGE = 800,
  /**中文普通话*/
  MANDARIN = 801,
  /**英语*/
  ENGLISH = 802,
  /**粤语*/
  CANTONESE = 803,
};

/**
 * @brief 包含的引擎类型
 */
enum EngineTypeId {
  /**只有LP引擎*/
  KWS_LP_ENGINE = 0,
  /**只有STD引擎*/
  KWS_STD_ENGINE = 1,
  /**同时包含LP和STD引擎*/
  KWS_STD_LP_ENGINE = 2,
};

/**
 * @brief Oneshot模式用于获取VAD信息的状态码
 */
enum EngineOneshotStatusCode {
  /**没有检测到新语音*/
  ASR_VAD_NOWAV = 3,
  /**检测到了新语音*/
  ASR_VAD_WAV_START = 4,
};

/**
 * @brief 前端相关设置
 */
enum EngineFrontendId {
  /**引擎前端采样率设置（KHz）
   * @note 仅支持8和16(默认)
   */
  ASR_SAMPLE_RATE_KHZ = 600,
  /**前端是否提取能量特征*/
  ASR_FRONTEND_EXTRACT_ENERGY = 601,
};

/**
 * @brief VAD功能相关选项
 */
enum EngineVadId {
  /**设置最短语音长度*/
  ASR_SET_VAD_MIN_SPEECH_LEN = 700,
  /**设置最长语音长度*/
  ASR_SET_VAD_MIN_SILENCE_LEN = 701,
  /**VAD功能开关*/
  ASR_ENGINE_ENABLE_VAD_ID = 702,
};

/**
 * @brief 引擎多线程相关选项
 * @todo 待补充详细说明
 */
enum EngineThreadId {
  ASR_SET_THREAD_NUMBER = 500,
  ASR_GET_THREAD_NUMBER = 501,
  ASR_SET_THREAD_INIT = 502,
  ASR_SET_THREAD_TOCPU0 = 510,
  ASR_SET_THREAD_TOCPU1 = 511,
  ASR_SET_THREAD_TOCPU2 = 512,
  ASR_SET_THREAD_TOCPU3 = 513,
  ASR_SET_THREAD_TOCPU4 = 514,
  ASR_SET_THREAD_TOCPU5 = 515,
  ASR_SET_THREAD_TOCPU6 = 516,
  ASR_SET_THREAD_TOCPU7 = 517,
  ASR_SET_THREAD_TOCPU8 = 518,
  ASR_SET_THREAD_TOCPU9 = 519,
  ASR_GET_THREAD0_TID = 520,
  ASR_GET_THREAD1_TID = 521,
  ASR_GET_THREAD2_TID = 522,
  ASR_GET_THREAD3_TID = 523,
  ASR_GET_THREAD4_TID = 524,
  ASR_GET_THREAD5_TID = 525,
  ASR_GET_THREAD6_TID = 526,
  ASR_GET_THREAD7_TID = 527,
  ASR_GET_THREAD8_TID = 528,
  ASR_GET_THREAD9_TID = 529,
};

/**
 * @brief 语法编译相关设置
 * @deprecated
 */
enum GrammarCompilerOptionId {
  /**设置单个字所允许的最大发音个数*/
  OPT_ID_GC_SET_MAX_PRONS_LIMIT = 1000,
  /**设置当超出最大发音个数时的处理方式*/
  OPT_ID_GC_SET_ACTION_WHEN_EXCEEDS_MAX_PRONS_LIMIT,
  /**获取编译语法过程种出现问题的词*/
  OPT_ID_GC_GET_PROBLEM_WORDS,
};

/**
 * @brief 语法编译状态码
 */
enum GrammarCompilerStatusCode {
  /**不支持的语法编译操作*/
  GC_OPTION_NOT_SUPPORT = -101,
  /**某些词的可能发音过多*/
  GC_HAS_WORDS_WITH_TOO_MUCH_PRONS = -102,
  /**语法编译失败*/
  GC_FATAL_ERROR = -1,
  /**语法编译成功*/
  GC_OK = 0,
};
// set nn bunch frame delay
enum {
  ASR_BUNCH_FRAME = 900,
};
#endif  // PUBLIC_OFA_CONSTS_H_
