import { getNativeFunction } from "juce-framework-frontend-mirror"

import { showErrorPopup } from "@/components/other/errorPopupWindow.vue"
import { EVENT_BRIDGE_KEYS } from "./bridge.generated"

//调用示例：
// const fetchSongs = async () => {
//   loading.value = true;
//   try {
//     // 2. 必须加 await，并指定返回类型
//     const data = await callJuceFunc<SongInfo[]>('getSongList', { page: 1 });
//     songs.value = data; // data 是真实的 SongInfo[] 数据
//   } catch (error) {
//     // 3. 捕获错误（虽然 callJuceFunc 内部已弹窗，但这里用于重置状态,防止继续向上冒泡）
//     console.log('请求失败，已由全局弹窗提示');
//   } finally {
//     loading.value = false;
//   }
// };
 export async function callJuceFunc<T = unknown>(name: string, ...args: unknown[]): Promise<T> {
  try {
    const results = await getNativeFunction(name)(...args)//如果没有三个点只会传进来第一个数
    if (results && typeof results === 'object') {
      if(EVENT_BRIDGE_KEYS.fullError in results){
        throw new Error(String(results[EVENT_BRIDGE_KEYS.fullError])) 
      }else if(EVENT_BRIDGE_KEYS.partError in results){
        
      }else if(EVENT_BRIDGE_KEYS.fullSuccess in results){

      }
    }
    return results as T
  } catch (error) {
    const err = error instanceof Error ? error : new Error(String(error))
    showErrorPopup(err)
    console.error(err.message)
    throw err
  }
}