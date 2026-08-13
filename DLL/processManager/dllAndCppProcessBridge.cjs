//这个文件用来生成dll后后端进程链接的桥接定义

const fs = require('fs');
const path = require('path');
const jsonc = require('jsonc-parser');

//这个放在对应的进程端
const otherDllAndCppBridgesPaths = ["../../JAudioProcess/AudioDefs.hpp"];//其他生成文件的格式和上面那两个不同，所以分开
const otherJsoncFilesPaths = ["../../JAudioProcess/AudioProcessAndDllBridge.jsonc"];

//这个放在DLL端
const dllBridgeHppPaths = ["AudioDefs.hpp"];
const hppPaths = [otherDllAndCppBridgesPaths,dllBridgeHppPaths];


for(column=0;column<dllBridgeHppPaths.length;column++){//文件索引
  // 读取定义文件
  const jsoncPath = path.resolve(__dirname, otherJsoncFilesPaths[column]);
  const jsonPrase = jsonc.parse(fs.readFileSync(jsoncPath, 'utf-8'));

  for(row=0;row<2;row++){
    const hppOutputPath = path.resolve(__dirname, hppPaths[row][column]);
    const hppLines = []
    hppLines.push('//WARNING:THIS FILE WILL BE GENERATED AUTOLY,DONT MODIFY IT BY YOURSELF');
    hppLines.push('#pragma once')
    hppLines.push(`namespace ${dllBridgeHppPaths[column].split(".")[0]} {`)

    for(const key of Object.keys(jsonPrase)){
      if(jsonPrase[key] !== "" || null){
        hppLines.push(`    static constexpr const char* ${key} = "${jsonPrase[key]}" ;`);
      }else{
        hppLines.push(`    static constexpr const char* ${key} = "${key}" ;`);
      }
    }
    hppLines.push(`}`);

    const hppContent = hppLines.join('\n')
    fs.writeFileSync(hppOutputPath, hppContent);
  }
}

console.log("generate success ! ! !");