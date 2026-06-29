const vscode = require("vscode");

const STYLES = {
  "tang-song": "唐宋风",
  "wei-jin": "魏晋风",
  modern: "现代诗",
  humor: "打油诗",
};

function activate(context) {
  const statusBar = vscode.window.createStatusBarItem(
    vscode.StatusBarAlignment.Right,
    100
  );
  statusBar.text = "$(pencil) 代码诗人";
  statusBar.tooltip = "点击切换创作风格";
  statusBar.command = "code-poet.switchStyle";
  statusBar.show();

  function getStyle() {
    return vscode.workspace.getConfiguration("codePoet").get("style", "tang-song");
  }

  context.subscriptions.push(
    vscode.commands.registerCommand("code-poet.poeticComment", async () => {
      const editor = vscode.window.activeTextEditor;
      if (!editor) return;
      const selection = editor.document.getText(editor.selection);
      if (!selection) {
        vscode.window.showInformationMessage("请先选中要生成注释的代码");
        return;
      }
      const style = STYLES[getStyle()];
      const line = editor.selection.start.line;
      const indent = " ".repeat(editor.document.lineAt(line).firstNonWhitespaceCharacterIndex);
      const comment = [
        `${indent}/**`,
        `${indent} * [${style}] ✨ 裁云镂月 · 代码生辉`,
        `${indent} * 将注释生成提示复制到 AI 中即可获得诗意注释`,
        `${indent} */`,
      ].join("\n");
      await editor.edit((eb) => {
        eb.insert(new vscode.Position(line, 0), comment + "\n");
      });
      vscode.window.showInformationMessage("诗韵注释模板已生成 ✨");
    })
  );

  context.subscriptions.push(
    vscode.commands.registerCommand("code-poet.poeticCommit", async () => {
      const msg = await vscode.window.showInputBox({
        prompt: "描述你的改动",
        placeHolder: "如：修复了登录页面在移动端的布局问题",
      });
      if (!msg) return;
      const style = STYLES[getStyle()];
      const prefix = vscode.workspace.getConfiguration("codePoet").get("commitPrefix", "feat");
      const commitMsg = [
        `${prefix}: [${style}] ✨ ${msg}`,
        "",
        "# 将以上信息复制到 AI 中生成完整诗意提交",
      ].join("\n");
      const doc = await vscode.workspace.openTextDocument({ content: commitMsg });
      await vscode.window.showTextDocument(doc);
    })
  );

  context.subscriptions.push(
    vscode.commands.registerCommand("code-poet.poeticHeader", async () => {
      const editor = vscode.window.activeTextEditor;
      if (!editor) return;
      const title = await vscode.window.showInputBox({
        prompt: "输入章节标题",
        placeHolder: "如：配置模块",
      });
      if (!title) return;
      const line = editor.selection.start.line;
      const header = [
        `// ${"=".repeat(40)}`,
        `// ｜  ${title}`.padEnd(45) + "｜",
        `// ${"=".repeat(40)}`,
      ].join("\n");
      await editor.edit((eb) => {
        eb.insert(new vscode.Position(line, 0), header + "\n");
      });
      vscode.window.showInformationMessage("篇章标题已生成 ✨");
    })
  );

  context.subscriptions.push(
    vscode.commands.registerCommand("code-poet.switchStyle", async () => {
      const pick = await vscode.window.showQuickPick(
        Object.entries(STYLES).map(([k, v]) => ({
          label: v,
          description: k === getStyle() ? "当前" : "",
          value: k,
        })),
        { placeHolder: "选择创作风格" }
      );
      if (pick) {
        await vscode.workspace
          .getConfiguration("codePoet")
          .update("style", pick.value, true);
        statusBar.text = `$(pencil) 代码诗人 · ${pick.label}`;
      }
    })
  );

  context.subscriptions.push(statusBar);
}

function deactivate() {}

module.exports = { activate, deactivate };
