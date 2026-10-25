# RoboPlanter 外观预览

这是 `../model.glb` 的本地可旋转预览。页面注明“Tripo生成 · 外观概念模型”；它不代表工程 CAD、制造尺寸或硬件已验证。

在仓库根目录启动只服务该模型目录的静态服务：

```sh
python3 -m http.server 8876 --bind 127.0.0.1 --directory design/models/roboplanter-all-in-one
```

打开 <http://127.0.0.1:8876/viewer/>。ES modules 需要 HTTP 服务，不要直接双击 `index.html`。所有脚本、样式和库都保存在本地；预览不控制设备。模型缺失或加载失败时会显示说明。

拖动旋转、滚轮或双指缩放；“重置视角”恢复初始取景。“自动转台”默认关闭，系统启用“减少动态效果”时禁用；页面隐藏时暂停渲染，可见时渲染上限约为 30 帧/秒。GLB 的嵌入资源以及同目录本地资源可以加载，跨域或目录外资源会被拒绝。

## 依赖与许可

`vendor/three/` 从仓库已有的 `tmp/docs-site/node_modules/three` 复制，版本 **0.170.0（r170）**，包括同版本 `GLTFLoader`、`OrbitControls`、`BufferGeometryUtils`、`RoomEnvironment`。没有联网或安装依赖。MIT 许可全文保留于 [`vendor/three/LICENSE`](vendor/three/LICENSE)，未修改库文件。

静态检查可运行 `node --input-type=module --check < design/models/roboplanter-all-in-one/viewer/viewer.js`。视觉检查需在支持 WebGL 的浏览器打开上述地址，并验证加载、手动旋转、缩放、重置、自动转台开关，以及手机尺寸布局；这些检查只验证预览，不验证模型尺寸或实物可制造性。
