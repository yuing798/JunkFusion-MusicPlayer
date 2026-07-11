
export interface SongInfo {

  /** 数据库主键，自增 ID，C++ 端为 int64_t */
  songId: number

  /** 文件完整路径 */
  filePath: string

  /** 文件大小（字节），C++ 端为 int64_t */
  fileSize: number

  /** 文件最后一次修改时间 */
  lastModifiedTime: string

  /** 歌曲时长（秒） */
  duration: number

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

  imageHash?: string

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

  /**
   * 优先级排序后的位置
   *
   * 由 C++ 端使用 ICU Collator 按 title 拼音排序后分配，
   * 每次插入/更新歌曲后自动重建。
   * 逻辑上不会因其他操作而改变。
   */
  nameId: number
}
