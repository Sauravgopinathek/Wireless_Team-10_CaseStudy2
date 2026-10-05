"""
plot_part3.py - plots the files written by part3_tcp_wifi_mobile.cc
Run from the folder that contains design1-*.dat and design2-*.dat:
    pip install matplotlib numpy
    python plot_part3.py
Creates part3_results.png (throughput + distance, and cwnd, for both designs).
"""
import numpy as np
import matplotlib.pyplot as plt

designs = [("design1", "Design 1: static STA"), ("design2", "Design 2: mobile STA")]
fig, axes = plt.subplots(2, 2, figsize=(13, 8))

for col, (tag, title) in enumerate(designs):
    thr = np.loadtxt(f"{tag}-throughput.dat", ndmin=2)  # time, distance, Mbps
    cw = np.loadtxt(f"{tag}-cwnd.dat", ndmin=2)         # time, cwnd bytes

    ax = axes[0][col]
    ax.plot(thr[:, 0], thr[:, 2], color="tab:blue", label="Throughput")
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("TCP throughput (Mbps)", color="tab:blue")
    ax.set_title(title)
    ax.grid(alpha=0.3)
    ax2 = ax.twinx()
    ax2.plot(thr[:, 0], thr[:, 1], color="tab:red", linestyle="--", label="Distance")
    ax2.set_ylabel("Distance STA-AP (m)", color="tab:red")

    ax = axes[1][col]
    ax.plot(cw[:, 0], cw[:, 1] / 1000.0, color="tab:green")
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Congestion window (KB)")
    ax.set_title(title + " - cwnd")
    ax.grid(alpha=0.3)

plt.tight_layout()
plt.savefig("part3_results.png", dpi=200)
print("saved part3_results.png")