"""
spiro.py
　
一个模拟万花尺轨迹的Python程序
　
编写者：Mahesh Venkitachalam
"""

import random, argparse
import numpy as np
import math
import turtle
import random
from PIL import Image
from datetime import datetime

# 一个绘制繁花曲线的类
class Spiro:
    # 构造函数
    def __init__(self, xc, yc, col, R, r, l):
        # 创建turtle对象
        self.t = turtle.Turtle()
        # 设置光标形状
        self.t.shape('turtle')
        # 设置以度为单位的步长
        self.step = 5
        # 设置绘画结束标志
        self.drawingComplete = False

        # 设置参数
        self.setparams(xc, yc, col, R, r, l)

        # 开始绘画
        self.restart()

    # 设置参数
    def setparams(self, xc, yc, col, R, r, l):
        # 设置定义繁花曲线的参数
        self.xc = xc
        self.yc = yc
        self.R = int(R)
        self.r = int(r)
        self.l = l
        self.col = col
        # 通过除以GCD将分数约分
        gcdVal = math.gcd(self.r, self.R)
        self.nRot = self.r//gcdVal
        # 计算半径比
        self.k = r/float(R)
        # 设置颜色
        self.t.color(*col)
        # 存储当前角度
        self.a = 0

    # 重新开始绘画
    def restart(self):
        # 设置绘画结束标志
        self.drawingComplete = False
        # 显示海龟
        self.t.showturtle()
        # 移到起始位置
        self.t.up()
        R, k, l = self.R, self.k, self.l
        a = 0.0
        x = R*((1-k)*math.cos(a) + l*k*math.cos((1-k)*a/k))
        y = R*((1-k)*math.sin(a) - l*k*math.sin((1-k)*a/k))
        try:
            self.t.setpos(self.xc + x, self.yc + y)
        except:
            print("Exception, exiting.")
            exit(0)
        self.t.down()

    # 绘制繁花曲线
    def draw(self):
        # 绘制余下的线段
        R, k, l = self.R, self.k, self.l
        for i in range(0, 360*self.nRot + 1, self.step):
            a = math.radians(i)
            x = R*((1-k)*math.cos(a) + l*k*math.cos((1-k)*a/k))
            y = R*((1-k)*math.sin(a) - l*k*math.sin((1-k)*a/k))
            try:
                self.t.setpos(self.xc + x, self.yc + y)
            except:
                print("Exception, exiting.")
                exit(0)
        # 绘制完毕后隐藏海龟光标
        self.t.hideturtle()

  # 绘制一条线段
    def update(self):
        # 如果已绘制完毕，就跳过后面的步骤
        if self.drawingComplete:
            return
        # 递增角度
        self.a += self.step
        # 绘制一条线段
        R, k, l = self.R, self.k, self.l
        # 设置角度
        a = math.radians(self.a)
        x = self.R*((1-k)*math.cos(a) + l*k*math.cos((1-k)*a/k))
        y = self.R*((1-k)*math.sin(a) - l*k*math.sin((1-k)*a/k))
        try:
            self.t.setpos(self.xc + x, self.yc + y)
        except:
            print("Exception, exiting.")
            exit(0)
        # 如果已绘制完毕，就设置相应的标志
        if self.a >= 360*self.nRot:
            self.drawingComplete = True
            # 已绘制完毕，因此隐藏海龟光标
            self.t.hideturtle()

    # 清屏
    def clear(self):
        # 抬起画笔
        self.t.up()
        # 清除turtle对象的内容
        self.t.clear()

