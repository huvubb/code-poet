#!/usr/bin/env node

const STYLES = {
  "tang-song": "唐宋风",
  "wei-jin": "魏晋风",
  modern: "现代诗",
  humor: "打油诗",
};

const POEMS = {
  comment: {
    "tang-song": "裁云镂月，缩万象于方寸 ✨",
    "wei-jin": "泛彼柏舟，在河之洲。数据之舟，行于网络之流 🍃",
    modern: "在代码的河流里，每一个函数都是一首未完的诗 🌊",
    humor: "这段代码写得妙，犹如老牛吃嫩草 🐮",
  },
  commit: {
    "tang-song": "修竹移影——",
    "wei-jin": "清风徐来——",
    modern: "轻轻推开一扇窗——",
    humor: "又改了一行代码，老板给加鸡腿——",
  },
  header: {
    "tang-song": "天工开物",
    "wei-jin": "山水之间",
    modern: "代码之歌",
    humor: "码农日常",
  },
};

const args = process.argv.slice(2);
const cmd = args[0] || "comment";
const style = args.includes("--style") ? args[args.indexOf("--style") + 1] || "tang-song" : "tang-song";

if (cmd === "--help" || cmd === "-h") {
  console.log(`
代码诗人 CLI — 为代码注入诗意

用法:
  code-poet <命令> [选项]

命令:
  comment   生成诗韵注释
  commit    生成雅致提交信息
  header    生成篇章标题

选项:
  --style    创作风格: tang-song | wei-jin | modern | humor (默认: tang-song)

示例:
  code-poet comment --style wei-jin
  code-poet commit
  code-poet header --style humor
`);
  process.exit(0);
}

if (!POEMS[cmd]) {
  console.error(`未知命令: ${cmd}。使用 --help 查看帮助。`);
  process.exit(1);
}

const poem = POEMS[cmd][style] || POEMS[cmd]["tang-song"];
const styleName = STYLES[style] || "唐宋风";

if (cmd === "comment") {
  console.log(`/**`);
  console.log(` * [${styleName}] ${poem}`);
  console.log(` * 将注释生成提示复制到 AI 中即可获得诗意注释`);
  console.log(` */`);
} else if (cmd === "commit") {
  console.log(`[${styleName}] ${poem} ✨`);
  console.log(`# 将以上信息复制到 AI 中生成完整诗意提交`);
} else if (cmd === "header") {
  console.log(`${"=".repeat(45)}`);
  console.log(`｜  ${styleName} · ${poem}`);
  console.log(`${"=".repeat(45)}`);
}

console.log(`\n💡 风格: ${styleName} | 定价: ¥10`);
