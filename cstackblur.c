// The Stack Blur Algorithm was invented by Mario Klingemann,
// mario@quasimondo.com and described here:
// http://incubator.quasimondo.com/processing/fast_blur_deluxe.php

// This original C++ RGBA (32 bit color) multi-threaded version
// by Victor Laskin (victor.laskin@gmail.com) could be found here:
// http://vitiy.info/stackblur-algorithm-multi-threaded-blur-for-cpp

// This file is porting from C++ multi-threaded version
// by Jones Chi (duguschi@gmail.com)
#include <stdio.h>
#include <Python.h>
#ifndef _WIN32
#include <pthread.h>
#endif

static unsigned short const stackblur_mul[255] =
{
    512,512,456,512,328,456,335,512,405,328,271,456,388,335,292,512,
    454,405,364,328,298,271,496,456,420,388,360,335,312,292,273,512,
    482,454,428,405,383,364,345,328,312,298,284,271,259,496,475,456,
    437,420,404,388,374,360,347,335,323,312,302,292,282,273,265,512,
    497,482,468,454,441,428,417,405,394,383,373,364,354,345,337,328,
    320,312,305,298,291,284,278,271,265,259,507,496,485,475,465,456,
    446,437,428,420,412,404,396,388,381,374,367,360,354,347,341,335,
    329,323,318,312,307,302,297,292,287,282,278,273,269,265,261,512,
    505,497,489,482,475,468,461,454,447,441,435,428,422,417,411,405,
    399,394,389,383,378,373,368,364,359,354,350,345,341,337,332,328,
    324,320,316,312,309,305,301,298,294,291,287,284,281,278,274,271,
    268,265,262,259,257,507,501,496,491,485,480,475,470,465,460,456,
    451,446,442,437,433,428,424,420,416,412,408,404,400,396,392,388,
    385,381,377,374,370,367,363,360,357,354,350,347,344,341,338,335,
    332,329,326,323,320,318,315,312,310,307,304,302,299,297,294,292,
    289,287,285,282,280,278,275,273,271,269,267,265,263,261,259
};

static unsigned char const stackblur_shr[255] =
{
    9, 11, 12, 13, 13, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17,
    17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21,
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24
};

void stackblurJob(PyObject *obj,      ///< list format of input image data
                  unsigned int w,     ///< image width
                  unsigned int h,     ///< image height
                  unsigned int radius,///< blur intensity (should be in 2..254 range)
                  int cores,          ///< total number of working threads
                  int core,           ///< current thread number
                  int step,           ///< step of processing (1,2)
                  unsigned char *stack///< stack buffer
                  )
{
    unsigned int x, y, xp, yp, i;
    unsigned int sp;
    unsigned int stack_start;
    unsigned char *stack_ptr;

    long src_index;
    long dst_index;
    long src_value;

    unsigned long sum;
    unsigned long sum_in;
    unsigned long sum_out;

    unsigned int wm = w - 1;
    unsigned int hm = h - 1;
    unsigned int div = (radius * 2) + 1;
    unsigned int mul_sum = stackblur_mul[radius];
    unsigned char shr_sum = stackblur_shr[radius];

    if (step == 1) {
        int minY = core * h / cores;
        int maxY = (core + 1) * h / cores;

        for (y = minY; y < maxY; y++) {
            sum = sum_in = sum_out = 0;

            src_index = w * y;
            src_value = PyInt_AsLong(PyList_GetItem(obj, src_index));

            for (i = 0; i <= radius; i++) {
                stack_ptr = &stack[i];
                stack_ptr[0] = src_value;
                sum += src_value * (i + 1);
                sum_out += src_value;
            }


            for (i = 1; i <= radius; i++) {
                if (i <= wm) src_index += 1;
                src_value = PyInt_AsLong(PyList_GetItem(obj, src_index));
                stack_ptr = &stack[i + radius];
                stack_ptr[0] = src_value;
                sum += src_value * (radius + 1 - i);
                sum_in += src_value;
            }


            sp = radius;
            xp = radius;
            if (xp > wm) xp = wm;
            src_index = xp + y * w; // img.pix_ptr(xp, y);
            dst_index = y * w; // img.pix_ptr(0, y);
            for (x = 0; x < w; x++) {
                PyList_SetItem(obj, dst_index, PyInt_FromLong((sum * mul_sum) >> shr_sum));
                dst_index += 1;

                sum -= sum_out;

                stack_start = sp + div - radius;
                if (stack_start >= div) stack_start -= div;
                stack_ptr = &stack[stack_start];

                sum_out -= stack_ptr[0];

                if (xp < wm) {
                    src_index ++;
                    ++xp;
                }

                src_value = PyInt_AsLong(PyList_GetItem(obj, src_index));
                stack_ptr[0] = src_value;

                sum_in += src_value;
                sum += sum_in;

                ++sp;
                if (sp >= div) sp = 0;
                stack_ptr = &stack[sp];

                sum_out += stack_ptr[0];
                sum_in  -= stack_ptr[0];
            }
        }
    }

    // step 2
    if (step == 2) {
        int minX = core * w / cores;
        int maxX = (core + 1) * w / cores;

        for (x = minX; x < maxX; x++) {
            sum = sum_in = sum_out = 0;

            src_index = x; // x,0
            src_value = PyInt_AsLong(PyList_GetItem(obj, src_index));
            for (i = 0; i <= radius; i++) {
                stack_ptr = &stack[i];
                stack_ptr[0] = src_value;
                sum += src_value * (i + 1);
                sum_out += src_value;
            }
            for (i = 1; i <= radius; i++) {
                if (i <= hm) src_index += w; // +stride

                src_value = PyInt_AsLong(PyList_GetItem(obj, src_index));
                stack_ptr = &stack[i + radius];
                stack_ptr[0] = src_value;
                sum += src_value * (radius + 1 - i);
                sum_in += src_value;
            }

            sp = radius;
            yp = radius;
            if (yp > hm) yp = hm;
            src_index = x + yp * w; // img.pix_ptr(x, yp);
            dst_index = x; // img.pix_ptr(x, 0);
            for (y = 0; y < h; y++) {
                PyList_SetItem(obj, dst_index, PyInt_FromLong((sum * mul_sum) >> shr_sum));
                dst_index += w;

                sum -= sum_out;

                stack_start = sp + div - radius;
                if (stack_start >= div) stack_start -= div;
                stack_ptr = &stack[stack_start];

                sum_out -= stack_ptr[0];

                if (yp < hm) {
                    src_index += w; // stride
                    ++yp;
                }

                src_value = PyInt_AsLong(PyList_GetItem(obj, src_index));
                stack_ptr[0] = src_value;

                sum_in += src_value;
                sum += sum_in;

                ++sp;
                if (sp >= div) sp = 0;
                stack_ptr = &stack[sp];

                sum_out += stack_ptr[0];
                sum_in -= stack_ptr[0];
            }
        }
    }
}

