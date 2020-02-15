/**
 * @file ual_ofa.h
 * @copyright 2019, Unisound. All rights reserved.
 * @brief KWS Engine C/C++ API Header
 */
#ifndef PUBLIC_UAL_OFA_H_
#define PUBLIC_UAL_OFA_H_
#include <stdint.h>
typedef void* HANDLE;
#ifndef _WIN32
#define KWS_EXPORT __attribute__((visibility("default")))
#define FOR_C_APP
#ifdef __cplusplus
#ifdef FOR_C_APP
extern "C" {
#else   // FOR_C_APP
namespace UniASR {
#endif  // FOR_C_APP
#endif  // __cplusplus
#else   // _WIN32
#ifdef DLL_EXPORT
#define KWS_EXPORT extern "C" __declspec(dllexport)
#else
#define KWS_EXPORT extern "C" __declspec(dllimport)
#endif
#endif

/**
 * @brief 获取引擎类型
 *
 * @return #EngineTypeId
 *    @retval #KWS_LP_ENGINE 只有LP引擎
 *    @retval #KWS_STD_ENGINE 只有STD引擎
 *    @retval #KWS_STD_LP_ENGINE 同时包含STD和LP引擎
 */
KWS_EXPORT int UalOFAGetEngineType();

/**
 * @brief 获得版本信息(引擎库版本和构建版本)
 *
 * @return 描述版本信息的字符串
 */
KWS_EXPORT const char* UalOFAGetVersion();

/**
 * @brief 创建并从文件初始化 KWS STD 引擎
 *
 * @param path_of_am 放置模型文件（asrfix.dat）的目录
 * @param path_of_grammar 语法文件的路径
 *                        如果有多个语法文件，用';'分开。
 *
 * @return #EngineStatusCode
 *
 *
 * @note 不同语法文件的 tag 必须唯一。
 *       假如有两个相同的 tag 的语法，后面加载的语法会把前面的语法覆盖掉
 *
 * @attention 仅适用于STD引擎
 */
KWS_EXPORT int UalOFAInitialize(const char* path_of_am,
                                const char* path_of_grammar);

/**
 * @brief 创建并从内存初始化 KWS LP 引擎
 *
 * @param am_buffer LP模型在内存中的起始地址
 * @param grammar_buffer LP语法在内存中的起始地址
 *
 * @return #EngineStatusCode
 *
 * @attention 仅适用于LP引擎
 */
KWS_EXPORT int UalOFAInitializeFromBuffer(const char* am_buffer,
                                          const char* grammar_buffer);

/**
 * @brief 释放所有引擎（STD和LP），回收内存。
 * @details 释放后，除非重新初始化，否则引擎将无法继续使用
 *
 * @return void
 */
KWS_EXPORT void UalOFARelease();

/**
 * @brief 重置当前引擎为初始状态
 *
 * @return #EngineStatusCode
 */
KWS_EXPORT int UalOFAReset();

/**
 * @brief 使用当前引擎开始识别
 *
 * @param grammar_tag 用到的识别语法的tag
                      如果要使用多个语法，用';'分割每个tag
 * @param am_index 声学模型id，具体数值请咨询模型提供方（LP引擎不生效）
 *
 * @return #EngineStatusCode
 *
 * @note 只能传入一个语种的多个语法文件的tag，
 *       不能同时传入多个语种的语法文件的tag
 */
KWS_EXPORT int UalOFAStart(const char* grammar_tag, int am_index);

/**
 * @brief 停止当前引擎的识别
 * @details 如果在此之前，引擎没有返回有效的结果，
 *         那么在调用此API后，要调用UalOFAGetResult去尝试拿下结果
 *
 * @return #EngineStatusCode
 */
KWS_EXPORT int UalOFAStop();

/**
 * @brief 输入数据给当前处于激活状态的引擎
 *
 * @param raw_audio 音频数据
 * @param len 音频数据的长度（单位：Bytes），不要超过32000
 *
 * @return #EngineStatusCode
 *    @retval #ASR_RECOGNIZER_PARTIAL_RESULT 调用 #UalOFAGetResult 获取识别结果
 */
KWS_EXPORT int UalOFARecognize(signed char* raw_audio, int len);

/**
 * @brief 获取处于激活状态的引擎的识别结果
 *
 * @return 识别结果（C语言字符串）
 *    @retval NULL 如果引擎没有识别结果
 */
KWS_EXPORT char* UalOFAGetResult();

/**
 * @brief 设置处于激活状态的引擎的属性值
 *
 * @param id 属性id
 * @param value 属性值
 *
 * @return #EngineStatusCode
 *
 * @note 更多细节请参考 wiki 或者 @ref ofa_consts.h
 */
KWS_EXPORT int UalOFASetOptionInt(int id, int value);

/**
 * @brief 获取处于激活状态的引擎的属性值
 *
 * @param id 属性id
 *
 * @return 属性值
 *    @retval -65535 不支持该属性
 *
 * @note 更多细节请参考 wiki 或者 @ref ofa_consts.h
 */
KWS_EXPORT int UalOFAGetOptionInt(int id);

/**
 * @brief 取消当前的识别进程
 *
 * @return #EngineStatusCode
 *
 * @attention 仅适用于STD引擎
 */
KWS_EXPORT int UalOFACancel();

/**
 * @brief 引擎是否处于空闲状态
 *
 * @return status
 *    @retval 0 引擎正忙
 *    @retval 1 引擎空闲
 *
 * @attention 仅适用于STD引擎
 */
KWS_EXPORT int UalOFAIsEngineIdle();

/**
 * @brief 判断语音是否结束
 *
 * @return #EngineOneshotStatusCode
 *
 * @note 当唤醒之后，sdk调用该函数来判断是否进入one-shot模式
 *
 * @attention 仅适启用加密的STD引擎
 */
KWS_EXPORT int UalOFACheckWavEnd();

/**
 * @brief 获取某个位置处的 VAD 状态
 *
 * @return VAD状态
 *    @retval 0  表示没有 VAD 状态输出
 *    @retval >0 返回值中包含三个信息：
 *               0-55位的含义是当前获取的VAD状态在时间轴上的位置,
 *               56-57位的含义是VAD state,
 *               58-61位是已经计算出，但是没有get到的 VAD 状态的数量
 *
 * @note VAD 状态有四种:
 *       0: 静音
 *       1: 语音开始
 *       2: 语音
 *       3: 语音结束
 * @note 时间轴上的位置是指
 *       相对于最近一次调用UalOFAStart的时间偏移
 *       单位: 帧(10ms)
 *
 * @attention 仅适用于STD引擎
 */
KWS_EXPORT int64_t UalOFAGetVadState();

/**
 * @brief 获取声学模型ID
 *
 * @param am_ith 声学模型序号（例如0, 1, 2）
 *
 * @return 声学模型ID
 *
 * @attention 仅适用于STD引擎
 */
KWS_EXPORT int UalOFAGetAMID(int am_ith);
/**
 * @brief 重新加载语法
 *
 * @param grammar_path 语法的文件路径
 *
 * @return #GrammarCompilerStatusCode
 *
 * @attention 仅适用于STD引擎
 */
KWS_EXPORT int UalOFAReLoadGrammar(const char* grammar_path);

/**
 * @brief 从内存种中加载语法
 *
 * @param grammar_str 语法文件的内容
 *
 * @return #GrammarCompilerStatusCode
 *
 * @attention 仅适用于STD引擎
 */
KWS_EXPORT int UalOFALoadGrammarStr(const char* grammar_str);

/**
 * @brief 卸载指定tag的语法
 *
 * @param grammar_tag 语法的tag，如wakeup, ivm等
 *
 * @return #GrammarCompilerStatusCode
 *
 * @attention 仅适用于STD引擎
 */
KWS_EXPORT int UalOFAUnLoadGrammar(const char* grammar_tag);

/**
 * @brief 初始化语法编译器
 *
 * @param modeldir 放置模型(asrfix.dat)的目录
 *
 * @return 语法编译器句柄
 *    @retval NULL 初始化失败
 *
 * @attention 仅适用于添加了动态编语法功能的STD引擎
 */
KWS_EXPORT HANDLE UalOFAInitializeUserDataCompiler(const char* modeldir);

/**
 * @brief 动态编译语法, 并写入文件（需要预先加载jsgf_clg.dat）
 *
 * @param handle 语法编译器句柄
 * @param jsgf jsgf的文本内容
 * @param user 用户词表内容
 * @param out_file 输出语法文件
 *
 * @return #GrammarCompilerStatusCode
 *
 * @note 如果jsgf里的tag是wakeup的话，会将传入的 jsgf 和 vocab 编译成一个语法
 *       否则，会把预加载的jsgf_clg.dat和vocab编译成一个语法文件
 *
 * @attention 仅适用于添加了动态编语法功能的STD引擎
 */
KWS_EXPORT int UalOFACompileUserData(HANDLE handle, const char* jsgf,
                                     const char* user, const char* out_file);

/**
 * @brief 仅根据jsgf和用户词表编译语法，并写入文件（grammar.dat和jsgf_clg.dat）
 *
 * @param handle 语法编译器句柄
 * @param jsgf jsgf的文本内容
 * @param vocab 用户词表内容
 * @param out_dir 输出语法文件的目录名
 *
 * @return #GrammarCompilerStatusCode
 *
 * @attention 仅适用于添加了动态编语法功能的STD引擎
 */
KWS_EXPORT int UalOFACompileGrammar(HANDLE handle, const char* jsgf,
                                    const char* vocab, const char* out_dir);

/**
 * @brief 编译指定发音的词条成语法，并写入文件
 *
 * @param handle 语法编译器句柄
 * @param user_data 指定发音的语法
 * @param out_grammar_file 输出语法文件的文件名，包含目录
 *
 * @return #GrammarCompilerStatusCode
 *
 * @attention 仅适用于添加了动态编语法功能的STD引擎
 */
KWS_EXPORT int UalOFACompileUserSpecificPronun(HANDLE handle,
                                               const char* user_data,
                                               const char* out_grammar_file);
/**
 * @brief slot by slot动态编译语法
 * @details 对于之前编译过的语法，如果只是其中一个slot的
 *         内容发生变化，可以通过这个接口编译，只替换发生变化的slot内容
 *
 * @param handle 语法编译器句柄
 * @param in_partial_data_path 部分语法文件路径
 * @param jsgf jsgf的文本内容
 * @param vocab 用户词表内容
 * @param out_dir 输出用于识别的语法文件
 * @param out_partial_data_path 输出的新生成的部分语法文件
 *
 * @return #GrammarCompilerStatusCode
 *
 * @note in_partial_data_path 第一次使用时，该文件不存在，但也要提供完整的路径
 *
 * @attention 仅适用于添加了动态编语法功能的STD引擎
 */
KWS_EXPORT int UalOFAPartialCompileUserData(HANDLE handle,
                                            const char* in_partial_data_path,
                                            const char* jsgf, const char* vocab,
                                            const char* out_dir,
                                            const char* out_partial_data_path);

/**
 * @brief 删除语法编译器并释放资源
 *
 * @param handle 语法编译器句柄
 *
 * @return void
 *
 * @attention 仅适用于添加了动态编语法功能的STD引擎
 */
KWS_EXPORT void UalOFAReleaseUserDataCompiler(HANDLE handle);

/**
 * @brief 装载预先编译好的jsgf_clg.dat
 *
 * @param handle 语法编译器句柄
 * @param path jsgf_clg.dat文件路径
 *
 * @return #GrammarCompilerStatusCode
 *
 * @attention 仅适用于添加了动态编语法功能的STD引擎
 */
KWS_EXPORT int UalOFALoadCompiledJsgf(HANDLE handle, const char* path);

/**
 * @brief 编译动态slot的用户词表数据
 * @details 这一部分数据通常较少，可以快速的编译，
 *         编译词表后，将自动将编译好的语法加载到的引擎里
 *
 * @param handle 语法编译器句柄
 * @param user_data 用户slot词表数据
 * @param grammar_tag 语法的tag(domain)
 *
 * @return #GrammarCompilerStatusCode
 *
 * @attention 仅适用于添加了动态编语法功能的STD引擎
 */
KWS_EXPORT int UalOFACompileDynamicUserData(HANDLE handle,
                                            const char* user_data,
                                            const char* grammar_tag);

/**
 * @brief 设置语法编译整数类型的选项
 *
 * @param handle 语法编译器句柄
 * @param opt_id 选项ID
 * @param opt_value 选项的值
 *
 * @return #GrammarCompilerStatusCode
 *
 * @attention 仅适用于添加了动态编语法功能的STD引擎
 */
KWS_EXPORT int UalOFAGrammarCompilerSetOptionInt(HANDLE handle, int opt_id,
                                                 int opt_value);

/**
 * @brief 获取语法编译字符串类型选项的值
 *
 * @param handle 语法编译器句柄
 * @param opt_id 选项ID
 *
 * @return 该选项的值
 *    @retval NULL 该选项不存在
 *
 * @attention 仅适用于添加了动态编语法功能的STD引擎
 */
KWS_EXPORT const char* UalOFAGrammarCompilerGetOptionString(HANDLE handle,
                                                            int opt_id);

/**
 * @brief 输入密码供引擎验证
 *
 * @param env 密码字符串(C-string)
 *
 * @return #EngineStatusCode
 *    @retval #ASR_FATAL_ERROR 验证失败
 *    @retval #ASR_RECOGNIZER_OK 验证成功
 *
 * @attention 仅适用于启用加密的STD引擎
 */
KWS_EXPORT int UalOFASetEnv(void* env);

/**
 * @brief 获取引擎加密方式
 *
 * @return EncryptionValue
 *    @retval 0 没有任何加密
 *    @retval 第1个bit位为1 过期时间限制(过期之后，引擎无法启动)
 *    @retval 第2个bit位为1 秘钥加密
 *    @retval 第3个bit位为1 过了用户指定时间之后，引擎性能严重下降
 *    @retval 第4个bit位为1 包名限制
 *
 * @attention 仅适用于STD引擎
 */
KWS_EXPORT int UalOFAGetEncryptionScheme();

#ifndef _WIN32
#ifdef __cplusplus
}
#endif  // __cplusplus
#endif  // _WIN32
#endif  // PUBLIC_UAL_OFA_H_
