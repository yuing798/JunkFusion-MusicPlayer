#!/usr/bin/env node
'use strict';

/**
 * 监听文件夹内所有 .jsonc 文件，发生变化时自动执行 mp run 指令
 * 零依赖，直接 node watch-jsonc.js 运行
 */

const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');
const {ROOT} = require("./rootPath");

/* ================= 配置区 ================= */
const CONFIG = {
  // 要监听的文件夹（默认是脚本同级的 config 目录）
  dir: process.env.WATCH_DIR || path.join(ROOT,"BridgeMacro"),

  // 监听的文件后缀
  ext: '.jsonc',

  // 变化后要执行的命令（整体交给 shell 执行）
  command: process.env.RUN_CMD || "npm run generate-macro-file",

  // 执行命令时的工作目录
  cwd: process.env.RUN_CWD || process.cwd(),

  // 防抖时间(ms)：编辑器保存一次可能触发多个事件，合并成一次执行
  debounce: 300,

  // 忽略的目录名
  ignoreDirs: new Set(['node_modules', '.git', 'dist', 'build', '.idea', '.vscode']),
};
/* ========================================= */

let debounceTimer = null;
let running = false; // 命令是否正在执行
let dirty = false;   // 执行期间是否又有新变更

/* ---------------- 执行命令 ---------------- */
function runCommand() {
  if (running) return; // 正在跑，先记下，等结束后再跑
  running = true;
  dirty = false;

  console.log(`\n[run] $ ${CONFIG.command}`);
  const child = spawn(CONFIG.command, {
    shell: true,
    stdio: 'inherit',
    cwd: CONFIG.cwd,
  });

  child.on('error', (err) => {
    running = false;
    console.error('[run] 启动失败:', err.message);
  });

  child.on('exit', (code, signal) => {
    running = false;
    console.log(`[run] 结束 (code=${code}${signal ? `, signal=${signal}` : ''})`);
    if (dirty) runCommand(); // 运行期间又有改动，补跑一次
  });
}

function schedule(file) {
  dirty = true;
  clearTimeout(debounceTimer);
  debounceTimer = setTimeout(runCommand, CONFIG.debounce);
}

/* ---------------- 文件监听 ---------------- */
const watchers = new Map(); // dirPath -> fs.FSWatcher

const isTargetFile = (p) => p.toLowerCase().endsWith(CONFIG.ext);

function watchDir(dir) {
  if (watchers.has(dir)) return;

  let watcher;
  try {
    watcher = fs.watch(dir);
  } catch (err) {
    console.error(`[watch] 无法监听 ${dir}: ${err.message}`);
    return;
  }

  watchers.set(dir, watcher);
  console.log(`[watch] + ${dir}`);

  watcher.on('change', (eventType, filename) => {
    if (filename) onFsEvent(dir, filename.toString());
  });

  watcher.on('error', (err) => {
    console.error(`[watch] ${dir} 出错:`, err.message);
  });

  // 递归监听子目录
  let entries = [];
  try {
    entries = fs.readdirSync(dir, { withFileTypes: true });
  } catch {
    return;
  }
  for (const entry of entries) {
    if (entry.isDirectory() && !CONFIG.ignoreDirs.has(entry.name)) {
      watchDir(path.join(dir, entry.name));
    }
  }
}

function onFsEvent(dir, filename) {
  const full = path.join(dir, filename);

  // 新增的目录也要递归监听
  fs.stat(full, (err, stat) => {
    if (!err && stat.isDirectory()) {
      if (!CONFIG.ignoreDirs.has(path.basename(full))) watchDir(full);
      return;
    }

    if (!isTargetFile(full)) return;

    const rel = path.relative(CONFIG.dir, full) || full;
    console.log(`[watch] 变化: ${rel}`);
    schedule(full);
  });
}

/* ---------------- 启动 ---------------- */
function main() {
  if (!fs.existsSync(CONFIG.dir)) {
    console.error(`[watch] 目录不存在: ${CONFIG.dir}`);
    process.exit(1);
  }

  console.log(`[watch] 目录: ${CONFIG.dir}`);
  console.log(`[watch] 匹配: *${CONFIG.ext}`);
  console.log(`[watch] 命令: ${CONFIG.command}\n`);

  watchDir(CONFIG.dir);

  const cleanup = () => {
    console.log('\n[watch] 退出中...');
    for (const w of watchers.values()) w.close();
    process.exit(0);
  };
  process.on('SIGINT', cleanup);
  process.on('SIGTERM', cleanup);
}

main();