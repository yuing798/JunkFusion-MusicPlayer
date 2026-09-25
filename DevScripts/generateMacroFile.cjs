
/**
 * @description 根据JSONC文件自动生成宏的脚本
 * @param jsonc文件的路径
 * @param nameSpace或者struct的名称
 * @param 输出文件的路径数组
 */

if(process.argv.length < 5){
  console.error(`命令行传参错误，要求最少3个，当前共有${process.argv.length-2}个参数`);
  return;
}

const fs = require('fs');//node.js的File System，文件系统模块
const jsonc = require('jsonc-parser');//用来解析jsonc

const jsoncPath = process.argv[2];

const nameSpaceName = process.argv[3];
const outputPaths = process.argv.slice(4);

const defs = jsonc.parse(fs.readFileSync(jsoncPath, 'utf-8'));

for(let i=0;i<outputPaths.length;i++){
  const extension = outputPaths[i].split(".").pop();
  if(extension === "hpp"){
    const hppLines = []
    hppLines.push('//warning:this file will be generated auto,dont modify it by yourself\n');
    hppLines.push('#pragma once\n');

    hppLines.push(`namespace ${nameSpaceName}{\n`);

    for(const key of Object.keys(defs)){
      hppLines.push("    ");
      if(defs[key] !== ""){
        hppLines.push(`static constexpr const char* ${key} = "${defs[key]}" ;`);
      }else{
        hppLines.push(`static constexpr const char* ${key} = "${key}" ;`);
      }

      hppLines.push("\n");
    }
    hppLines.push("}");

    const hppContent = hppLines.join('')
    fs.writeFileSync(outputPaths[i], hppContent);
    console.log("hpp file generate success");
  }

  if(extension === "js"){
    const jsLines = []
    jsLines.push('//warning:this file will be generated auto,dont modify it by yourself\n');

    jsLines.push(`export const ${nameSpaceName}{\n`);

    for(const key of Object.keys(defs)){
      jsLines.push("  ");
      if(defs[key] !== ""){
        jsLines.push(`${key} : "${defs[key]}" ;`);
      }else{
        jsLines.push(`${key} : "${key}" ;`);
      }

      jsLines.push("\n");
    }
    jsLines.push("};");

    const jsContent = jsLines.join('')
    fs.writeFileSync(outputPaths[i], jsContent);
    console.log("js file generate success");
  }
  if(extension === "dart"){
    const dartLines = []
    dartLines.push('//warning:this file will be generated auto,dont modify it by yourself\n');
    dartLines.push('\n');
    dartLines.push(`abstract class ${nameSpaceName} {\n`);

    for(const key of Object.keys(defs)){
      dartLines.push("  ");
      if(defs[key] !== ""){
        dartLines.push(`static const String ${key} = "${defs[key]}";`);
      }else{
        dartLines.push(`static const String ${key} = "${key}";`);
      }

      dartLines.push("\n");
    }
    dartLines.push("}\n");

    const dartContent = dartLines.join('')
    fs.writeFileSync(outputPaths[i], dartContent);
    console.log("dart file generate success");
  }
}