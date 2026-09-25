#!/usr/bin/env node

const fs = require('fs');
const path = require('path');

// 参数：node merge.js <输出文件> <输入1> <输入2> ...
const args = process.argv.slice(2);

if (args.length < 2) {
  console.error('用法：node merge-compile-commands.js <输出json> <输入json...>');
  process.exit(1);
}

const outputPath = path.resolve(args[0]);
const inputPaths = args.slice(1).map(p => path.resolve(p));

const merged = new Map(); // key: 规范化后的 file 绝对路径, value: 条目
const stats = [];

for (const inputPath of inputPaths) {
  if (!fs.existsSync(inputPath)) {
    console.warn(`跳过（不存在）：${inputPath}`);
    stats.push({ file: inputPath, count: 0, skipped: true });
    continue;
  }

  let entries;
  try {
    const text = fs.readFileSync(inputPath, 'utf-8');
    entries = JSON.parse(text);
  } catch (err) {
    console.error(`解析失败：${inputPath}`);
    console.error(err.message);
    stats.push({ file: inputPath, count: 0, error: true });
    continue;
  }

  if (!Array.isArray(entries)) {
    console.error(`格式错误（不是数组）：${inputPath}`);
    stats.push({ file: inputPath, count: 0, error: true });
    continue;
  }

  let added = 0;
  for (const entry of entries) {
    if (!entry || typeof entry !== 'object' || !entry.file) continue;

    // 每个输入文件里 directory 可能是相对的，基于该 json 所在目录解析
    const dir = entry.directory
      ? path.resolve(path.dirname(inputPath), entry.directory)
      : path.dirname(inputPath);

    // 统一 file 为绝对路径，方便去重
    const fileAbs = path.isAbsolute(entry.file)
      ? path.normalize(entry.file)
      : path.resolve(dir, entry.file);

    const normalized = {
      ...entry,
      directory: dir,
      file: fileAbs,
    };

    if (merged.has(fileAbs)) {
      // 已存在：保留先到的，或者用新的覆盖，看需求
      // 这里选择保留先到的，避免后面的覆盖前面的
      continue;
    }

    merged.set(fileAbs, normalized);
    added++;
  }

  stats.push({ file: inputPath, count: entries.length, added });
}

const result = Array.from(merged.values());

fs.mkdirSync(path.dirname(outputPath), { recursive: true });
fs.writeFileSync(outputPath, JSON.stringify(result, null, 2), 'utf-8');

console.log('合并完成');
for (const s of stats) {
  if (s.skipped) {
    console.log(`  跳过  ${s.file}`);
  } else if (s.error) {
    console.log(`  错误  ${s.file}`);
  } else {
    console.log(`  ${String(s.added).padStart(6)} / ${String(s.count).padStart(6)}  ${s.file}`);
  }
}
console.log(`输出：${outputPath}（共 ${result.length} 条）`);