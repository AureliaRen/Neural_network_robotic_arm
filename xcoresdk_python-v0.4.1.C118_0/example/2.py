# # # # # import numpy as np
# # # # # import matplotlib.pyplot as plt
# # # # #
# # # # # # 定义网格范围
# # # # # x = np.linspace(-1.5, 1.5, 100)
# # # # # y = np.linspace(-1.5, 1.5, 100)
# # # # # X, Y = np.meshgrid(x, y)
# # # # #
# # # # # # 计算零斜率线
# # # # # F_xnull = Y * (1 + (1 - X**2) * (Y**2 - X**2*(1 - X**2/2)))
# # # # # F_ynull = X*(1 - X**2) - Y*(Y**2 - X**2*(1 - X**2/2))
# # # # #
# # # # # # 绘图设置
# # # # # plt.figure(figsize=(10, 8))
# # # # # plt.contour(X, Y, F_xnull, levels=[0], colors='blue', linestyles='solid', linewidths=2)
# # # # # plt.contour(X, Y, F_ynull, levels=[0], colors='red', linestyles='dashed', linewidths=2)
# # # # #
# # # # # # 向量场计算
# # # # # DX = Y + Y*(1 - X**2)*(Y**2 - X**2*(1 - X**2/2))
# # # # # DY = X*(1 - X**2) - Y*(Y**2 - X**2*(1 - X**2/2))
# # # # #
# # # # # # 绘制流线
# # # # # plt.streamplot(X, Y, DX, DY, color='k', density=1.5)
# # # # #
# # # # # # 标记不动点
# # # # # plt.scatter([0, 1, -1], [0, 0, 0], c='r', s=100, label='Fixed Points', zorder=5)
# # # # #
# # # # # # 创建图例
# # # # # plt.plot([], [], 'b-', label='x-Nullclines')
# # # # # plt.plot([], [], 'r--', label='y-Nullclines')
# # # # # plt.legend(loc='upper right')
# # # # #
# # # # # plt.xlabel('x')
# # # # # plt.ylabel('y')
# # # # # plt.title('Phase Plane with Nullclines')
# # # # # plt.grid()
# # # # # plt.show()
# # # # import matplotlib.pyplot as plt
# # # # import numpy as np
# # # #
# # # # # 定义特征和性别数据
# # # # features = ("Adelie", "Chinstrap", "Gentoo")
# # # # male = [73, 34, 61]          # 雄性数据
# # # # female = [73, 34, 58]        # 雌性数据
# # # # male_std = [4, 2, 3]         # 雄性误差范围
# # # # female_std = [3, 2, 2]       # 雌性误差范围
# # # #
# # # # y = np.arange(len(features)) # 特征标签位置
# # # # width = 0.4                  # 柱状图宽度
# # # #
# # # # fig, ax = plt.subplots(figsize=(8, 4), layout='constrained')
# # # #
# # # # # 绘制雄性柱状图（带误差条）
# # # # male_bars = ax.barh(
# # # #     y - width/2,
# # # #     male,
# # # #     width,
# # # #     color='#008B8B',
# # # #     label='Male',
# # # #     xerr=male_std,
# # # #     capsize=5
# # # # )
# # # #
# # # # # 绘制雌性柱状图（带误差条）
# # # # female_bars = ax.barh(
# # # #     y + width/2,
# # # #     female,
# # # #     width,
# # # #     color='chocolate',
# # # #     label='Female',
# # # #     xerr=female_std,
# # # #     capsize=5
# # # # )
# # # #
# # # # # 添加数据标签
# # # # ax.bar_label(male_bars, padding=5, fmt='%d', fontsize=9)
# # # # ax.bar_label(female_bars, padding=5, fmt='%d', fontsize=9)
# # # #
# # # # # 坐标轴和标题设置
# # # # ax.set_xlabel('number', fontsize=10)
# # # # ax.set_ylabel('Features', fontsize=10)
# # # # ax.set_yticks(y, features)
# # # # ax.set_title('I maybe do not like Penguin',
# # # #             fontsize=12, pad=15)
# # # # ax.legend(loc='upper right', frameon=False)
# # # # ax.set_xlim(0, 90)  # 根据数据最大值+误差范围调整
# # # # plt.show()
# # #
# # #
# # #
# # # import numpy as np
# # # import matplotlib.pyplot as plt
# # # from pylab import mpl
# # # mpl.rcParams['font.sans-serif'] = ['SimHei']        # 指定默认字体为黑体
# # # mpl.rcParams['axes.unicode_minus'] = False      # 解决将负号'-'显示为方块的问题
# # # x1 = np.linspace(0.0, 5.0, 50)  # 50个点，用于更平滑的曲线
# # # x2 = np.linspace(0.0, 2.0, 30)  # 30个点，用于稀疏的离散数据
# # # y1 = np.cos(2 * np.pi * x1) * np.exp(-x1)
# # # y2 = np.cos(2 * np.pi * x2)
# # #
# # # # Step2：新增第三个子图的函数
# # # x3 = np.linspace(-2, 2, 100)
# # # y3 = x3*np.sin(5*x3)  # 自定义函数：x²·sin(5x)
# # #
# # # # 创建3个子图（3行1列布局）
# # # fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(8, 10), layout='constrained')
# # # #1
# # # ax1.plot(x1, y1, 'bs--', markersize=4, linewidth=1, label='Damped')
# # # ax1.set_title('阻尼振荡曲线')
# # # ax1.set_ylabel('幅值')
# # # ax1.grid(True, linestyle=':')  # 添加网格线
# # # ax1.legend()
# # # #2
# # # ax2.plot(x2, y2, 'g^:', markersize=6, linewidth=1.5, label='Undamped')
# # # ax2.set_title('未阻尼振荡曲线')
# # # ax2.set_ylabel('幅值')
# # # ax2.set_xlabel('时间 (秒)')
# # # ax2.legend()
# # # #3
# # # ax3.plot(x3, y3, 'r-', linewidth=2, label='x·sin(5x)')
# # # ax3.set_title('自定义')
# # # ax3.set_xlabel('x轴')
# # # ax3.set_ylabel('y轴')
# # # ax3.set_ylim(-5, 5)  # 限制y轴范围以突出波动
# # # ax3.legend()
# # #
# # # plt.show()
# #
# # import numpy as np
# # import matplotlib.pyplot as plt
# # from pylab import mpl
# #
# # # 设置字体属性，确保中文正常显示
# # mpl.rcParams['font.sans-serif'] = ['SimHei']  # 使用黑体
# # mpl.rcParams['axes.unicode_minus'] = False    # 解决负号显示问题
# #
# # # 表1中的数据
# # periods = ['十五', '十一五', '十二五', '十三五', '十四五']  # 时期列表
# # bridge_counts = [8, 13, 16, 17, 17]  # 桥梁数量列表
# #
# # # 计算桥梁总数量
# # total_bridges = sum(bridge_counts)
# #
# # # 创建两个子图
# # fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 6), layout='constrained')
# #
# # # 第一个子图：饼图
# # ax1.pie(
# #     bridge_counts,
# #     labels=periods,
# #     autopct='%1.2f%%',  # 保留两位小数的百分比
# #     startangle=90,      # 起始角度
# #     shadow=True         # 添加阴影效果
# # )
# # ax1.set_title('贵州省各时期桥梁建成比例')
# #
# # # 第二个子图：折线图
# # ax2.plot(periods, bridge_counts, 'ro-', linewidth=2, markersize=8)  # 红色圆点标记+实线
# # ax2.set_title('贵州省各时期桥梁建成数量')
# # ax2.set_xlabel('建成时期')
# # ax2.set_ylabel('数量（座）')
# # ax2.grid(True, linestyle='--', alpha=0.7)  # 添加网格线
# #
# # # 在折线图上添加数据标签
# # for i, count in enumerate(bridge_counts):
# #     ax2.text(i, count + 0.5, str(count),
# #              ha='center', va='bottom', fontsize=10)
# #
# # # 保存图片
# # plt.savefig('task1_result.png', dpi=300)
# #
# # # 显示统计图
# # plt.show()
# #
# # # 打印总数量
# # print(f"贵州省桥梁总数量: {total_bridges}座")
#
#
# import numpy as np
# import matplotlib.pyplot as plt
# from pylab import mpl
#
# # 设置字体属性，确保中文正常显示
# mpl.rcParams['font.sans-serif'] = ['SimHei']  # 使用黑体
# mpl.rcParams['axes.unicode_minus'] = False    # 解决负号显示问题
#
# # 准备数据
# years = [str(i) for i in range(2013, 2025)]  # 生成年份列表
# sales_volume = [1.7, 7.5, 33.1, 50.7, 77.7, 125.6, 120.6, 136.7,
#                 352.1, 688.7, 949.5, 1286.6]  # 销售量（万辆）
# market_share = [0.09, 0.32, 1.3, 1.8, 2.7, 4.5, 4.7, 5.4,
#                 13.4, 27.6, 31.6, 40.9]  # 市场占有率（%）
#
# # 创建上下排列的两个子图
# fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 10), layout='constrained')
#
# # ===== 上半部分：销售量柱状图 =====
# # 创建渐变色
# colors = plt.cm.viridis(np.linspace(0, 1, len(years)))
#
# # 绘制柱状图
# bars = ax1.bar(
#     years,
#     sales_volume,
#     width=0.35,
#     color=colors,
#     edgecolor='black',
#     linewidth=1
# )
#
# # 添加柱状图数据标签
# ax1.bar_label(
#     bars,
#     labels=[f"{sv:.1f}" for sv in sales_volume],
#     padding=3,
#     fontsize=9
# )
#
# # 设置图表属性
# ax1.set_title('张三-新能源汽车年销售量', fontsize=14, pad=20)
# ax1.set_ylabel('销售量（万辆）', fontsize=12)
# ax1.set_xlabel('年份', fontsize=12)
# ax1.set_ylim(0, 1400)  # 设置y轴范围
# ax1.set_yticks(np.arange(0, 1401, 200))  # 设置y轴刻度
# ax1.grid(axis='y', linestyle='--', alpha=0.7)  # 添加横向网格线
# ax1.legend(['销售量'], loc='upper left')  # 添加图例
#
# # ===== 下半部分：市场占有率折线图 =====
# # 绘制折线图
# line, = ax2.plot(
#     years,
#     market_share,
#     'o-',  # 带圆点标记的实线
#     color='#FF6B6B',
#     linewidth=2,
#     markersize=8,
#     markerfacecolor='white',
#     markeredgewidth=1.5
# )
#
# # 添加数据点标签
# for i, (year, ms) in enumerate(zip(years, market_share)):
#     ax2.text(
#         i,
#         ms + 1,  # 标签在数据点上方
#         f"{ms}%",
#         ha='center',
#         va='bottom',
#         fontsize=9,
#         bbox=dict(facecolor='white', alpha=0.8, edgecolor='none', pad=2)
#     )
#
# # 设置图表属性
# ax2.set_title('张三-新能源汽车市场占有率', fontsize=14, pad=20)
# ax2.set_ylabel('市场占有率（%）', fontsize=12)
# ax2.set_xlabel('年份', fontsize=12)
# ax2.set_ylim(0, 45)  # 设置y轴范围
# ax2.set_yticks(np.arange(0, 46, 5))  # 设置y轴刻度
# ax2.grid(axis='y', linestyle='--', alpha=0.7)  # 添加横向网格线
# ax2.legend(['市场占有率'], loc='upper left')  # 添加图例
#
# # 调整子图间距
# plt.subplots_adjust(hspace=0.3)
#
# # 保存图片
# plt.savefig('task2_result.png', dpi=300, bbox_inches='tight')
#
# # 显示图表
# plt.show()
import numpy as np
import matplotlib.pyplot as plt
from pylab import mpl

