
export enum PlaybackState {
  // 当前为空闲状态（即播放栏为空，或没有歌曲被选中）
  Stopped = 'stopped',//没有对应图片
  // 在暂停状态
  Paused = 'paused',//对应UI/src/assets/image/pause.svg图片
  // 在播放演奏状态
  Playing = 'playing'//对应UI/src/assets/image/play.svg图片
}