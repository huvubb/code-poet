# 代码诗人 · GitHub Action

> 自动生成诗意代码注释和提交信息 — ¥10

## 使用

```yaml
- uses: huvubb/code-poet/action@main
  with:
    type: comment
    style: tang-song
```

## 输入

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `type` | comment / commit / header | comment |
| `style` | tang-song / wei-jin / modern / humor | tang-song |
| `message` | 提交描述（type=commit 时） | - |
