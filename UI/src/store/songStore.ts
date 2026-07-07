import { defineStore } from 'pinia'
import type { SongInfo } from './SongInfo'
import { PlaybackState } from '@/macro/playState'

const songPage : string = 'songPage'
// “对象字面量”就是用一对花括号 {} 包裹起来，里面写 键: 值 键值对（Key-Value Pair）的结构，用来直接在代码里“凭空”创建一个 JavaScript 对象。
export const songStore = defineStore(songPage,{//这一页的15首歌曲
  state:()=>({
    //箭头后面跟着圆括号的原因：state 必须是一个“函数”，而箭头函数 () => 后面如果直接跟 {}，
    // 会被 JavaScript 解析为“函数体（代码块）”，而不是“对象字面量”。为了让 JS 知道你想返回一个对象，必须用 () 把对象字面量包起来。
    //“对象字面量”就是直接用 {} 把 键: 值 写出来，来“凭空”创建一个 JavaScript 对象。
    songs:[] as SongInfo[],//当前展示的歌曲信息
  }),

  actions:{//里面放置会修改数据的操作
    async toggleMyLike(songId:number){
      
    }
  }
}
  
)
// getters 里写“只读的计算逻辑”（基于 state 算新数据），actions 里写“会改数据的操作”（包括异步请求、修改 state、调用桥接）