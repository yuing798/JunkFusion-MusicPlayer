//批量生成宏文件的脚本
const { exec, execSync } = require('node:child_process');

const finalLines = [];

const inputFiles = 
  [
    "BridgeMacro/songInfo.jsonc",
    "BridgeMacro/coordinatorMacro.jsonc",
    "BridgeMacro/sliderParam.jsonc",
    "BridgeMacro/AlbumInfo.jsonc",
    "BridgeMacro/playlistMacro.jsonc"
  ];
const namespaceArr = ["SongInfoMacro","CoordinatorMacro","SliderParam","AlbumInfoMacro","PlaylistMacro"]
const outputFiles = 
  [
    ["Macro/SongInfoMacro.hpp","junk_fusion_ui/lib/Macro/SongInfoMacro.dart"],
    ["Macro/coordinatorMacro.hpp","junk_fusion_ui/lib/Macro/coordinatorMacro.dart"],
    ["Macro/sliderParam.hpp","junk_fusion_ui/lib/Macro/sliderParam.dart"],
    ["Macro/AlbumInfoMacro.hpp","junk_fusion_ui/lib/Macro/AlbumInfoMacro.dart"],
    ["Macro/PlaylistMacro.hpp","junk_fusion_ui/lib/Macro/PlaylistMacro.dart"]
  ];

for(let i=0;i<inputFiles.length;i++){
  finalLines.push("node ");
  finalLines.push("DevScripts/generateMacroFile.cjs ");
  finalLines.push(`${inputFiles[i]} `);
  finalLines.push(`${namespaceArr[i]} `);
  for(let j=0;j<outputFiles[i].length;j++){
    finalLines.push(`${outputFiles[i][j]} `);
  }
  if(i!==inputFiles.length-1){
    finalLines.push(" && ");
  }
}
const finalCommand = finalLines.join("");
console.log(`最终输出的命令行字符串:${finalCommand}`);
execSync(finalCommand);
console.log("全部宏文件生成完成");

