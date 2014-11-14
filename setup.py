from distutils.core import setup, Extension

modules = ['stackblur']
ext_modules = Extension('cstackblur', sources = ['cstackblur.c'])

setup(name = 'StackBlur',
      version = '1.0',
      description = 'The filter for Stack Blur',
      author='Jones Chi',
      author_email='duguschi@gmail.com',
      url='https://github.com/JonesChi/pillow-stackblur',
      py_modules = modules,
      ext_modules = [ext_modules])
