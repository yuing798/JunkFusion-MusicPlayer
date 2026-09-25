/// 使用cmake-js构建node文件的脚本

const { execSync } = require('child_process');
const path = require('path');
const fs = require('fs');

// 过滤 PATH：移除 SysGCC（防止编译器选错）
const sep = process.platform === 'win32' ? ';' : ':';
process.env.PATH = process.env.PATH
    .split(sep)
    .filter(p => !p.toLowerCase().includes('sysgcc'))
    .join(sep);

console.log('PATH has SysGCC:', process.env.PATH.toLowerCase().includes('sysgcc'));

// ★ 关键：直接调用 cmake-js 的 JS 入口，不用 npx
// npx 会把 node_modules/.bin prepend 到 PATH，导致 rc/mt 被 npm shim 抢走
const cmakeJsEntry = path.resolve('node_modules/cmake-js/bin/cmake-js');
if (!fs.existsSync(cmakeJsEntry)) {
    console.error('找不到 cmake-js 入口：', cmakeJsEntry);
    process.exit(1);
}

const cmakeJsCommand =
    `"${process.execPath}" "${cmakeJsEntry}" compile ` +
    `-d cpp/NodeLink ` +
    `--runtime=electron --runtime-version=42.11.6 --arch=x64 ` +
    `--generator Ninja`;

function build() {
    if (process.platform === 'win32') {
        const vswherePath = path.join(
            process.env['ProgramFiles(x86)'],
            'Microsoft Visual Studio', 'Installer', 'vswhere.exe'
        );

        if (!fs.existsSync(vswherePath)) {
            console.error('找不到 vswhere.exe，请确保安装了 Visual Studio 2017 或更高版本。');
            process.exit(1);
        }

        const vswhereCmd = `"${vswherePath}" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`;

        let vsInstallPath;
        try {
            vsInstallPath = execSync(vswhereCmd).toString().trim();
        } catch (e) {
            console.error('未找到安装了 C++ 工具集的 Visual Studio。');
            process.exit(1);
        }

        if (!vsInstallPath) {
            console.error('未找到合适的 Visual Studio 安装。');
            process.exit(1);
        }

        const vcvarsPath = path.join(vsInstallPath, 'VC', 'Auxiliary', 'Build', 'vcvars64.bat');

        console.log(`Using vcvars: ${vcvarsPath}`);
        console.log(`Working directory: ${process.cwd()}`);

        // call vcvars 后，Windows SDK 会被加进 PATH 前面
        // 因为没有 npx 的干扰，CMake 就能找到真正的 rc.exe 和 mt.exe
        const fullCommand = `call "${vcvarsPath}" && ${cmakeJsCommand}`;

        execSync(fullCommand, {
            stdio: 'inherit',
            shell: 'cmd.exe',
            env: process.env,
        });
    } else {
        execSync(cmakeJsCommand, { stdio: 'inherit' });
    }
}

build();