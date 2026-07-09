import { defineStore } from 'pinia'
import type { SongInfo } from './SongInfo'
import { callJuceFunc } from '@/bridge/bridgeSupport'
import { BRIDGE_KEYS } from '@/bridge/bridge.generated'
import { showErrorPopup } from '@/components/other/errorPopupWindow.vue'

// “对象字面量”就是用一对花括号 {} 包裹起来，里面写 键: 值 键值对（Key-Value Pair）的结构，用来直接在代码里“凭空”创建一个 JavaScript 对象。
export const songStore = defineStore('songPage',{//这一页的15首歌曲
  state:()=>({
    //箭头后面跟着圆括号的原因：state 必须是一个“函数”，而箭头函数 () => 后面如果直接跟 {}，
    // 会被 JavaScript 解析为“函数体（代码块）”，而不是“对象字面量”。为了让 JS 知道你想返回一个对象，必须用 () 把对象字面量包起来。
    //“对象字面量”就是直接用 {} 把 键: 值 写出来，来“凭空”创建一个 JavaScript 对象。
    songs:[] as SongInfo[],//当前展示的歌曲信息
  }),

  actions:{//里面放置会修改数据的操作
    async toggleMyLike(songId:number){
      const index = this.songs.findIndex(s=>s.songId === songId)//遍历数组，找到第一个满足条件的元素，并返回它的位置（索引）。
      if(index === -1) return//找不到就滚蛋
      const originSong = this.songs[index]!//保存原始数据，方便回滚，感叹号表示该数据一定不是undefined
      this.songs[index] = {
        ...originSong,
        isMyLike:!originSong.isMyLike,
      }//乐观更新
      try{
        await callJuceFunc<void>(BRIDGE_KEYS.toggleMyLike,songId)
      }catch(error){
        console.error('点赞失败，回滚状态')
        this.songs[index] = originSong
      }
    }
  }
}
  
)
// getters 里写“只读的计算逻辑”（基于 state 算新数据），actions 里写“会改数据的操作”（包括异步请求、修改 state、调用桥接）