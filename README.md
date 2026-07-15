# QQMusic2MP3 v1.0

QQ音乐加密文件批量转MP3工具

## 功能
- 自动扫描指定目录下的QQ音乐加密文件
- 支持格式: .qmc0/.qmc2/.qmc3/.qmc4/.qmc6/.qmc8/.qmcflac/.qmcogg/.mgg/.mflac
- 自动解密并转换为MP3（320kbps高质量）
- 自动下载ffmpeg（无需手动安装）
- 多线程并行处理
- 输出到 D:\desktop\QQMusic_MP3

## 使用方法
1. 双击运行 `qqmusic2mp3.exe`
2. 按提示输入QQ音乐文件夹路径
3. 等待处理完成

或命令行：
```
qqmusic2mp3.exe D:\QQMusic
```

## 文件说明
- `qqmusic2mp3.exe` - 主程序（已签名）
- `qqmusic2mp3.cpp` - 源代码
- `build.bat` - 编译脚本（需安装MinGW）
- `sign.ps1` - 签名脚本
- `MyCodeSignCert.pfx` - 自签名证书

## 编译要求
- MinGW-W64 (g++ 16+)
- Windows SDK (urlmon, shlwapi, wininet)

## 输出目录
转换后的MP3文件保存在: `D:\desktop\QQMusic_MP3`

---

## ⚠️ 免责声明

本软件不提供任何形式的明示或暗示担保。
使用即视为您同意自行承担所有风险。
开发者对因使用本软件而导致的任何数据丢失、系统损坏或其他损失概不负责。
