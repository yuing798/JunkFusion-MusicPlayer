// UI/scripts/generate-bridge.js
const fs = require('fs');//node.js的File System，文件系统模块
const path = require('path');//node.js的专门用来处理文件和目录的路径
//require是node.js的专属语法

// 1. 读取定义文件
const defsPath = path.resolve(__dirname, 'bridgeDefs.json');
//__dirname：这是 Node.js 里的一个全局变量，代表当前正在执行的这个 JS 文件所在的文件夹路径
// path.resolve()：把路径片段拼接在一起，并计算出一个绝对路径。

const defs = JSON.parse(fs.readFileSync(defsPath, 'utf-8'));
//把那个 JSON 文件的内容读出来，并转换成 JS 对象。

// 2. 生成 TypeScript 文件 (src/types/bridge.generated.ts)
const tsOutputPath = path.resolve(__dirname, 'src/bridge.generated.ts');//vue端的桥接函数名称
// ./ 表示“当前目录”，../ 表示“上一级目录（父目录）”
const tsLines = []
tsLines.push(`// ⚠️ This file is AUTO-GENERATED. DO NOT EDIT MANUALLY.`);
// const：声明一个“不可变的绑定”。变量一旦赋值，就不能再指向新的值（不能重新赋值）。
// let：声明一个“可变的绑定”。变量赋值后，还可以随时指向新的值（可以重新赋值）。

tsLines.push(`// Generated from bridgeDefs.json\n`)
tsLines.push(`export const BRIDGE_KEYS = {`)
for (const [key, value] of Object.entries(defs)) {
  tsLines.push(`  ${key}: '${value}' as const,`)
}

//Object.entries(defs)是 JavaScript 内置方法，用来把一个对象，转换成一个键值对数组，方便你用 for...of 循环去遍历。

// 假设你的 bridgeDefs.json 里的 functions 长这样：

// json
// "functions": {
//   "playAudio": "play_audio_cpp",
//   "stopAudio": "stop_audio_cpp",
//   "getVolume": "get_volume_cpp"
// }
// 那么 Object.entries(defs.functions) 就会返回这个二维数组：

// javascript
// [
//   ['playAudio', 'play_audio_cpp'],
//   ['stopAudio', 'stop_audio_cpp'],
//   ['getVolume', 'get_volume_cpp']
// ]
// 接着你用 for (const [key, value] of ...) 循环，其实就是数组解构，每一次循环把 ['playAudio', 'play_audio_cpp'] 拆开，
// 让 key = 'playAudio'，value = 'play_audio_cpp'。这样你就能在模板字符串里动态插入它们了。

// 分号（;）结束的是“语句”（比如 let a = 1;）。这里的 tsContent += ... 是语句，末尾有分号没问题。

// 你问的逗号 , 是写在模板字符串里面的，它最终会变成生成文件里的对象属性分隔符。

// 因为你要生成的是这段文本：

// typescript
// export const BRIDGE_KEYS = {
//   playAudio: 'play_audio_cpp',
//   stopAudio: 'stop_audio_cpp',
//   getVolume: 'get_volume_cpp',
// };
// 在 JS/TS 的对象 {} 里，每个属性之间必须用逗号（,）隔开，否则语法报错。

tsLines.push(`} as const;\n`)
tsLines.push(`export type BridgeFunctionName = typeof BRIDGE_KEYS[keyof typeof BRIDGE_KEYS];`);

const tsContent = tsLines.join('\n')
fs.writeFileSync(tsOutputPath, tsContent);

// 3. 生成 C++ 头文件 (BridgeNames.h) 
//    注意：这个文件最终需要被你的 C++ 项目包含。
const cppOutputPath = path.resolve(__dirname, '../Utils/BridgeNames.h');
const cppLines = []
cppLines.push(`// ⚠️ This file is AUTO-GENERATED. DO NOT EDIT MANUALLY.`);
cppLines.push(`// Generated from bridge-defs.json\n`)
cppLines.push(`#pragma once\n`)
cppLines.push(`namespace BridgeKeys {`)
for (const [key, value] of Object.entries(defs)) {
  cppLines.push(`    inline constexpr const char* ${key} = "${value}";`)
}
cppLines.push(`} // namespace BridgeKeys`)
const cppContent = cppLines.join('\n')
fs.writeFileSync(cppOutputPath, cppContent);

console.log('✅ Bridge files generated successfully!');
console.log(`   - TS: ${tsOutputPath}`);
console.log(`   - C++: ${cppOutputPath}`);