# 设置字体属性，确保中文正常显示
mpl.rcParams['font.sans-serif'] = ['SimHei']  # 使用黑体
mpl.rcParams['axes.unicode_minus'] = False    # 解决负号显示问题

# 表1中的数据
periods = ['十五', '十一五', '十二五', '十三五', '十四五']  # 时期列表
bridge_counts = [8, 13, 16, 17, 17]  # 各时期建成数量

# 计算累计桥梁数量
cumulative_counts = []
total = 0
for count in bridge_counts:
    total += count
    cumulative_counts.append(total)

# 计算桥梁总数量
total_bridges = sum(bridge_counts)

# 创建两个子图
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6), layout='constrained')

# ===== 第一个子图：饼图（各时期比例） =====
ax1.pie(
    bridge_counts,
    labels=periods,
    autopct='%1.1f%%',          # 保留一位小数的百分比
    startangle=90,              # 起始角度
    shadow=True,                # 添加阴影
    explode=(0, 0.1, 0, 0, 0),  # 突出"十一五"时期
    colors=['#FF9999', '#66B2FF', '#99FF99', '#FFCC99', '#C9A0DC']  # 自定义颜色
)
ax1.set_title('贵州省各时期桥梁建成比例', fontsize=14, pad=15)
ax1.legend(
    title="建设时期",
    loc="center left",
    bbox_to_anchor=(0.9, 0, 0.5, 1),
    fontsize=10
)

