pillow-stackblur
================

The Stack Blur filter for Pillow.

The Stack Blur Algorithm was invented by Mario Klingemann,
mario@quasimondo.com and described here:
http://incubator.quasimondo.com/processing/fast_blur_deluxe.php

This original C++ RGBA (32 bit color) multi-threaded version
by Victor Laskin (victor.laskin@gmail.com) could be found here:
http://vitiy.info/stackblur-algorithm-multi-threaded-blur-for-cpp

The python implementation is porting from C++ multi-threaded version.
And wrap the implementation as a filter for pillow.

Example:
--------
Blur the image with radius 10.
```python
from stackblur import StackBlur
from PIL import Image
im = Image.open('img.png')
im = im.filter(StackBlur(10))
im.save('blurred_img.png')
```

Installation:
-------------
Install via setup tools:

    ./setup.py install

Install via PIP:

    pip install -e git://github.com/JonesChi/pillow-stackblur.git#egg=pillow-stackblur

Please remember to install `pillow` before using this library.
