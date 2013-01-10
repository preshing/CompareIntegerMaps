#---------------------------------------------------
# Reads experiment results from results.txt, and renders some graphs to .png files.
# You can filter the graphs to render by filename by passing a regular expression as a script argument.
# For example: make_graphs.py lookup.png
#---------------------------------------------------

import os
import re
import sys
import cairo
import math
from pprint import pprint
import cairo


# Smoothing looks good but runs slowly in Python, so you can disable it by specifying --nosmooth
SMOOTHING_ENABLED = True


#---------------------------------------------------
#  Cairo drawing helpers
#---------------------------------------------------
def createScaledFont(family, size, slant=cairo.FONT_SLANT_NORMAL, weight=cairo.FONT_WEIGHT_NORMAL):
    """ Simple helper function to create a cairo ScaledFont. """
    face = cairo.ToyFontFace(family, slant, weight)
    DEFAULT_FONT_OPTIONS = cairo.FontOptions()
    DEFAULT_FONT_OPTIONS.set_antialias(cairo.ANTIALIAS_SUBPIXEL)
    return cairo.ScaledFont(face, cairo.Matrix(xx=size, yy=size), cairo.Matrix(), DEFAULT_FONT_OPTIONS)

def fillAlignedText(cr, x, y, scaledFont, text, alignment = 0):
    """ Draw some aligned text at the specified co-ordinates.
    alignment = 0: left-justify
    alignment = 0.5: center
    alignment = 1: right-justify """
    ascent, descent = scaledFont.extents()[:2]
    x_bearing, y_bearing, width, height = scaledFont.text_extents(text)[:4]
    with Saved(cr):
        cr.set_scaled_font(scaledFont)
        cr.move_to(math.floor(x + 0.5 - width * alignment), math.floor(y + 0.5))
        cr.text_path(text)
        cr.fill()

class Saved():
    """ Preserve cairo state inside the scope of a with statement. """
    def __init__(self, cr):
        self.cr = cr
    def __enter__(self):
        self.cr.save()
        return self.cr
    def __exit__(self, type, value, traceback):
        self.cr.restore()


#---------------------------------------------------
#  Utility funcs
#---------------------------------------------------
def floatrange(lo, hi, step):
    """ Like Python's xrange(), but with floats. """
    sign = 1.0 if step >= 0 else -1.0
    i = 0
    while 1:
        x = lo + i * step
        if x * sign >= hi:
            break
        yield x
        i += 1
    
def smoothPoints(inputs, points, k = 1):
    """ Smoothly resamples the curve defined by points.
    inputs: Specifies x co-ordinates of new points.
    k: Amount of smoothing. """
    k = 1.0 / k
    points = list(points)
    for x in inputs:
        sum = 0.0
        weight = 0.0
        for tx, ty in points:
            diff = tx - x
            exp = k * diff * diff
            if exp > 20:
                continue
            w = math.exp(-exp)
            sum += ty * w
            weight += w
        yield x, sum / weight if weight > 1e-9 else 0


#---------------------------------------------------
#  AxisAttribs
#---------------------------------------------------
class AxisAttribs:
    """ Describes one axis on the graph. Can be linear or logarithmic. """
    
    def __init__(self, size, min, max, step, logarithmic, labeler = lambda x: str(int(x + 0.5))):
        self.size = float(size)
        self.logarithmic = logarithmic
        self.labeler = labeler
        self.toAxis = lambda x: math.log(x) if logarithmic else float(x)
        self.fromAxis = lambda x: math.exp(x) if logarithmic else float(x)
        self.min = self.toAxis(min)
        self.max = self.toAxis(max)
        self.step = self.toAxis(step)

    def mapAxisValue(self, x):
        """ Maps x to a point along the axis.
        x should already have been filtered through self.toAxis(), especially if logarithmic. """
        return (x - self.min) / (self.max - self.min) * self.size
    
    def iterLabels(self):
        """ Helper to iterate through all the tick marks along the axis. """
        lo = int(math.floor(self.min / self.step + 1 - 1e-9))
        hi = int(math.floor(self.max / self.step + 1e-9))
        for i in xrange(lo, hi + 1):
            value = i * self.step
            if self.min == 0 and i == 0:
                continue
            yield self.mapAxisValue(value), self.labeler(self.fromAxis(value))


