# 代码诗人 · code-poet

为代码注入诗意 ✨

code-poet 是一个轻量的开发辅助工具，帮助你把代码注释、提交说明和标题写得更自然、更优雅，同时也方便和 Codex、Claude Code、GitHub Action 等工作流结合使用。

[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Cross%20Tooling-lightgrey.svg)]()

> 适合：希望代码表达更清晰、提交信息更优雅、项目文档更有气质的开发者。

## 这是什么

它不是“把代码写成诗”，而是让代码更容易被理解、更有韵律感，同时保留工程上的清晰度。

它可以用于：

- 生成更自然的注释
- 生成更顺畅的 commit message
- 生成简洁但有风格的标题
- 为 AI 编程助手生成更人性化的输出
- 让代码审查、提交日志和团队协作更顺

## 典型用法

### 1. 代码注释

把机械、枯燥的注释转成更有节奏、可读性更强的表达。

### 2. 提交信息

生成适合 Git 提交的说明文字，而不是单调的“fix bug”或“update files”。

### 3. 分隔标题和文案

适合生成 PR 标题、版本说明、功能说明段落等。

## 风格支持

目前支持多种风格：

- 唐宋风：更古雅、节奏感强
- 魏晋风：更含蓄、简洁、冷静
- 现代风：更直观、更实用
- 幽默风：更轻松、适合日常开发输出

## 快速开始

### 本地使用

```bash
git clone https://github.com/huvubb/code-poet.git
cd code-poet
```

查看脚本和示例后按自己的方式接入即可。

### GitHub Action

```yaml
- uses: huvubb/code-poet/action@main
  with:
    type: comment
    style: tang-song
```

支持参数：

- `type`: `comment` / `commit` / `header`
- `style`: `tang-song` / `wei-jin` / `modern` / `humor`
- `message`: 提交说明（仅在 `type=commit` 时使用）

### Claude Code

把项目中的 `CLAUDE.md` 复制到目标项目根目录：

```bash
cp CLAUDE.md /你的项目/
```

或全局生效：

```bash
cp CLAUDE.md ~/.claude/CLAUDE.md
```

随后在 Claude Code 中直接使用相关提示词即可。

## 目录结构

```text
.
├── LICENSE
├── README.md
├── CLAUDE.md
├── code-poet-action/
│   └── README.md
├── code-poet-claude/
│   └── README.md
├── ...
└── scripts/
```

## 适用场景

- 个人开发者想让代码输出更自然
- 团队协作时希望提交信息更统一、更有说明性
- AI 编程辅助工作流中想生成更像人写的文案
- GitHub Actions 自动化生成注释、标题与提交说明

## 许可证

MIT License

## 说明

这个项目的目标不是把代码“变成诗”，而是让工程中的文字表达更自然、更舒适、更具有可读性，同时保持开发工作流的实用性与清晰度。
