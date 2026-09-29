import numpy as np
import matplotlib.pyplot as plt

plt.rcParams['font.sans-serif'] = ['SimHei']  # 中文显示
plt.rcParams['axes.unicode_minus'] = False  # 负号显示

# ========== 鞍结分岔 ==========
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

# 参数范围
r_vals = np.linspace(-3, 3, 1000)

# 计算不动点及稳定性
x_stable_a1, x_unstable_a1 = [], []
x_stable_a2, x_unstable_a2 = [], []

for r in r_vals:
    D = r ** 2 - 4
    if D >= 0:
        x1 = (-r + np.sqrt(D)) / 2
        x2 = (-r - np.sqrt(D)) / 2
        f_prime1 = r + 2 * x1
        f_prime2 = r + 2 * x2

        # 根据稳定性分类
        if r > 2:
            if f_prime1 < 0:
                x_stable_a1.append((r, x1))
            else:
                x_unstable_a1.append((r, x1))
            if f_prime2 < 0:
                x_stable_a1.append((r, x2))
            else:
                x_unstable_a1.append((r, x2))
        elif r < -2:
            if f_prime1 < 0:
                x_stable_a2.append((r, x1))
            else:
                x_unstable_a2.append((r, x1))
            if f_prime2 < 0:
                x_stable_a2.append((r, x2))
            else:
                x_unstable_a2.append((r, x2))

# 绘图
ax1.plot(*zip(*x_stable_a1), 'b-', lw=2, label='稳定')
ax1.plot(*zip(*x_unstable_a1), 'b--', lw=2, label='不稳定')
ax1.plot(*zip(*x_stable_a2), 'r-', lw=2)
ax1.plot(*zip(*x_unstable_a2), 'r--', lw=2)
ax1.set_xlabel('r'), ax1.set_ylabel('x')
ax1.set_title('鞍结分岔'), ax1.legend()
ax1.grid(True)

# ==========  跨临界分岔 ==========
r_vals = np.linspace(0.1, 2, 1000)  # 避免r=0

# 计算不动点及稳定性
x0_stable, x0_unstable = [], []
x1_stable, x1_unstable = [], []

for r in r_vals:
    x1_val = 1 - 1 / r if r != 0 else np.nan

    # x=0点稳定性
    f_prime0 = 1 - r
    if f_prime0 < 0:
        x0_stable.append((r, 0))
    else:
        x0_unstable.append((r, 0))

    # x=1-1/r点稳定性
    if r <= 1 and r > 0:
        f_prime1 = r - 1
        if f_prime1 < 0:
            x1_stable.append((r, x1_val))
        else:
            x1_unstable.append((r, x1_val))
    elif r > 1:
        f_prime1 = r - 1
        if f_prime1 < 0:
            x1_stable.append((r, x1_val))
        else:
            x1_unstable.append((r, x1_val))

# 绘图
ax2.plot(*zip(*x0_stable), 'b-', lw=2, label='稳定')
ax2.plot(*zip(*x0_unstable), 'b--', lw=2, label='不稳定')
ax2.plot(*zip(*x1_stable), 'r-', lw=2)
ax2.plot(*zip(*x1_unstable), 'r--', lw=2)
ax2.set_xlabel('r'), ax2.set_ylabel('x')
ax2.set_title('跨临界分岔'), ax2.legend()
ax2.grid(True)

plt.tight_layout()
plt.show()
import numpy as np
import matplotlib.pyplot as plt

# 定义网格范围
x = np.linspace(-1.5, 1.5, 100)
y = np.linspace(-1.5, 1.5, 100)
X, Y = np.meshgrid(x, y)

# 计算零斜率线
F_xnull = Y * (1 + (1 - X**2) * (Y**2 - X**2*(1 - X**2/2)))
F_ynull = X*(1 - X**2) - Y*(Y**2 - X**2*(1 - X**2/2))

# 绘图设置
plt.figure(figsize=(10, 8))
plt.contour(X, Y, F_xnull, levels=[0], colors='blue', linestyles='solid', linewidths=2)
plt.contour(X, Y, F_ynull, levels=[0], colors='red', linestyles='dashed', linewidths=2)

# 向量场计算
DX = Y + Y*(1 - X**2)*(Y**2 - X**2*(1 - X**2/2))
DY = X*(1 - X**2) - Y*(Y**2 - X**2*(1 - X**2/2))

# 绘制流线
plt.streamplot(X, Y, DX, DY, color='k', density=1.5)

# 标记不动点
plt.scatter([0, 1, -1], [0, 0, 0], c='r', s=100, label='Fixed Points', zorder=5)

plt.xlabel('x')
plt.ylabel('y')
plt.title('Phase Plane with Nullclines')
plt.legend()
plt.grid()
plt.show()