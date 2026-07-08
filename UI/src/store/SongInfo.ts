/**
 * SongInfo — 歌曲完整信息接口
 *
 * 与 C++ fileManage/fileMessage.hpp 中的 SongInfo 结构体一一对应。
 * C++ 端通过 SongInfo::toVar() 序列化为 juce::var（juce::DynamicObject），
 * JS 端通过 JUCE bridge 接收到的对象与此接口完全匹配。
 *
 * 字段分为 6 组：文件信息、标签信息、FFmpeg 解码层、AI 分析、用户信息、排序
 */

export interface SongInfo {
  // ════════════════════════════════════════════════════════════════
  // 0. 主键
  // ════════════════════════════════════════════════════════════════

  /** 数据库主键，自增 ID，C++ 端为 int64_t */
  songId: number

  // ════════════════════════════════════════════════════════════════
  // 1. 文件信息
  // ════════════════════════════════════════════════════════════════

  /** 文件完整路径 */
  filePath: string

  /** 文件大小（字节），C++ 端为 int64_t */
  fileSize: number

  /** 文件最后一次修改时间 */
  lastModifiedTime: string

  /** 歌曲时长（秒） */
  duration: number

  // ════════════════════════════════════════════════════════════════
  // 2. 标签信息（来自文件容器元数据）—— 可能缺失
  // ════════════════════════════════════════════════════════════════

  /**
   * 歌曲标题
   *
   * 在 FFmpeg 提取阶段已做兜底：
   * 如果文件中无 title 标签，则使用文件 stem 名称（无扩展名）。
   * 因此该字段一定存在，不需要 optional。
   */
  title: string

  /** 艺术家名称，C++ 端为 std::optional */
  artist?: string

  /** 专辑名称，C++ 端为 std::optional */
  album?: string

  /** 专辑艺术家，C++ 端为 std::optional */
  albumArtist?: string

  /** 体裁，C++ 端为 std::optional */
  genre?: string

  /** 轨道号，C++ 端为 std::optional（undefined 表示不存在） */
  trackNumber?: number

  /** 碟片号，C++ 端为 std::optional（undefined 表示不存在） */
  discNumber?: number

  /** 发行年份，C++ 端为 std::optional（undefined 表示不存在） */
  year?: number

  /** 作曲者，C++ 端为 std::optional */
  composer?: string

  /**
   * 专辑封面图片的哈希值，C++ 端为 std::optional
   *
   * 图片文件命名规则：{imageHash}.jpg
   * 存储在 C++ 端 imageDirId 目录下。
   * 获取封面时，通过桥接函数将 imageHash 发送给 C++ 端，
   * 由 C++ 拼接完整路径后返回。
   */
  imageHash?: string

  // ════════════════════════════════════════════════════════════════
  // 3. FFmpeg 解码层信息
  // ════════════════════════════════════════════════════════════════

  /** 是否包含多路音频流（如多语言、多声道） */
  isMultiStreamFile: boolean

  /** 比特率（kbps），C++ 端为 int64_t */
  bitRate: number

  /** 采样率（Hz） */
  sampleRate: number

  /** 通道数 */
  numChannels: number

  /** 位深 */
  bitDepth: number

  /** 编码器名称，C++ 端为 std::optional */
  codecName?: string

  // ════════════════════════════════════════════════════════════════
  // 4. AI 分析信息 —— 未分析时字段为 undefined
  // ════════════════════════════════════════════════════════════════

  /** 是否被检测为音乐资源 */
  isMusic: boolean

  /** AI 分析体裁，C++ 端为 std::optional */
  aiGenre?: string

  /** 节拍数（BPM），C++ 端为 std::optional（undefined 表示未知） */
  bpm?: number

  /** 调性（如 "C major", "A minor"），C++ 端为 std::optional */
  key?: string

  /** 是否已经进行过 AI 处理 */
  aiProcessed: boolean

  // ════════════════════════════════════════════════════════════════
  // 5. 用户信息
  // ════════════════════════════════════════════════════════════════

  /** 是否添加到"我喜欢"列表 */
  isMyLike: boolean

  /** 用户备注，C++ 端为 std::optional */
  comment?: string

  /** 已经播放了多少次 */
  hadPlayedNum: number

  // ════════════════════════════════════════════════════════════════
  // 6. 排序字段
  // ════════════════════════════════════════════════════════════════

  /**
   * 优先级排序后的位置
   *
   * 由 C++ 端使用 ICU Collator 按 title 拼音排序后分配，
   * 每次插入/更新歌曲后自动重建。
   * 逻辑上不会因其他操作而改变。
   */
  nameId: number
}