# 一个以动画方式绘制繁花曲线的类
class SpiroAnimator:
    # 构造函数
    def __init__(self, N):
        # 设置定时器值，单位为毫秒
        self.deltaT = 10
        # 获取窗口尺寸
        self.width = turtle.window_width()
        self.height = turtle.window_height()
        # 设置重新开始标志
        self.restarting = False
        # 创建Spiro对象
        self.spiros = []
        for i in range(N):
            # 生成随机参数
            rparams = self.genRandomParams()
            # 设置繁花曲线参数
            spiro = Spiro(*rparams)
            self.spiros.append(spiro)
        # 调用定时器
        turtle.ontimer(self.update, self.deltaT)

    # 重新开始繁花曲线绘制
    def restart(self):
        # 如果正在重新开始，就不再重新开始
        if self.restarting:
            return
        else:
            self.restarting = True
        # 重新开始
        for spiro in self.spiros:
            # 清屏
            spiro.clear()
            # 生成随机参数
            rparams = self.genRandomParams()
            # 设置繁花曲线的参数
            spiro.setparams(*rparams)
            # 重新开始绘制
            spiro.restart()
        # 结束重新开始绘制过程
        self.restarting = False

    # 生成随机参数
    def genRandomParams(self):
        width, height = self.width, self.height
        R = random.randint(50, min(width, height)//2)
        r = random.randint(10, 9*R//10)
        l = random.uniform(0.1, 0.9)
        xc = random.randint(-width//2, width//2)
        yc = random.randint(-height//2, height//2)
        col = (random.random(),
               random.random(),
               random.random())
        return (xc, yc, col, R, r, l)

    def update(self):
        # 更新所有的繁花曲线
        nComplete = 0
        for spiro in self.spiros:
            # 更新
            spiro.update()
            # 计算已绘制完毕的繁花曲线数
            if spiro.drawingComplete:
                nComplete+= 1
        # 如果所有的繁花曲线都已绘制完毕，就重新开始
        if nComplete == len(self.spiros):
            self.restart()
        # 调用定时器
        try:
            turtle.ontimer(self.update, self.deltaT)
        except:
            print("Exception, exiting.")
            exit(0)

    # 在显示和隐藏海龟光标之间切换
    def toggleTurtles(self):
        for spiro in self.spiros:
            if spiro.t.isvisible():
                spiro.t.hideturtle()
            else:
                spiro.t.showturtle()

# 将繁花曲线保存为图像
def saveDrawing():
    # 隐藏海龟光标
    turtle.hideturtle()
    # 生成独一无二的文件名
    dateStr = (datetime.now()).strftime("%d%b%Y-%H%M%S")
    fileName = 'spiro-' + dateStr
    print('saving drawing to {}.eps/png'.format(fileName))
    # 获取tkinter画布
    canvas = turtle.getcanvas()
    # 将图形保存为EPS文件
    canvas.postscript(file = fileName + '.eps')
    # 使用模块Pillow将EPS文件转换为PNG文件
    img = Image.open(fileName + '.eps')
    img.save(fileName + '.png', 'png')
    # 显示海龟光标
    turtle.showturtle()

# 函数main()
def main():
    # 必要时使用sys.argv
    print('generating spirograph...')
    # 创建对象
    descStr = """这个程序使用模块turtle绘制繁花曲线
    如果运行时没有指定参数，这个程序将绘制随机的繁花曲线

    参数说明如下
    R：外圆半径
    r：内圆半径
    l：孔洞距离与r与R的比值
    """

    parser = argparse.ArgumentParser(description=descStr)

    # 添加要求的参数
    parser.add_argument('--sparams', nargs=3, dest='sparams', required=False,
                        help="The three arguments in sparams: R, r, l.")
    # 分析参数
    args = parser.parse_args()

    # 将绘图窗口的宽度设置为屏幕宽度的80%
    turtle.setup(width=0.8)

    # 设置光标形状
    turtle.shape('turtle')

    # 设置标题
    turtle.title("Spirographs!")
    # 添加保存图像的按键处理程序
    turtle.onkey(saveDrawing, "s")
    # 开始侦听
    turtle.listen()

    # 隐藏海龟光标
    turtle.hideturtle()

    # 检查参数并绘制繁花曲线
    if args.sparams:
        params = [float(x) for x in args.sparams]
        # 使用给定的参数绘制繁花曲线
        # 默认为黑色
        col = (0.0, 0.0, 0.0)
        spiro = Spiro(0, 0, col, *params)
        spiro.draw()
    else:
        # 创建SpiroAnimator对象
        spiroAnim = SpiroAnimator(4)
        # 添加在显示/隐藏海龟光标之间切换的按键处理程序
        turtle.onkey(spiroAnim.toggleTurtles, "t")
        # 添加重新开始绘制的按键处理程序
        turtle.onkey(spiroAnim.restart, "space")

    # 开始turtle主循环
    turtle.mainloop()

# 调用函数main()
if __name__ == '__main__':
    main()
