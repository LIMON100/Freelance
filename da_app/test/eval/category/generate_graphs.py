import operator
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
import os
import sys

def adjust_axes(r, t, fig, axes):
        """ ... (rest of the adjust_axes function) ... """
        # get text width for re-scaling
        bb = t.get_window_extent(renderer=r)
        text_width_inches = bb.width / fig.dpi
        # get axis width in inches
        current_fig_width = fig.get_figwidth()
        new_fig_width = current_fig_width + text_width_inches
        propotion = new_fig_width / current_fig_width
        # get axis limit
        x_lim = axes.get_xlim()
        axes.set_xlim([x_lim[0], x_lim[1]*propotion])

def draw_plot_func(dictionary, n_classes, window_title, plot_title, x_label, output_path, to_show, plot_color, true_p_bar):
    """ ... (rest of the draw_plot_func function) ... """
    # sort the dictionary by decreasing value, into a list of tuples
    sorted_dic_by_value = sorted(dictionary.items(), key=operator.itemgetter(1))

    # unpacking the list of tuples into two lists
    sorted_keys, sorted_values = zip(*sorted_dic_by_value)
    #
    if true_p_bar != "":
        """
        Special case to draw in:
            - green -> TP: True Positives (object detected and matches ground-truth)
            - red -> FP: False Positives (object detected but does not match ground-truth)
            - pink -> FN: False Negatives (object not detected but present in the ground-truth)
        """
        fp_sorted = []
        tp_sorted = []
        fn_sorted = [30, 33]

        for key in sorted_keys:
            fp_sorted.append(dictionary[key] - true_p_bar[key])
            tp_sorted.append(true_p_bar[key])
        fp_sorted = [2,3]

        plt.barh(range(n_classes), tp_sorted, color='forestgreen', label='True Positive')
        plt.barh(range(n_classes), fn_sorted, color='orange', label='False Negative')

        # add legend
        plt.legend(loc='lower right')
        """
        Write number on side of bar
        """
        fig = plt.gcf() # gcf - get current figure
        axes = plt.gca()
        r = fig.canvas.get_renderer()
        for i, val in enumerate(sorted_values):
            fp_val = fp_sorted[i]
            tp_val = tp_sorted[i]
            fn_val = fn_sorted[i]

            fp_str_val = " " + str(fp_val)
            tp_str_val = fp_str_val + " " + str(tp_val)
            fn_str_val = " " + str(fn_val)
            # trick to paint multicolor with offset:
            # first paint everything and then repaint the first number
            t = plt.text(val, i, tp_str_val, color='forestgreen', va='center', fontweight='bold')
            plt.text(val, i, fp_str_val, color='crimson', va='center', fontweight='bold')

            if i == (len(sorted_values)-1): # largest bar
                adjust_axes(r, t, fig, axes)
    else:
        plt.barh(range(n_classes), sorted_values, color=plot_color)
        """
        Write number on side of bar
        """
        fig = plt.gcf() 
        axes = plt.gca()
        r = fig.canvas.get_renderer()
        for i, val in enumerate(sorted_values):
            str_val = " " + str(val) 
            if val < 1.0:
                str_val = " {0:.2f}".format(val)
            t = plt.text(val, i, str_val, color=plot_color, va='center', fontweight='bold')
            # re-set axes to show number inside the figure
            if i == (len(sorted_values)-1):
                adjust_axes(r, t, fig, axes)

    # set window title
    fig.canvas.manager.set_window_title(window_title)
    # write classes in y axis
    tick_font_size = 12
    plt.yticks(range(n_classes), sorted_keys, fontsize=tick_font_size)
    """
    Re-scale height accordingly
    """
    init_height = fig.get_figheight()
    # comput the matrix height in points and inches
    dpi = fig.dpi
    height_pt = n_classes * (tick_font_size * 1.4) # 1.4 (some spacing)
    height_in = height_pt / dpi
    # compute the required figure height
    top_margin = 0.15 # in percentage of the figure height
    bottom_margin = 0.05 # in percentage of the figure height
    figure_height = height_in / (1 - top_margin - bottom_margin)
    # set new height
    if figure_height > init_height:
        fig.set_figheight(figure_height)

    # set plot title
    plt.title(plot_title, fontsize=14)
    plt.xlabel(x_label, fontsize='large')
    fig.tight_layout()
    fig.savefig(output_path)
    if to_show:
        plt.show()
    plt.close()


