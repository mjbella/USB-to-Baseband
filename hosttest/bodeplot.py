#!/usr/bin/env python

import ctypes

l = ctypes.CDLL("./streamtest.so")

print l.barf()
