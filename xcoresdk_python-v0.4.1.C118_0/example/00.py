import numpy as np
import matplotlib.pyplot as plt

# 定义x的范围
x = np.linspace(-2, 2, 400)

# 计算y的值
y1 = np.sqrt(x**2 * (1 - x**2 / 2))
y2 = -np.sqrt(x**2 * (1 - x**2 / 2))

# 绘制图像
plt.figure(figsize=(8, 6))
plt.plot(x, y1, label='y = sqrt(x^2(1 - x^2/2))')
plt.plot(x, y2, label='y = -sqrt(x^2(1 - x^2/2))')

# 添加图例
plt.legend()

# 添加标题和标签
plt.title('Plot of y^2 = x^2(1 - x^2/2)')
plt.xlabel('x')
plt.ylabel('y')

# 显示网格
plt.grid(True)

# 显示图像
plt.show()