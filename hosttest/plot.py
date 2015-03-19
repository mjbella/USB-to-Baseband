#!/usr/bin/env python
# plot.py - plot FIR filter response
import ctypes
import scipy
from scipy import signal
import matplotlib
from matplotlib import pyplot as plt
import numpy as np

# Open our shared library
l = ctypes.CDLL("./streamtest.so")

# Get the number of taps
n_taps = ctypes.c_int.in_dll(l, "taps_len").value
# Create a type to read the taps (array of floats)
taps_type = ctypes.c_float * n_taps
# Read out the taps (and convert to python-native list type)
taps = list(taps_type.in_dll(l, "taps"))

# Use SciPy's frequency analysis for digital filters
w, h = signal.freqz(taps)

# Make our plot figure
fig = plt.figure()
plt.title('FIR filter frequency response')
ax1 = fig.add_subplot(111)

# Plot our signals; w = angle (rad)
# h = complex gain, so we take the magnitude of it (abs) and log-scale it to dB
plt.plot(w, 20 * np.log10(abs(h)), 'b')
# Label our axes
plt.ylabel('Amplitude [dB]', color='b')
plt.xlabel('Frequency [rad/sample]')

# graphy graphy graph graph
ax2 = ax1.twinx()
angles = np.unwrap(np.angle(h))
plt.plot(w, angles, 'g')
plt.ylabel('Angle (radians)', color='g')
plt.grid()
plt.axis('tight')

# Save plot to file
plt.savefig("pyplot.png")

# this makes a popup of the figure, comment this out to disable it
plt.show()
