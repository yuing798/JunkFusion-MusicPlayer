// stores/playBackStore.ts
import { defineStore } from 'pinia';

export const usePlayBackStore = defineStore('playBack', {
  state: () => ({
    /** 当前正在播放的歌曲 ID，null 表示未播放任何歌曲 */
    currentSongId: null as number | null,
    /** 是否正在播放（true 表示播放中，false 表示暂停） */
    isPlaying: false,
    // 这首歌曲有多长
    timeLength : null as number|null,
    //当前播放到哪里了
    currentTimeStamp : null as number|null,
  }),

  actions: {
    //当当前播放歌曲发生改变的时候，及时存储
    setCurrentSongId(songId:number){
      this.currentSongId=songId;
      localStorage.setItem("playbar_currentSongId",String(songId));
    },
    restoreCurrentSongId(){
      const songId = localStorage.getItem("playbar_currentSongId");
      if(songId) this.currentSongId = Number(songId);
    }
  },
});