# ===== 第二个子图：折线图（累计数量） =====
# 绘制折线图
ax2.plot(
    periods,
    cumulative_counts,
    'D-',  # 菱形标记+实线
    color='#E63946',
    linewidth=2.5,
    markersize=9,
    markerfacecolor='white',
    markeredgewidth=1.5
)

# 添加数据标签
for i, (period, count) in enumerate(zip(periods, cumulative_counts)):
    ax2.text(
        i,
        count + 1.5,
        f"{count}座",
        ha='center',
        va='bottom',
        fontsize=10,
        fontweight='bold',
        bbox=dict(facecolor='white', alpha=0.8, edgecolor='#E63946', boxstyle='round,pad=0.3')
    )

# 设置图表属性
ax2.set_title('贵州省桥梁累计建成数量', fontsize=14, pad=15)
ax2.set_xlabel('建设时期', fontsize=12)
ax2.set_ylabel('累计数量（座）', fontsize=12)
ax2.grid(True, linestyle='--', alpha=0.7)  # 添加网格线
ax2.set_ylim(0, max(cumulative_counts) + 10)  # 设置y轴范围

# 添加总数量标注
ax2.text(
    0.5, -0.15,
    f"桥梁总数量: {total_bridges}座",
    transform=ax2.transAxes,
    ha='center',
    va='center',
    fontsize=12,
    fontweight='bold',
    bbox=dict(facecolor='#F1FAEE', edgecolor='#A8DADC', boxstyle='round,pad=0.5')
)

# 保存图片
plt.savefig('task1_result.png', dpi=300, bbox_inches='tight')

# 显示统计图
plt.show()