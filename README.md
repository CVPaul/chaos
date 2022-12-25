## Chaos

五脏齐全的期权期货交易系统（Over CTP）

## Features

- 内置Pandas DataFrame Like的特征计算引擎API
  - 实时数据天然Rolling Queue存储
- 轻量级订单管理引擎
  - All In Memory模式下50us即可以完成订单更新
- 数据dump&回放（回测）
  - 数据dump和交易可分离，明文存储其它程序/脚本可轻易解析&使用该数据
- 可插拔的Strategy接入方式
  - e.g: chaos::core::strategy::inteval\_trade
