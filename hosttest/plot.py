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
h_dB = 20 * np.log10(abs(h))  # Magnitude Response
h_Phase = np.unwrap(np.angle(h))  # Phase Response

# Make our plot figure
fig = plt.figure()
plt.title('FIR filter frequency response')
ax1 = fig.add_subplot(111)

# Plot Magnitude Response
plt.plot(w, h_dB, 'b')
plt.ylabel('Amplitude [dB]', color='b')
plt.xlabel('Frequency [rad/sample]')

# Plot Phase Response
ax2 = ax1.twinx()
plt.plot(w, h_Phase, 'g')
plt.ylabel('Angle (radians)', color='g')
plt.grid()
plt.axis('tight')

# Save plot to file
plt.savefig("pyplot.png")

# Show the figure in a window, comment this out to disable it
plt.show()
