// UI/scripdart/generate-bridge.js
const fs = require('fs');//node.js的File System，文件系统模块
const path = require('path');//node.js的专门用来处理文件和目录的路径
const jsonc = require('jsonc-parser');//用来解析jsonc
//require是node.js的专属语法

// 读取定义文件
const defsPath = path.resolve(__dirname, 'bridgeDefs.jsonc');
// path.resolve()：把路径片段拼接在一起，并计算出一个绝对路径。

const defs = jsonc.parse(fs.readFileSync(defsPath, 'utf-8'));

//生成 TypeScript 文件 (UI/src/bridge/bridge.generated.dart)
const dartOutputPath = path.resolve(__dirname, '../junk_fusion_ui/lib/bridge/dll/dllBridgeName.dart');//vue端的桥接函数名称
// ./ 表示“当前目录”，../ 表示“上一级目录（父目录）”
const dartLines = []
dartLines.push(`// ⚠️ This file is AUTO-GENERATED. DO NOT EDIT MANUALLY.`);

dartLines.push(`// Generated from bridgeDefs.json\n`)

for(const key of Object.keys(defs)){
  dartLines.push(`class B_${key}{`);
  dartLines.push(`  static const String name = '${key}';`);
  const obj = defs[key];
  for (const name of Object.keys(obj)) {
    if(obj[name] === ""){
      dartLines.push(`  static const String ${name} = '${name}';`);//空的话直接把名称赋给value
    }else{
      dartLines.push(`  static const String ${name} = '${obj[name]}';`);
    }
    
  }
  dartLines.push(`}\n`);
}

const dartContent = dartLines.join('\n')
fs.writeFileSync(dartOutputPath, dartContent);

// 3. 生成 C++ 头文件 (BridgeNames.h) 
//    注意：这个文件最终需要被 C++ 项目包含。
const hppOutputPath = path.resolve(__dirname, 'dllBridgeName.hpp');
const hppLines = []
hppLines.push('//warning:this file will be generated auto,dont modify it by yourself\n');
hppLines.push('#pragma once')

for(const key of Object.keys(defs)){
  hppLines.push(`struct B_${key}{`);
  hppLines.push(`    static constexpr const char* name = "${key}" ;`);
  const obj = defs[key];
  for (const name of Object.keys(obj)) {
    if(obj[name] === ''){
      hppLines.push(`    static constexpr const char* ${name} = "${name}" ;`);//空的话直接把名称赋给value
    }else{
      hppLines.push(`    static constexpr const char* ${name} = "${obj[name]}" ;`);
    }
  }
  hppLines.push(`};\n`);
}

const hppContent = hppLines.join('\n')
fs.writeFileSync(hppOutputPath, hppContent);

console.log('✅ Bridge files generated successfully!');
console.log(`   - dart: ${dartOutputPath}`);
console.log(`   - C++: ${hppOutputPath}`);

//node D:/audio_develop/Junk-Fusion/JunkFusionDLL/generateBridge.cjs执行