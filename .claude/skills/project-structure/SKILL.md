---
name: project-structure
description: This document describes the architecture of this project.
---

## 功能：按库区分

**FFmpeg**：解码层，采样点对齐功能与重采样层

**JUCE**：音频引擎，音频图，UI设计，VST插件支持

**ONNX Runtime**:神经网络推理:在该应用中包括：ai音乐风格分类，ai-agent用户交互，ai音质增强，ai分轨，ai音源更换

**SQLite**:曲库管理,以及对每首歌曲的内置参数管理

**spdlog**：日志管理

## 功能：按线程与进程划分

### 主进程：播放器主体

线程一：UI线程

线程二：音频线程

线程三：文件读取(音视频文件)(这一个线程指的是读取磁盘文件)

线程四：文件读取(音视频文件)和插件读取(VST,AU),FFmpeg 解码(这一个线程指的是读取已经加入序列化，数据库的文件)

线程五：ai轻量推理(ai音乐风格分类，ai-agent用户交互)

线程六：SQL数据库查询

### 次级进程一：AI神经网络

线程一：ai音质增强，ai分轨，ai音源更换

### 次级进程二：VST/AU音频插件插入

单线程:我这个播放器是类似DAW软件可以插入音频插件的