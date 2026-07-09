import { getNativeFunction } from "juce-framework-frontend-mirror"
import type { BridgeFunctionName } from "./bridge.generated"
import { showErrorPopup } from "@/components/other/errorPopupWindow.vue"

//调用示例：
// const fetchSongs = async () => {
//   loading.value = true;
//   try {
//     // 2. 必须加 await，并指定返回类型
//     const data = await callJuceFunc<SongInfo[]>('getSongList', { page: 1 });
//     songs.value = data; // data 是真实的 SongInfo[] 数据
//   } catch (error) {
//     // 3. 捕获错误（虽然 callJuceFunc 内部已弹窗，但这里用于重置状态）
//     console.log('请求失败，已由全局弹窗提示');
//   } finally {
//     loading.value = false;
//   }
// };
 export async function callJuceFunc<T = unknown>(name: BridgeFunctionName, ...args: unknown[]): Promise<T> {
  try {
    const results = await getNativeFunction(name)(...args)//如果没有三个点只会传进来第一个数
    if (results && typeof results === 'object' && '__error' in results) {
      throw new Error(String(results.__error)) //如果throw了会直接跳转到catch块中，throw也有返回的属性
    }
    return results as T
  } catch (__error) {
    const err = __error instanceof Error ? __error : new Error(String(__error))
    showErrorPopup(err)
    console.error(err.message)
    throw err
  }
}