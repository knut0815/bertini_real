"""
Dan Hessler
University of Wisconsin, Eau Claire
Fall 2018 - Spring 2019
Porting to Glumpy for faster surface rendering
using OpenGL

.. module:: glumpyplotter
    :platform: Unix, Windows
    :synopsis: The glumpy plotter uses OpenGL to render a decomposed
               surface created using bertini_real.
"""

import math
import numpy as np
from glumpy import app, gl, glm, gloo
from glumpy.transforms import Trackball, Position
import bertini_real

class GlumpyPlotter():
    """ Creates the glumpyplotter object which is used to render
        3d surfaces created from bertini_real
    """

    def __init__(self, data=None):
        """ Reads data from disk if it is not given any

            Args:
                data: the decomposition to be read
        """
        if data is None:
            self.decomposition = bertini_real.data.ReadMostRecent()
        else:
            self.decomposition = data

    def plot(self, cmap=None, color_function=None):
        """ Method used to plot a surface """
        print("Plotting object of dimension: {}".format(self.decomposition.dimension))

        data = self.decomposition
        tuples = data.surface.surface_sampler_data
        points = extract_points(data)

        triangle = []
        for i in range(len(tuples)):
            for tri in tuples[i]:
                x_coordinate = int(tri[0])
                y_coordinate = int(tri[1])
                z_coordinate = int(tri[2])

                point = [x_coordinate, y_coordinate, z_coordinate]
                triangle.append(point)

        triangle = np.asarray(triangle)


# ----------------------------------------------------------------------------- #
#                    Vertex and fragment are OpenGL code
# ----------------------------------------------------------------------------- #

        vertex = """
        attribute vec4 a_color;         // Vertex Color
        uniform mat4   u_model;         // Model matrix
        uniform mat4   u_view;          // View matrix
        uniform mat4   u_projection;    // Projection matrix
        attribute vec3 position;        // Vertex position
        varying vec4   v_color;         // Interpolated fragment color (out)
        void main()
        {
            v_color = a_color;
            gl_Position = <transform>;
        }
        """

        fragment = """
        varying vec4  v_color;          // Interpolated fragment color (in)
        void main()
        {
            gl_FragColor = v_color;
        }
        """

        window = app.Window(width=2048, height=2048,
                            color=(0.30, 0.30, 0.35, 1.00))

        verts = np.zeros(len(points), [("position", np.float32, 3),
                                       ("a_color", np.float32, 4)])
        verts["position"] = points
        verts["a_color"] = make_colors(verts["position"], cmap, color_function)

        verts = verts.view(gloo.VertexBuffer)
        indeces = np.array(triangle).astype(np.uint32)
        indeces = indeces.view(gloo.IndexBuffer)

        surface = gloo.Program(vertex, fragment)
        surface.bind(verts)
        surface['u_model'] = np.eye(4, dtype=np.float32)
        surface['u_view'] = glm.translation(0, 0, -5)
        surface['transform'] = Trackball(Position("position"))
        window.attach(surface['transform'])

        @window.event
        def on_draw(draw_triangles):
            window.clear()

            surface.draw(gl.GL_TRIANGLES, indeces)
            #  surface.draw(gl.GL_LINES, indeces)

        @window.event
        def on_init():
            gl.glEnable(gl.GL_DEPTH_TEST)
            gl.glPolygonOffset(1, 1)
            gl.glEnable(gl.GL_LINE_SMOOTH)
            gl.glBlendFunc(gl.GL_SRC_ALPHA, gl.GL_ONE_MINUS_SRC_ALPHA)

        app.run()

# ----------------------------------------------------------------------------- #

def plot(data=None, cmap='hsv', color_function=None):
    """
    sets default values for colormap if none are specified
    """
    import matplotlib.pyplot as plt

    cmap = plt.get_cmap(cmap)

    surface = GlumpyPlotter(data)
    surface.plot(cmap, color_function)

# ----------------------------------------------------------------------------- #
#                               Helper Methods
# ----------------------------------------------------------------------------- #

def default_color_function(x_coordinate, y_coordinate, z_coordinate):
    """ Helper method for make_colors()
        The default color function that is used to compute
        our points that will then be fed into the colormap.

        :param x: x coordinate of triangle
        :param y: y coordinate of traingle
        :param z: z coordinate of triangle

    """
    return math.sqrt(x_coordinate**2 + y_coordinate**2 + z_coordinate**2)

def make_colors(points, cmap, color_function):
    """ Helper method for plot()
        computes colors according to a function
        then applies a colormap from the matplotlib library

        :param points: The triangles that will be rendered
        :param cmap: Matplotlib colormap to be used
        :param color_function: Function used to compute colors with cmap
    """
    colors = []
    data = []

    # run data through specified color function
    if color_function is None:
        for i in range(len(points)):
            function_result = default_color_function(points[i][0],
                                                     points[i][1],
                                                     points[i][2])
            data.append(function_result)
    else:
        for i in range(len(points)):
            function_result = color_function(points[i][0],
                                             points[i][1],
                                             points[i][2])
            data.append(function_result)

    data = np.asarray(data)

    # normalize the data
    temp = data-min(data)
    data = temp / max(temp)

    # finally, run that data through the mpl.pyplot colormap function
    # and receive our rgb values
    colors = cmap(data)

    return colors

def extract_points(data):
    """ Helper method for plot()
        Extract points from vertices

        :param data: the decomposition that we are rendering
    """

    points = []
    for vertex in data.vertices:
        point = [None]*3

        for j in range(3):
            point[j] = vertex['point'][j].real
        points.append(point)

    return points
