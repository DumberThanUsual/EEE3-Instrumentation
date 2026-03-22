# plot_bin.py
import numpy as np
import matplotlib.pyplot as plt

data = np.fromfile("data.bin", dtype=np.int16)  # adjust dtype to your buffer type
adc1 = data[0::2]  # Assuming interleaved ADC1 and ADC2 samples
adc2 = data[1::2]
plt.plot(adc1 - 2048, label='ADC1')
plt.plot(adc2 - 2048, label='ADC2', linestyle='dotted')
plt.title("sample_buffer")
plt.xlabel("Index")
plt.ylabel("Value")
plt.grid(True)
plt.legend()
plt.show()