#---------------------------------------------------
#  Graph
#---------------------------------------------------
class Graph:
    """ Renders a graph. """
    
    def __init__(self, filename, yaxis):
        self.filename = filename
        self.yaxis = yaxis
        self.xaxis = 'Population'
        self.xattribs = AxisAttribs(400, 55, 18000000, 10, True)
        self.yattribs = AxisAttribs(150, 0, 500, 100, False, lambda x: '%d ns' % int(x + 0.5))
        self.curves = []
        self.smoothing = True
        self.small = False
        self.xlabelshift = 0

    def addSmoothCurve(self, label, color, results, resultName, labelNudge = 0, width = None):
        if resultName not in results:
            print('*** %s is missing' % resultName)
            return
        points = results[resultName]
        if 'MEMORY' in resultName:
            points = [(x, y / x) for x, y in points]
        xattribs = self.xattribs
        yattribs = self.yattribs
        points = [(xattribs.toAxis(x), yattribs.toAxis(y)) for x, y in points]
        if self.smoothing and SMOOTHING_ENABLED:
            points = list(smoothPoints(floatrange(xattribs.min, xattribs.max, .01), points, k=.0005))
        if not width:
            width = 1.2 if self.small else 2.5
        self.curves.append((label, color, points, width, labelNudge))

    def render(self):
        xattribs = self.xattribs
        yattribs = self.yattribs

        # Create the image surface and cairo context
        if self.small:
            surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 46 + int(xattribs.size + 0.5), 69 + int(yattribs.size + 0.5))
        else:
            surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 116 + int(xattribs.size + 0.5) - self.xlabelshift, 65 + int(yattribs.size + 0.5))
        cr = cairo.Context(surface)
        cr.set_source_rgb(1, 1, 1)
        cr.paint()
        cr.set_miter_limit(1.414)
        if self.small:
            cr.translate(40, 26 + yattribs.size)
        else:
            cr.translate(58 - self.xlabelshift, 11 + yattribs.size)

        # Draw axes
        labelFont = createScaledFont('Arial', 11)
        with Saved(cr):
            cr.set_line_width(1)
            cr.set_source_rgb(.4, .4, .4)

            # Horizontal axis
            cr.move_to(0, -0.5)
            cr.rel_line_to(xattribs.size + 1, 0)
            for pos, label in xattribs.iterLabels():    # Tick marks
                x = math.floor(pos + 0.5) + 0.5
                cr.move_to(x, -1)
                cr.rel_line_to(0, 4)
            cr.stroke()
            for pos, label in xattribs.iterLabels():    # Labels
                x = math.floor(pos + 0.5)
                with Saved(cr):
                    cr.translate(x - 1, 5)
                    cr.rotate(-math.pi / 4)
                    fillAlignedText(cr, 0, 6, labelFont, label, 1)

            # Vertical axis
            cr.move_to(0.5, 0)
            cr.rel_line_to(0, -yattribs.size - 0.5)
            for pos, label in yattribs.iterLabels():    # Tick marks
                if label == '0':
                    continue
                y = -math.floor(pos + 0.5) - 0.5
                cr.move_to(1, y)
                cr.rel_line_to(-4, 0)
            cr.stroke()
            for pos, label in yattribs.iterLabels():    # Labels
                if label == '0':
                    continue
                fillAlignedText(cr, -4, -pos + 4, labelFont, label, 1)

        # Draw curves
        for label, color, points, width, labelNudge in self.curves:
            with Saved(cr):
                cr.set_line_width(width)
                cr.set_source_rgba(*color)
                with Saved(cr):
                    cr.rectangle(0, 5, xattribs.size, -yattribs.size - 15)
                    cr.clip()
                    cr.move_to(xattribs.mapAxisValue(points[0][0]), -yattribs.mapAxisValue(points[0][1]))
                    for x, y in points[1:]:
                        cr.line_to(xattribs.mapAxisValue(x) + 0.5, -yattribs.mapAxisValue(y) - 0.5)
                    cr.stroke()

                # Label
                if not self.small:
                    labelFont = createScaledFont('Arial', 11)
                    x, y = points[-1]
                    fillAlignedText(cr, xattribs.mapAxisValue(x) + 3, -yattribs.mapAxisValue(y) + 4 + labelNudge, labelFont, label, 0)

        # Draw axis names
        if self.small:
            cr.set_source_rgb(0, 0, 0)
            axisFont = createScaledFont('Helvetica', 14, weight=cairo.FONT_WEIGHT_BOLD)
            fillAlignedText(cr, xattribs.size / 2.0, -yattribs.size - 15, axisFont, self.yaxis, 0.5)
        else:
            cr.set_source_rgb(0, 0, 0)
            axisFont = createScaledFont('Helvetica', 14, weight=cairo.FONT_WEIGHT_BOLD)
            with Saved(cr):
                cr.translate(-47 + self.xlabelshift, -yattribs.size / 2.0)
                cr.rotate(-math.pi / 2)
                fillAlignedText(cr, 0, 0, axisFont, self.yaxis, 0.5)
            fillAlignedText(cr, xattribs.size / 2.0, 50, axisFont, self.xaxis, 0.5)

        # Save PNG file
        surface.write_to_png(self.filename)


