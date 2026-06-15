# 巡线小车 + 二轴云台激光打靶

第一届机甲竞技赛作品，85分 / 100分。

## 系统架构

```
STM32F103C8T6 (巡线 + 编码器停车)     K230 (视觉 + 云台 + 激光)
       |                                      |
  八路巡线模块                            cv_lite 矩形检测
  TIM2/TIM4 编码器                        二轴舵机云台
  TIM1 PWM 电机驱动                       GPIO24 激光
  PB15 停车信号 ──(未调通)──→              IO10 停车检测
```

## 目录

- `stm32/` - STM32 固件源码（Makefile 构建）
- `k230/` - K230 MicroPython 视觉+云台+激光
- `docs/` - 比赛总结文档

## 关键参数

- 巡线 PID：Kp=200, Ki=1, Kd=25
- 速度：BASE_SPEED=600
- 编码器：TIM2/TIM4 硬件正交解码，2x 分辨率 (~520脉冲/轮转)
- 编码器停车阈值：15650 / 24870 / 31020 / 73000
- 激光：对准靶心后自动打 3 次（亮 500ms / 灭 200ms），稳定对准 15 帧触发
- 云台：X轴 GPIO40，Y轴 GPIO44，UART 控制

## 硬件接线

### 编码器
| 信号 | STM32 | 定时器 |
|------|-------|--------|
| 左A  | PB6   | TIM4_CH1 |
| 左B  | PB7   | TIM4_CH2 |
| 右A  | PA15  | TIM2_CH1 (remap) |
| 右B  | PB3   | TIM2_CH2 (remap) |

### 电机
| 信号 | STM32 |
|------|-------|
| 左PWM | PA8 (TIM1_CH1) |
| 左IN1 | PA9 |
| 左IN2 | PA10 |
| 右PWM | PA11 (TIM1_CH4) |
| 右IN1 | PB12 |
| 右IN2 | PB13 |
| STBY | PC13 |

### 巡线
PA0 ~ PA7（八路灰度传感器）

## 构建与烧录

STM32 CubeIDE 2.1.1，MinGW Make + STM32CubeProgrammer CLI：

```bash
make -C stm32 all
STM32_Programmer_CLI -c port=SWD -w stm32/build/line_follower.hex -v -rst
```

K230 通过 CanMV IDE 烧录 `k230/k230_fixed.py`。

## 遗留问题

STM32 PB15 → K230 IO10 通信未调通（GPIO电平检测/UART串口均失败）。当前激光打靶依赖 K230 自主视觉判定，缺少到站确认。