#ifndef _WIN32
struct WorkerData
{
    PyObject *obj;
    unsigned int w;
    unsigned int h;
    unsigned int radius;
    unsigned int cores;
    unsigned int core;
    unsigned int step;
    unsigned char *stack;
};

void worker_func(void *data)
{
    struct WorkerData *d = data;
    stackblurJob(d->obj, d->w, d->h, d->radius, d->cores, d->core, d->step, d->stack);
}
#endif

static PyObject *
cstackblur_stackblur(PyObject *self, PyObject *args)
{
    PyObject *obj;
    unsigned int w, h, radius, cores;
    if (!PyArg_ParseTuple(args, "OIIII", &obj, &w, &h, &radius, &cores)) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    if (!PyList_Check(obj)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    unsigned int div = (radius * 2) + 1;

#ifndef _WIN32
    if (cores == 1) {
#endif
        unsigned char *stack = malloc(div);
        // no multithreading
        stackblurJob(obj, w, h, radius, 1, 0, 1, stack);
        stackblurJob(obj, w, h, radius, 1, 0, 2, stack);
        free(stack);
#ifndef _WIN32
    } else {
        unsigned char *stack = malloc(div * cores);
        pthread_t ids[cores];
        struct WorkerData data[cores];
        int i;

        for (i = 0; i < cores; i++) {
            data[i].obj = obj;
            data[i].w = w;
            data[i].h = h;
            data[i].radius = radius;
            data[i].cores = cores;
            data[i].core = i;
            data[i].step = 1;
            data[i].stack = stack + div * i;
            int ret = pthread_create(&ids[i], NULL, (void *)worker_func, &data[i]);
            if (ret != 0) {
                Py_INCREF(Py_None);
                return Py_None;
            }
        }

        for (i = 0; i < cores; i++) {
            pthread_join(ids[i], NULL);
        }

        for (i = 0; i < cores; i++) {
            data[i].step = 2;
            int ret = pthread_create(&ids[i], NULL, (void *)worker_func, &data[i]);
            if (ret != 0) {
                Py_INCREF(Py_None);
                return Py_None;
            }
        }

        for (i = 0; i < cores; i++) {
            pthread_join(ids[i], NULL);
        }
        free(stack);
    }
#endif
    return Py_BuildValue("O", obj);
}

static PyMethodDef StackBlurMethods[] = {
    {"stackblur", cstackblur_stackblur, METH_VARARGS,
     "Blur the image by Stack Blur."},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initcstackblur(void)
{
    (void) Py_InitModule("cstackblur", StackBlurMethods);
}