def make_metric_graph_for_three_matric(results, category_names, output_path):
    """ ... (rest of the make_metric_graph_for_three_matric function) ... """
    """
    Parameters
    ----------
    results : dict
        A mapping from question labels to a list of answers per category.
        It is assumed all lists contain the same number of entries and that
        it matches the length of *category_names*.
    category_names : list of str
        The category labels.
    """
    labels = list(results.keys())
    data = np.array(list(results.values()))
    #print(data)
    data_cum = data.cumsum(axis=1)
    category_colors = plt.get_cmap('RdYlGn')(
        np.linspace(0.15, 0.95, data.shape[1]))

    #print(category_colors)

    fig, ax = plt.subplots(figsize=(9.2, 5))
    ax.invert_yaxis()
    ax.xaxis.set_visible(False)
    ax.set_xlim(0, np.sum(data, axis=1).max())

    color2 = ["green", 'crimson', 'purple']

    for i, (colname, color) in enumerate(zip(category_names, category_colors)):
        widths = data[:, i]
        starts = data_cum[:, i] - widths

        # Check if there are any values greater than zero for this metric
        if np.any(widths > 0):
            ax.barh(labels, widths, left=starts, height=0.5,
                    label=colname, color=color2[i])
            xcenters = starts + widths / 2

            r, g, b, _ = color
            text_color = 'darkgrey' #if r * g * b < 0.5 else 'darkgrey'
            for y, (x, c) in enumerate(zip(xcenters, widths)):
                if c > 0: # Only add text if value is greater than 0
                    ax.text(x, y, str(int(c)), ha='center', va='center',
                            color="gainsboro")
    ax.legend(ncol=len(category_names), bbox_to_anchor=(0, 1),
            loc='lower left', fontsize='small')

    plt.savefig(output_path)
    plt.close() # Close plot after saving

    return fig, ax

def make_metric_graph_for_two_matric(results, category_names, output_path):
    """ ... (rest of the make_metric_graph_for_two_matric function) ... """
    """
    Parameters
    ----------
    results : dict
        A mapping from question labels to a list of answers per category.
        It is assumed all lists contain the same number of entries and that
        it matches the length of *category_names*.
    category_names : list of str
        The category labels.
    """
    labels = list(results.keys())
    data = np.array(list(results.values()))
    #print(data)
    data_cum = data.cumsum(axis=1)
    category_colors = plt.get_cmap('RdYlGn')(
        np.linspace(0.15, 0.95, data.shape[1]))

    #print(category_colors)

    fig, ax = plt.subplots(figsize=(9.2, 5))
    ax.invert_yaxis()
    ax.xaxis.set_visible(False)
    ax.set_xlim(0, np.sum(data, axis=1).max())

    color2 = ["green", 'purple']

    for i, (colname, color) in enumerate(zip(category_names, category_colors)):
        widths = data[:, i]
        starts = data_cum[:, i] - widths

        # Check if there are any values greater than zero for this metric
        if np.any(widths > 0):
            ax.barh(labels, widths, left=starts, height=0.5,
                    label=colname, color=color2[i])
            xcenters = starts + widths / 2

            r, g, b, _ = color
            text_color = 'darkgrey' #if r * g * b < 0.5 else 'darkgrey'
            for y, (x, c) in enumerate(zip(xcenters, widths)):
                if c > 0: # Only add text if value is greater than 0
                    ax.text(x, y, str(int(c)), ha='center', va='center',
                            color="gainsboro")
    ax.legend(ncol=len(category_names), bbox_to_anchor=(0, 1),
            loc='lower left', fontsize='small')

    plt.savefig(output_path)
    plt.close() # Close plot after saving

    return fig, ax