#---------------------------------------------------
#  main
#---------------------------------------------------
if __name__ == '__main__':
    from datetime import datetime
    start = datetime.now()
    
    os.chdir(os.path.split(sys.argv[0])[0])
    filter = re.compile((sys.argv + ['.*'])[1])
    if '--nosmooth' in sys.argv[1:]:
        SMOOTHING_ENABLED = False
    results = eval(open('results.txt', 'r').read())

    graph = Graph('lookup.png', 'Lookup Time')
    if filter.match(graph.filename):
        print('Rendering %s...' % graph.filename)
        graph.addSmoothCurve('Hash Table', (1, .4, .4), results, 'LOOKUP_0_TABLE')
        graph.addSmoothCurve('Judy Array', (.4, .4, .9), results, 'LOOKUP_0_JUDY')
        graph.render()

    graph = Graph('insert.png', 'Insert Time')
    if filter.match(graph.filename):
        print('Rendering %s...' % graph.filename)
        graph.addSmoothCurve('Hash Table', (1, .4, .4), results, 'INSERT_0_TABLE')
        graph.addSmoothCurve('Judy Array', (.4, .4, .9), results, 'INSERT_0_JUDY')
        graph.render()

    graph = Graph('lookup-cache-stomp.png', 'Lookup Times')
    if filter.match(graph.filename):
        print('Rendering %s...' % graph.filename)
        graph.xattribs.size = 220
        graph.yattribs.size = 100
        graph.small = True
        #graph.addSmoothCurve('', (1, .4, .4, .5), results, 'LOOKUP_0_TABLE', width=0.5)
        graph.addSmoothCurve('', (1, .4, .4, .6), results, 'LOOKUP_1000_TABLE')
        graph.addSmoothCurve('', (1, .4, .4), results, 'LOOKUP_10000_TABLE', width=1.8)
        #graph.addSmoothCurve('', (.4, .4, .9, .5), results, 'LOOKUP_0_JUDY', width=0.35)
        graph.addSmoothCurve('', (.4, .4, .9, .6), results, 'LOOKUP_1000_JUDY')
        graph.addSmoothCurve('', (.4, .4, .9), results, 'LOOKUP_10000_JUDY', width=1.8)
        graph.render()

    graph = Graph('insert-cache-stomp.png', 'Insert Times')
    if filter.match(graph.filename):
        print('Rendering %s...' % graph.filename)
        graph.xattribs.size = 220
        graph.yattribs.size = 100
        graph.small = True
        #graph.addSmoothCurve('', (1, .4, .4, .5), results, 'INSERT_0_TABLE', width=0.5)
        graph.addSmoothCurve('', (1, .4, .4, .6), results, 'INSERT_1000_TABLE')
        graph.addSmoothCurve('', (1, .4, .4), results, 'INSERT_10000_TABLE', width=1.8)
        #graph.addSmoothCurve('', (.4, .4, .9, .5), results, 'INSERT_0_JUDY', width=0.5)
        graph.addSmoothCurve('', (.4, .4, .9, .6), results, 'INSERT_1000_JUDY')
        graph.addSmoothCurve('', (.4, .4, .9), results, 'INSERT_10000_JUDY', width=1.8)
        graph.render()

    graph = Graph('memory.png', 'Total Bytes Per Item')
    if filter.match(graph.filename):
        print('Rendering %s...' % graph.filename)
        graph.yattribs = AxisAttribs(150, 0, 25, 5, False)
        graph.smoothing = False
        graph.xlabelshift = 21
        graph.addSmoothCurve('Hash Table', (1, .4, .4), results, 'MEMORY_TABLE', -3)
        graph.addSmoothCurve('Judy Array', (.4, .4, .9), results, 'MEMORY_JUDY')
        graph.render()
    
    print('Elapsed time: %s' % (datetime.now() - start))
