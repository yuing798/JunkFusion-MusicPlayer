首先，感谢你愿意为这个项目做出贡献！正是因为有像你这样的开发者，开源社区才能如此繁荣。

为了保证协作顺畅，并保持 Git 历史的干净整洁，请在提交 Pull Request (PR) 前，仔细阅读以下规范。

# 一、准备工作
在 GitHub 上点击 Fork 按钮，将本仓库 Fork 到你的账号下。

将你的 Fork 克隆到本地：

``` bash
git clone https://github.com/yuing798/JunkFusion-MusicPlayer.git
cd JunkFusion-MusicPlayer
``` 
配置远程仓库别名，将官方仓库添加为 upstream：

``` bash
git remote add upstream https://github.com/yuing798/JunkFusion-MusicPlayer.git
git remote -v
``` 
以后 origin 指向你的 Fork，upstream 指向官方仓库。

# 二、开发流程（黄金法则）
永远不要在 main 分支上直接开发！ 所有的改动都必须在独立的 feature 或 fix 分支上进行。

同步官方最新代码：

``` bash
git checkout main
git fetch upstream
git reset --hard upstream/main
git push origin main
``` 
创建新分支（请使用描述性名称）：

``` bash
git checkout -b feat/your-feature-name  或 git checkout -b fix/issue-123
``` 
进行开发并提交：

不要提交自动生成的文件（如 generated_*、GeneratedPluginRegistrant.*、build/ 目录、pubspec.lock 等），除非你的功能确实需要更改依赖。
不过这些应该加入gitignore了，所以无需重视此条规范

提交信息请遵循 Conventional Commits 规范：

```text
<type>(<scope>): <subject>

<body>
```

常用的commit前缀
前缀	含义	什么时候用
feat	新功能	给用户/API 增加新能力
fix	修复 bug	修正错误行为
docs	文档	只改 README、注释、文档
style	代码格式	空格、分号、格式化，不改逻辑
refactor	重构	改代码结构，但不改外部行为
perf	性能优化	提升速度、减少内存/重建
test	测试	增加或修改测试
build	构建/依赖	改 pubspec、Gradle、npm、构建脚本
ci	持续集成	改 GitHub Actions、CI 配置
chore	杂项	不涉及 src/test 的杂活，如配置、工具
revert	回滚	撤销之前的 commit

示例：

``` bash
git commit -m "feat(menu): add custom action support"
``` 
同步并整理历史（Squash）：
在推送前，请先变基到官方最新代码，并整理你的提交历史：

``` bash
git fetch upstream
git rebase -i upstream/main
``` 
在编辑器中，保留第一个 commit 为 pick，将其余的 commit 改为 squash（或者 s）。保存后，重写一个干净、清晰的 commit message。

推送到你的 Fork：

bash
git push -u origin feat/your-feature-name
（如果之前已经推送过，且 rebase 改写了历史，请使用 git push --force）

# 三、提交 Pull Request (PR)
打开 GitHub，从你的 Fork 分支向官方仓库的 main 分支发起 PR。

PR 标题：请保持与你的 commit 标题一致，例如 feat(menu): add custom action support。

PR 描述：请详细说明以下内容：

背景/解决的问题：为什么要做这个改动？修复了哪个 Issue？

改动内容：具体修改了哪些文件，实现了什么逻辑。

测试方式：如何在本地验证这个改动？

确保 CI（持续集成）测试通过。

四、响应评审与更新 PR
如果维护者对你的代码提出了评审意见（Review Comments），不要关闭 PR 并重新开一个。请直接在原分支上修改：

在本地修改代码，并提交新的 commit：

``` bash
git add <修改的文件>
git commit -m "Integrate review feedback"
#再次将新 commit 压缩进原来的 commit 中：
git rebase -i upstream/main
``` 
将新的 commit 改为 squash，合并到原来的 commit 中
强制推送到你的 Fork，PR 会自动更新：


``` bash
git push --force
``` 
五、合并与清理
当你的 PR 被维护者合并后，你可以清理本地的分支：

``` bash
git checkout main
git fetch upstream
git reset --hard upstream/main
git branch -d feat/your-feature-name
```
---

再次感谢你的贡献！