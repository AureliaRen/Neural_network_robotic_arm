import numpy as np
import matplotlib.pyplot as plt

h = 1.0  # 假设支撑点高度h=1
l0 = np.linspace(0, 2, 1000)
x_stable_low = np.zeros_like(l0[l0 < h])
x_stable_high = np.sqrt(l0[l0 >= h]**2 - h**2)
x_unstable = np.zeros_like(l0[l0 >= h])

plt.figure(figsize=(8, 5))
plt.plot(l0[l0 < h], x_stable_low, 'b-', label='Stable')
plt.plot(l0[l0 >= h], x_stable_high, 'b-')
plt.plot(l0[l0 >= h], -x_stable_high, 'b-')
plt.plot(l0[l0 >= h], x_unstable, 'r--', label='Unstable')
plt.xlabel('$l_0/h$'), plt.ylabel('$x$')
plt.title('Bifurcation Diagram ($m=0$)')
plt.legend(), plt.grid()
plt.show()