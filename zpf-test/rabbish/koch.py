"""
koch.py

一个绘制科赫雪花的程序。

开发者：Mahesh Venkitachalam
"""


import turtle
import math

#以递归的方式绘制科赫雪花
def drawKochSF(x1, y1, x2, y2, t):
    d = math.sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2))
    r = d/3.0
    h = r*math.sqrt(3)/2.0
    p3 = ((x1 + 2*x2)/3.0, (y1 + 2*y2)/3.0)
    p1 = ((2*x1 + x2)/3.0, (2*y1 + y2)/3.0)
    c = (0.5*(x1+x2), 0.5*(y1+y2))
    n = ((y1-y2)/d, (x2-x1)/d)
    p2 = (c[0]+h*n[0], c[1]+h*n[1])
    if d > 10:
        # 第1个片段
        drawKochSF(x1, y1, p1[0], p1[1], t)
        # 第2个片段
        drawKochSF(p1[0], p1[1], p2[0], p2[1], t)
        # #第3个片段
        drawKochSF(p2[0], p2[1], p3[0], p3[1], t)
        # 第4个片段
        drawKochSF(p3[0], p3[1], x2, y2, t)
    else:
        # 绘制中间的角
        t.up()
        t.setpos(p1[0], p1[1])
        t.down()
        t.setpos(p2[0], p2[1])
        t.setpos(p3[0], p3[1])
        # 绘制两侧的边
        t.up()
        t.setpos(x1, y1)
        t.down()
        t.setpos(p1[0], p1[1])
        t.up()
        t.setpos(p3[0], p3[1])
        t.down()
        t.setpos(x2, y2)

# 函数main()
def main():
    print('Drawing the Koch Snowflake...')
    t = turtle.Turtle()
    t.hideturtle()

    # 绘制科赫雪花
    try:
        drawKochSF(-100, 0, 100, 0, t)
        drawKochSF(0, -173.2, -100, 0, t)
        drawKochSF(100, 0, 0, -173.2, t)
    except:
        print("Exception, exiting.")
        exit(0)

    # 等用户在屏幕上单击后退出
    turtle.Screen().exitonclick()

# 调用函数main()
if __name__ == '__main__':
    main()
