import pandas as pd

# 读取原始数据
df = pd.read_excel("33.xlsx", sheet_name="Sheet1")
points = df.values.tolist()

# 处理数据：递归保留间距大于 0.002 的点
processed_points = []
if points:
    processed_points.append(points[0])
    last_point = points[0]
    for point in points[1:]:
        dx = point[0] - last_point[0]
        dy = point[1] - last_point[1]
        distance = (dx ** 2 + dy ** 2) ** 0.5
        if distance > 0.002:
            processed_points.append(point)
            last_point = point
new_point=[]
if len(processed_points)%2==0:
    dx =2*((processed_points[len(processed_points)-1])[0])-processed_points[len(processed_points)-2][0]
    dy = 2*((processed_points[len(processed_points) - 1])[1])- processed_points[len(processed_points) - 2][1]
    new_point.append(dx)
    new_point.append(dy)
    processed_points.append(new_point)
# 生成新表格
result_df = pd.DataFrame(processed_points, columns=["x", "y"])
result_df.to_excel("new_processed_data3.xlsx", index=False)