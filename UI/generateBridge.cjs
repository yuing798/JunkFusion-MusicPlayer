// UI/scripts/generate-bridge.js
const fs = require('fs');//node.js的File System，文件系统模块
const path = require('path');//node.js的专门用来处理文件和目录的路径
const jsonc = require('jsonc-parser');//用来解析jsonc
//require是node.js的专属语法

// 读取定义文件
const defsPath = path.resolve(__dirname, 'bridgeDefs.jsonc');
// path.resolve()：把路径片段拼接在一起，并计算出一个绝对路径。

const defs = jsonc.parse(fs.readFileSync(defsPath, 'utf-8'));

//生成 TypeScript 文件 (UI/src/bridge/bridge.generated.ts)
const tsOutputPath = path.resolve(__dirname, 'src/bridge/bridge.generated.ts');//vue端的桥接函数名称
// ./ 表示“当前目录”，../ 表示“上一级目录（父目录）”
const tsLines = []
tsLines.push(`// ⚠️ This file is AUTO-GENERATED. DO NOT EDIT MANUALLY.`);

tsLines.push(`// Generated from bridgeDefs.json\n`)

// 处理 function：每个函数生成一个独立常量
for (const funcName of Object.keys(defs.function)) {
  tsLines.push(`export const BRIDGE_${funcName} = {`);
  const funcObj = defs.function[funcName];
  for (const key of Object.keys(funcObj)) {
    tsLines.push(`  ${key}: '${funcObj[key]}' as const,`);
  }
  tsLines.push(`} as const`);
}

// 处理 event：所有事件合并为一个对象
tsLines.push(`export const EVENT_BRIDGE_KEYS = {`);
for (const eventName of Object.keys(defs.event)) {
  tsLines.push(`  ${eventName}: '${eventName}' as const,`);
}
tsLines.push(`} as const`);

const tsContent = tsLines.join('\n')
fs.writeFileSync(tsOutputPath, tsContent);

// 3. 生成 C++ 头文件 (BridgeNames.h) 
//    注意：这个文件最终需要被你的 C++ 项目包含。
const hppOutputPath = path.resolve(__dirname, '../Utils/BridgeNames.h');
const hppLines = []
hppLines.push('//warning:this file will be generated auto,dont modify it by yourself\n');
hppLines.push('#pragma once')

// 遍历 function，为每个函数生成一个 struct
for (const funcName of Object.keys(defs.function)) {
    hppLines.push(`struct BRIDGE_${funcName} {`);
    const funcObj = defs.function[funcName];
    for (const key of Object.keys(funcObj)) {
        // 假设值都是字符串，直接生成 const char*
        hppLines.push(`    static constexpr const char* ${key} = "${funcObj[key]}";`);
    }
    hppLines.push('};');
    hppLines.push('');  // 空行分隔
}

// 遍历 event，生成 EVENT_BRIDGE_KEYS 结构体
hppLines.push('struct EVENT_BRIDGE_KEYS {');
for (const eventName of Object.keys(defs.event)) {
    hppLines.push(`    static constexpr const char* ${eventName} = "${eventName}";`);
}
hppLines.push('};');
const hppContent = hppLines.join('\n')
fs.writeFileSync(hppOutputPath, hppContent);

console.log('✅ Bridge files generated successfully!');
console.log(`   - TS: ${tsOutputPath}`);
console.log(`   - C++: ${hppOutputPath}`);