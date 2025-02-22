""" Calculate accuracy, mean_average_precision, average_precision, true_positive
    false_positive, false_negative

Created on Sat Feb 22 2025

@author: limon
"""


import glob
import json
import os
import shutil
import operator
import sys
import argparse
import math
import datetime
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
import pandas as pd

def read_classes_from_file(class_file_path):
    """
    Read class names from a file.
    Each class name should be on a separate line.
    """
    with open(class_file_path, 'r') as file:
        classes = [line.strip() for line in file.readlines() if line.strip()]
    return classes


def calculate_mean_avg_pres(root_dir, gt_classes):

    MINOVERLAP = 0.4 

    parser = argparse.ArgumentParser()
    parser.add_argument('-na', '--no-animation', help="no animation is shown.", action="store_true")
    parser.add_argument('-np', '--no-plot', help="no plot is shown.", action="store_true")
    parser.add_argument('-q', '--quiet', help="minimalistic console output.", action="store_true")
    # argparse receiving list of classes to be ignored (e.g., python main.py --ignore person book)
    parser.add_argument('-i', '--ignore', nargs='+', type=str, help="ignore a list of classes.")
    # argparse receiving list of classes with specific IoU (e.g., python main.py --set-class-iou person 0.7)
    parser.add_argument('--set-class-iou', nargs='+', type=str, help="set IoU for a specific class.")
    args = parser.parse_args()


    # if there are no classes to ignore then replace None by empty list
    if args.ignore is None:
        args.ignore = []

    specific_iou_flagged = False
    if args.set_class_iou is not None:
        specific_iou_flagged = True

    # make sure that the cwd() is the location of the python script (so that every path makes sense)
    os.chdir(os.path.dirname(os.path.abspath(__file__)))


    GT_PATH2 = os.path.join(os.getcwd(), root_dir, 'actual')
    GT_PATH = os.path.join(os.getcwd(), root_dir, 'ground-truth')
    DR_PATH = os.path.join(os.getcwd(), root_dir, 'detection-results')
    # if there are no images then no animation can be shown
    IMG_PATH = os.path.join(os.getcwd(), root_dir, 'images')

    n_classes = len(gt_classes)
    print(f"Number of classes: {n_classes}")
    print(f"Classes: {gt_classes}")


    if os.path.exists(IMG_PATH): 
        for dirpath, dirnames, files in os.walk(IMG_PATH):
            if not files:
                # no image files found
                args.no_animation = True
    else:
        args.no_animation = True

    # try to import OpenCV if the user didn't choose the option --no-animation
    show_animation = False
    if not args.no_animation:
        try:
            import cv2
            show_animation = True
        except ImportError:
            print("\"opencv-python\" not found, please install to visualize the results.")
            args.no_animation = True

    # try to import Matplotlib if the user didn't choose the option --no-plot
    draw_plot = False
    if not args.no_plot:
        try:
            import matplotlib.pyplot as plt
            draw_plot = True
        except ImportError:
            print("\"matplotlib\" not found, please install it to get the resulting plots.")
            args.no_plot = True


    def log_average_miss_rate(prec, rec, num_images):
        """
            log-average miss rate:
                Calculated by averaging miss rates at 9 evenly spaced FPPI points
                between 10e-2 and 10e0, in log-space.

            output:
                    lamr | log-average miss rate
                    mr | miss rate
                    fppi | false positives per image

            references:
                [1] Dollar, Piotr, et al. "Pedestrian Detection: An Evaluation of the
                State of the Art." Pattern Analysis and Machine Intelligence, IEEE
                Transactions on 34.4 (2012): 743 - 761.
        """

        # if there were no detections of that class
        if prec.size == 0:
            lamr = 0
            mr = 1
            fppi = 0
            return lamr, mr, fppi

        fppi = (1 - prec)
        mr = (1 - rec)

        fppi_tmp = np.insert(fppi, 0, -1.0)
        mr_tmp = np.insert(mr, 0, 1.0)

        # Use 9 evenly spaced reference points in log-space
        ref = np.logspace(-2.0, 0.0, num = 9)
        for i, ref_i in enumerate(ref):
            # np.where() will always find at least 1 index, since min(ref) = 0.01 and min(fppi_tmp) = -1.0
            j = np.where(fppi_tmp <= ref_i)[-1][-1]
            ref[i] = mr_tmp[j]

        # log(0) is undefined, so we use the np.maximum(1e-10, ref)
        lamr = math.exp(np.mean(np.log(np.maximum(1e-10, ref))))

        return lamr, mr, fppi


    """
    throw error and exit
    """
    def error(msg):
        print(msg)
        sys.exit(0)

    """
    check if the number is a float between 0.0 and 1.0
    """
    def is_float_between_0_and_1(value):
        try:
            val = float(value)
            if val > 0.0 and val < 1.0:
                return True
            else:
                return False
        except ValueError:
            return False

    """
    Calculate the AP given the recall and precision array
        1st) We compute a version of the measured precision/recall curve with
            precision monotonically decreasing
        2nd) We compute the AP as the area under this curve by numerical integration.
    """
    def voc_ap(rec, prec):
        """
        --- Official matlab code VOC2012---
        mrec=[0 ; rec ; 1];
        mpre=[0 ; prec ; 0];
        for i=numel(mpre)-1:-1:1
                mpre(i)=max(mpre(i),mpre(i+1));
        end
        i=find(mrec(2:end)~=mrec(1:end-1))+1;
        ap=sum((mrec(i)-mrec(i-1)).*mpre(i));
        """
        rec.insert(0, 0.0) # insert 0.0 at begining of list
        rec.append(1.0) # insert 1.0 at end of list
        mrec = rec[:]
        prec.insert(0, 0.0) # insert 0.0 at begining of list
        prec.append(0.0) # insert 0.0 at end of list
        mpre = prec[:]
        """
        This part makes the precision monotonically decreasing
            (goes from the end to the beginning)
            matlab: for i=numel(mpre)-1:-1:1
                        mpre(i)=max(mpre(i),mpre(i+1));
        """
        # matlab indexes start in 1 but python in 0, so I have to do:
        #     range(start=(len(mpre) - 2), end=0, step=-1)
        # also the python function range excludes the end, resulting in:
        #     range(start=(len(mpre) - 2), end=-1, step=-1)
        for i in range(len(mpre)-2, -1, -1):
            mpre[i] = max(mpre[i], mpre[i+1])
        """
        This part creates a list of indexes where the recall changes
            matlab: i=find(mrec(2:end)~=mrec(1:end-1))+1;
        """
        i_list = []
        for i in range(1, len(mrec)):
            if mrec[i] != mrec[i-1]:
                i_list.append(i) # if it was matlab would be i + 1
        """
        The Average Precision (AP) is the area under the curve
            (numerical integration)
            matlab: ap=sum((mrec(i)-mrec(i-1)).*mpre(i));
        """
        ap = 0.0
        for i in i_list:
            ap += ((mrec[i]-mrec[i-1])*mpre[i])
        return ap, mrec, mpre


    """
    Convert the lines of a file to a list
    """
    def file_lines_to_list(path):
        # open txt file lines to a list
        with open(path) as f:
            content = f.readlines()
        # remove whitespace characters like `\n` at the end of each line
        content = [x.strip() for x in content]
        return content


    """
    Draws text in image
    """
    def draw_text_in_image(img, text, pos, color, line_width):
        font = cv2.FONT_HERSHEY_PLAIN
        fontScale = 1
        lineType = 1
        bottomLeftCornerOfText = pos
        cv2.putText(img, text,
                bottomLeftCornerOfText,
                font,
                fontScale,
                color,
                lineType)
        text_width, _ = cv2.getTextSize(text, font, fontScale, lineType)[0]
        return img, (line_width + text_width)

    """
    Plot - adjust axes
    """
    def adjust_axes(r, t, fig, axes):
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

    """
    Draw plot using Matplotlib
    """
    def draw_plot_func(dictionary, n_classes, window_title, plot_title, x_label, output_path, to_show, plot_color, true_p_bar):
        # sort the dictionary by decreasing value, into a list of tuples
        sorted_dic_by_value = sorted(dictionary.items(), key=operator.itemgetter(1))
        
        # unpacking the list of tuples into two lists
        sorted_keys, sorted_values = zip(*sorted_dic_by_value)
        #print(sorted_values)
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
            fig = plt.gcf() # gcf - get current figure
            axes = plt.gca()
            r = fig.canvas.get_renderer()
            for i, val in enumerate(sorted_values):
                str_val = " " + str(val) # add a space before
                if val < 1.0:
                    str_val = " {0:.2f}".format(val)
                t = plt.text(val, i, str_val, color=plot_color, va='center', fontweight='bold')
                # re-set axes to show number inside the figure
                if i == (len(sorted_values)-1): # largest bar
                    adjust_axes(r, t, fig, axes)
        # set window title
        # fig.canvas.set_window_title(window_title)
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
        # set axis titles
        # plt.xlabel('classes')
        plt.xlabel(x_label, fontsize='large')
        # adjust size of window
        fig.tight_layout()
        # save the plot
        fig.savefig(output_path)
        # show image
        if to_show:
            plt.show()
        # close the plot
        plt.close()

    """
    Create a ".temp_files/" and "output/" directory
    """
    TEMP_FILES_PATH = ".temp_files"
    if not os.path.exists(TEMP_FILES_PATH): # if it doesn't exist already
        os.makedirs(TEMP_FILES_PATH)
    output_files_path = "output"
    if os.path.exists(output_files_path): # if it exist already
        # reset the output directory
        shutil.rmtree(output_files_path)

    os.makedirs(output_files_path)
    if draw_plot:
        os.makedirs(os.path.join(output_files_path, "classes"))
    if show_animation:
        os.makedirs(os.path.join(output_files_path, "images", "detections_one_by_one"))

    """
    ground-truth
        Load each of the ground-truth files into a temporary ".json" file.
        Create a list of all the class names present in the ground-truth (gt_classes).
    """
    # get a list with the ground-truth files

    ground_truth_files_list = glob.glob(GT_PATH2 + '/*.txt')
    if len(ground_truth_files_list) == 0:
        error("Error: No ground-truth files found!")
    ground_truth_files_list.sort()

    # dictionary with counter per class
    gt_counter_per_class = {}
    counter_images_per_class = {}

    gt_files = []
    for txt_file in ground_truth_files_list:
        #print(txt_file)
        file_id = txt_file.split(".txt", 1)[0]
        file_id = os.path.basename(os.path.normpath(file_id))

        # check if there is a correspondent detection-results file


        lines_list = file_lines_to_list(txt_file)
        # create ground-truth dictionary
        bounding_boxes = []
        is_difficult = False
        already_seen_classes = []
        for line in lines_list:
            try:
                if "difficult" in line:
                        class_name, left, top, right, bottom, _difficult = line.split()
                        is_difficult = True
                else:
                        class_name, left, top, right, bottom = line.split()
            except ValueError:
                error_msg = "Error: File " + txt_file + " in the wrong format.\n"
                error_msg += " Expected: <class_name> <left> <top> <right> <bottom> ['difficult']\n"
                error_msg += " Received: " + line
                error_msg += "\n\nIf you have a <class_name> with spaces between words you should remove them\n"
                error_msg += "by running the script \"remove_space.py\" or \"rename_class.py\" in the \"extra/\" folder."
                error(error_msg)
            # check if class is in the ignore list, if yes skip
            if class_name in args.ignore:
                continue
            bbox = left + " " + top + " " + right + " " +bottom
            if is_difficult:
                bounding_boxes.append({"class_name":class_name, "bbox":bbox, "used":False, "difficult":True})
                is_difficult = False
            else:
                bounding_boxes.append({"class_name":class_name, "bbox":bbox, "used":False})
                # count that object
                if class_name in gt_counter_per_class:
                    gt_counter_per_class[class_name] += 1
                else:
                    # if class didn't exist yet
                    gt_counter_per_class[class_name] = 1

                if class_name not in already_seen_classes:
                    if class_name in counter_images_per_class:
                        counter_images_per_class[class_name] += 1
                    else:
                        # if class didn't exist yet
                        counter_images_per_class[class_name] = 1
                    already_seen_classes.append(class_name)


        # dump bounding_boxes into a ".json" file
        new_temp_file = TEMP_FILES_PATH + "/" + file_id + "_ground_truth.json"
        gt_files.append(new_temp_file)
        with open(new_temp_file, 'w') as outfile:
            json.dump(bounding_boxes, outfile)

    gt_classes = list(gt_counter_per_class.keys())
    # let's sort the classes alphabetically
    gt_classes = sorted(gt_classes)

    n_classes = len(gt_classes)

    """
    Check format of the flag --set-class-iou (if used)
        e.g. check if class exists
    """
    if specific_iou_flagged:
        n_args = len(args.set_class_iou)
        error_msg = \
            '\n --set-class-iou [class_1] [IoU_1] [class_2] [IoU_2] [...]'
        if n_args % 2 != 0:
            error('Error, missing arguments. Flag usage:' + error_msg)

        specific_iou_classes = args.set_class_iou[::2] # even
        # iou_list = ['IoU_1', 'IoU_2']
        iou_list = args.set_class_iou[1::2] # odd
        if len(specific_iou_classes) != len(iou_list):
            error('Error, missing arguments. Flag usage:' + error_msg)
        for tmp_class in specific_iou_classes:
            if tmp_class not in gt_classes:
                        error('Error, unknown class \"' + tmp_class + '\". Flag usage:' + error_msg)
        for num in iou_list:
            if not is_float_between_0_and_1(num):
                error('Error, IoU must be between 0.0 and 1.0. Flag usage:' + error_msg)

    """
    detection-results
        Load each of the detection-results files into a temporary ".json" file.
    """
    # get a list with the detection-results files
    dr_files_list = glob.glob(DR_PATH + '/*.txt')
    dr_files_list.sort()

    for class_index, class_name in enumerate(gt_classes):
        bounding_boxes = []
        for txt_file in dr_files_list:
            #print(txt_file)
            # the first time it checks if all the corresponding ground-truth files exist
            file_id = txt_file.split(".txt",1)[0]
            file_id = os.path.basename(os.path.normpath(file_id))
            temp_path = os.path.join(GT_PATH, (file_id + ".txt"))
            if class_index == 0:
                if not os.path.exists(temp_path):
                    error_msg = "Error. File not found: {}\n".format(temp_path)
                    error_msg += "(You can avoid this error message by running extra/intersect-gt-and-dr.py)"
                    error(error_msg)
            lines = file_lines_to_list(txt_file)
            for line in lines:
                try:
                    tmp_class_name, confidence, left, top, right, bottom = line.split()
                except ValueError:
                    error_msg = "Error: File " + txt_file + " in the wrong format.\n"
                    error_msg += " Expected: <class_name> <confidence> <left> <top> <right> <bottom>\n"
                    error_msg += " Received: " + line
                    error(error_msg)
                if tmp_class_name == class_name:
                    bbox = left + " " + top + " " + right + " " +bottom
                    bounding_boxes.append({"confidence":confidence, "file_id":file_id, "bbox":bbox})
    
        # sort detection-results by decreasing confidence
        bounding_boxes.sort(key=lambda x:float(x['confidence']), reverse=True)
        with open(TEMP_FILES_PATH + "/" + class_name + "_dr.json", 'w') as outfile:
            json.dump(bounding_boxes, outfile)

    """
    Calculate the AP for each class
    """
    sum_AP = 0.0
    ap_dictionary = {}
    lamr_dictionary = {}
    # open file to store the output

    f_html = open("validation_report_new_helmet_model.html", "w")

    message2='''
    <html>
    <body>
        <h1><p style="text-align:center;">Validation Report</p></h1>
    </body>
    </html>
    '''
    f_html.write(message2)


    f_html.write("<pre><h1>" + "Date" + "</h1></pre>\n")
    f_html.write("<pre><h3>" + str(datetime.datetime.now()) + "</h3></pre> <br>\n")
    f_html.write("<pre><h1>" + "Author" + "</h1></pre>\n")
    f_html.write("<pre><h3>" + str("Limon") + "</h3></pre> <br>\n")
    f_html.write("<pre><h1>" + "model_version" + "</h1></pre>\n")
    f_html.write("<pre><h3>" + str("m_0.0.1") + "</h3></pre> <br>\n")


    """ 
    Threshold value table 
    """

    message='''
    <html>
    <head>
        <style>
        table {
            font-family: arial, sans-serif;
            border-collapse: collapse;
            }

            td, th {
            border: 1px solid #dddddd;
            text-align: left;
            padding: 8px;
            }

            tr:nth-child(even) {

            }
        </style>

    </head>
    <body>
        <h1>Threshold value</h1>
        <table>
            <tr>
                <th>Threshold name</th>
                <th>value</th>
            </tr>
            <tr>
                <td>Confidence level</td>
                <td>0.5</td>
            </tr>
            <tr>
                <td>Non-max suppresion(nms)</td>
                <td>0.4</td>
            </tr>
            <tr>
                <td>Intersection over union(IoU)</td>
                <td>0.3</td>
            </tr>
    </table>
    </body>
    <br>\n
    </html>'''

    f_html.write(message)


    message_box_color='''
    <html>
    <head>
        <style>
        table {
            font-family: arial, sans-serif;
            border-collapse: collapse;
            }

            td, th {
            border: 1px solid #dddddd;
            text-align: left;
            padding: 8px;
            }

            tr:nth-child(even) {

            }
        </style>

    </head>
    <body>
        <h1>Box color defined</h1>
        <table>
            <tr>
                <th>Box color</th>
                <th>Indicate</th>
            </tr>
            <tr>
                <td><h5><p style="color:blue">Blue</p></h5></td>
                <td><h5>Ground-truth/Actual-annotation</h5></td>
            </tr>
            <tr>
                <td><h5><p style="color:green">Green</p></h5></td>
                <td><h5>True Positive</h5></td>
            </tr>
            <tr>
                <td><h5><p style="color:red">Red</p></h45</td>
                <td><h5>False Positive</h5></td>
            </tr>
            <tr>
                <td><h5><p style="color:purple">Purple</p></h5></td>
                <td><h5>False Negative</h5></td>
            </tr>
    </table>
    </body>
    <br>\n
    </html>'''

    f_html.write(message_box_color)



    """
    Matrics definition
    """
    message_metric_definition='''
    <html>
    <head>
        <style>
        table {
            font-family: arial, sans-serif;
            border-collapse: collapse;
            }

            td, th {
            border: 1px solid #dddddd;
            text-align: left;
            padding: 8px;
            }

            tr:nth-child(even) {

            }
        </style>

    </head>
    <body>
        <h1>Metrics Definition</h1>
        <table>
            <tr>
                <th>Metric name</th>
                <th>Definition</th>
            </tr>
            <tr>
                <td><h5>True positice</p></h5></td>
                <td><h5>A true positive result is when model correctly labels or categorizes an image. For example, categorizing an image of a With Helmet as a With Helmet</h5></td>
            </tr>
            <tr>
                <td><h5>False Positive</h5></td>
                <td><h5>A false positive result is when model labels or categorizes an image when it should not have. For example, categorizing an image of a With Helmet as a Without Helmet</h5></td>
            </tr>
            <tr>
                <td><h5>False Negative</h5></td>
                <td><h5>A false negative result is when model does not label or categorize an image, but should have. For example, not categorizing an image of a With Helmet as a With Helmet</h5></td>
            </tr>
    </table>
    </body>
    <br>\n
    </html>'''

    f_html.write(message_metric_definition)


    f_html.write("<pre><h1>" + "Images with class name, confidence level " + "</h1></pre> <br>\n")


    message3='''
    <html>
    <head>
        
        <meta charset="UTF-8">
        <meta http-equiv="X-UA-Compatible" content="IE=edge">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">

        <meta http-equiv="X-UA-Compatible" content="IE=edge">
        <script src="filter.js"></script>
        
    </head>

    <body>
        <table id="emp-table">
            <thead>
                <th col-index = 1>Img No. &nbsp </th>

                <th col-index = 2>Actual Class
                    <select class="table-filter" onchange="filter_rows()">
                        <option value="all"></option>
                    </select> &nbsp </th>

                <th col-index = 3>Predicted Class
                    <select class="table-filter" onchange="filter_rows()">
                        <option value="all"></option>
                    </select> &nbsp </th>

                <th col-index = 4>Confidence level 
                    <select class="table-filter" onchange="filter_rows()">
                        <option value="all"></option>
                    </select> &nbsp </th>

                <th col-index = 5>Actual Bounding Box (x,y),(w,h) &nbsp &nbsp </th>
                <th col-index = 6>predicted Bounding Box (x,y), (w,h) &nbsp &nbsp </th>
            
                <th col-index = 7>Result 
                    <select class="table-filter" onchange="filter_rows()">
                        <option value="all"></option>
                    </select>&nbsp &nbsp </th>

                <th col-index = 8>Image &nbsp &nbsp </th>
        
            </thead>
            <tbody>

    '''

    f_html.write(message3)
    cnt=0

    file_double_name = []
    file_double_name.append(0)
    dict_info = {}
    blue_flag=0


    # s 119 13 28
    with open(output_files_path + "/output.txt", 'w') as output_file:
        output_file.write("# AP and precision/recall per class\n")
        count_true_positives = {}

        count_new_true_positives = {}
        for class_index, class_name in enumerate(gt_classes):
            count_true_positives[class_name] = 0
            count_new_true_positives[class_name] = 0
            """
            Load detection-results of that class
            """
            dr_file = TEMP_FILES_PATH + "/" + class_name + "_dr.json"
            dr_data = json.load(open(dr_file))
            #print(dr_data)

            """
            Assign detection-results to ground-truth objects
            """
            nd = len(dr_data)
            tp = [0] * nd # creates an array of zeros of size nd
            fp = [0] * nd
            for idx, detection in enumerate(dr_data):
                file_id = detection["file_id"]
                if show_animation:
                    # find ground truth image
                    ground_truth_img = glob.glob1(IMG_PATH, file_id + ".*")
                    #tifCounter = len(glob.glob1(myPath,"*.tif"))
                    if len(ground_truth_img) == 0:
                        error("Error. Image not found with id: " + file_id)
                    elif len(ground_truth_img) > 1:
                        error("Error. Multiple image with id: " + file_id)
                    else: # found image
                        #print(IMG_PATH + "/" + ground_truth_img[0])
                        # Load image
                        img = cv2.imread(IMG_PATH + "/" + ground_truth_img[0])
                        # load image with draws of multiple detections
                        img_cumulative_path = output_files_path + "/images/" + ground_truth_img[0]
                        if os.path.isfile(img_cumulative_path):
                            img_cumulative = cv2.imread(img_cumulative_path)
                        else:
                            img_cumulative = img.copy()
                        # Add bottom border to image
                        bottom_border = 60
                        BLACK = [0, 0, 0]
                        img = cv2.copyMakeBorder(img, 0, bottom_border, 0, 0, cv2.BORDER_CONSTANT, value=BLACK)
                # assign detection-results to ground truth object if any
                # open ground-truth with that file_id
                gt_file = TEMP_FILES_PATH + "/" + file_id + "_ground_truth.json"
                ground_truth_data = json.load(open(gt_file))
                ovmax = -1
                gt_match = -1
                # load detected object bounding-box
                bb = [ float(x) for x in detection["bbox"].split() ]
                for obj in ground_truth_data:
                    # look for a class_name match
                    if obj["class_name"] == class_name:
                        bbgt = [ float(x) for x in obj["bbox"].split() ]
                        bi = [max(bb[0],bbgt[0]), max(bb[1],bbgt[1]), min(bb[2],bbgt[2]), min(bb[3],bbgt[3])]
                        iw = bi[2] - bi[0] + 1
                        ih = bi[3] - bi[1] + 1
                        if iw > 0 and ih > 0:
                            # compute overlap (IoU) = area of intersection / area of union
                            ua = (bb[2] - bb[0] + 1) * (bb[3] - bb[1] + 1) + (bbgt[2] - bbgt[0]
                                            + 1) * (bbgt[3] - bbgt[1] + 1) - iw * ih
                            ov = iw * ih / ua
                            if ov > ovmax:
                                ovmax = ov
                                gt_match = obj

                #print(ground_truth_data[0]["class_name"])
                # assign detection as true positive/don't care/false positive
                if show_animation:
                    status = "NO MATCH FOUND!" # status is only used in the animation
                # set minimum overlap
                min_overlap = MINOVERLAP
                if specific_iou_flagged:
                    if class_name in specific_iou_classes:
                        index = specific_iou_classes.index(class_name)
                        min_overlap = float(iou_list[index])
                if ovmax >= min_overlap:
                    if "difficult" not in gt_match:
                            if not bool(gt_match["used"]):
                                # true positive
                                tp[idx] = 1
                                gt_match["used"] = True
                                count_true_positives[class_name] += 1
                                
                                # update the ".json" file
                                with open(gt_file, 'w') as f:
                                        f.write(json.dumps(ground_truth_data))
                                if show_animation:
                                    status = "MATCH!"
                            else:
                                # false positive (multiple detection)
                                fp[idx] = 1
                                if show_animation:
                                    status = "REPEATED MATCH!"
                else:
                    # false positive
                    fp[idx] = 1
                    if ovmax > 0:
                        status = "INSUFFICIENT OVERLAP"

                """
                Draw image to show animation
                """
                
                if show_animation:
                    height, widht = img.shape[:2]
                    # colors (OpenCV works with BGR)
                    white = (255,255,255)
                    light_blue = (255,0,0) #255,200,100)
                    #green = (0,255,0)
                    green = (0,128,0)
                    light_red = (30,30,255)
                    # 1st line
                    margin = 10
                    v_pos = int(height - margin - (bottom_border / 2.0))
                    text = "Image: " + ground_truth_img[0] + " "
                    img, line_width = draw_text_in_image(img, text, (margin, v_pos), white, 0)
                    text = "Class [" + str(class_index) + "/" + str(n_classes) + "]: " + class_name + " "
                    img, line_width = draw_text_in_image(img, text, (margin + line_width, v_pos), light_blue, line_width)
        
                    
                    if ovmax != -1:
                        color = light_red
                        if status == "INSUFFICIENT OVERLAP":
                            text = "IoU: {0:.2f}% ".format(ovmax*100) + "< {0:.2f}% ".format(min_overlap*100)
                        else:
                            text = "IoU: {0:.2f}% ".format(ovmax*100) + ">= {0:.2f}% ".format(min_overlap*100)
                            color = green
                            #print(text)
                        img, _ = draw_text_in_image(img, text, (margin + line_width, v_pos), color, line_width)
                    # 2nd line
                    v_pos += int(bottom_border / 2.0)
                    rank_pos = str(idx+1) # rank position (idx starts at 0)
                    text = "Detection #rank: " + rank_pos + " confidence: {0:.2f}% ".format(float(detection["confidence"])*100)
                    img, line_width = draw_text_in_image(img, text, (margin, v_pos), white, 0)
                    #print(text)
                    #check here now

                    color = light_red
                    if status == "MATCH!":
                        color = green
                    text = "Result: " + status + " "
                    img, line_width = draw_text_in_image(img, text, (margin + line_width, v_pos), color, line_width)

                    for i in range(0, len(file_double_name)):
                        if file_double_name[i] == ground_truth_img[0]:
                            #print(ground_truth_img[0])
                            cnt += 1
                            dict_info = {ground_truth_img[0]: cnt} 
                            break

                    file_double_name.append(ground_truth_img[0])


                    font = cv2.FONT_HERSHEY_SIMPLEX
                    if  ovmax <0:
                        tp[idx] = 1
                        count_new_true_positives[class_name] += 1
                    
         
                    
                    if ovmax >= 0: # if there is intersections between the bounding-boxes
                        
                        f_html.write("<tr><td>"'<h4>' + str(ground_truth_img[0].split(".")[-2]) + '</h4>'"</td>")
                        if (ground_truth_data[0]["class_name"] == class_name):
                            f_html.write("<td>"'<h4>'+ str(ground_truth_data[0]["class_name"]) + '</h4>'"</td>")
                            f_html.write("<td>"'<h4>'+ str(class_name) +'</h4>'"</td>")
                        if (ground_truth_data[0]["class_name"] != class_name):
                            #f_html.write("<td>"'<h4>'+ str(ground_truth_data[0]["class_name"]) + '</h4>'"</td>")
                            f_html.write("<td>"'<h4>'+ str(class_name) +'</h4>'"</td>")
                            f_html.write("<td>"'<h4>'+ str(class_name) +'</h4>'"</td>")
                        f_html.write("<td>"'<h4>'+ str(detection["confidence"]) +'</h4>'"</td>")

                    
                        bbgt = [ int(round(float(x))) for x in gt_match["bbox"].split() ]
                        cv2.rectangle(img,(bbgt[0],bbgt[1]),(bbgt[2],bbgt[3]),light_blue,2)
                        cv2.rectangle(img_cumulative,(bbgt[0],bbgt[1]),(bbgt[2],bbgt[3]),light_blue,2)
        

                        f_html.write("<td>" + '<h4><p style="color:blue;">'+"x:" + str(bbgt[0]) + " " + "y:" + str(bbgt[1])+'</p></h4>')
                        f_html.write('<h4><p style="color:blue;">'+"w:" + str(bbgt[2]) + " " + "h:" + str(bbgt[3])+'</p></h4>' +"</td>")
                        
                        blue_flag=1
                        cv2.putText(img_cumulative, class_name, (bbgt[0],bbgt[1] - 5), font, 0.6, light_blue, 1, cv2.LINE_AA)


                        bb = [int(i) for i in bb]


                        if ovmax >= 0.1:

                            f_html.write("<td>" + '<h4><p style="color:green;">'+"x:" + str(bb[0]) + " " + "y:" + str(bb[1])+'</p></h4>')
                            f_html.write('<h4><p style="color:green;">'+"w:" + str(bb[2]) + " " + "h:" + str(bb[3])+'</p></h4>' +"</td>")

                            """set empty to red"""

                            
                            cv2.putText(img, "True positive", (bb[0], bb[1] - 5), font, 0.6, green, 1, cv2.LINE_AA)

                            f_html.write("<td>"'<h4>True Positive</h4>'"</td>")
                            cv2.rectangle(img,(bb[0],bb[1]),(bb[2],bb[3]),green,2)

                            tp[idx] = 1
                            count_new_true_positives[class_name] += 1

                        else:

                            f_html.write("<td>" + '<h4><p style="color:red;">'+"x:" + str(bb[0]) + " " + "y:" + str(bb[1])+'</p></h4>')
                            f_html.write('<h4><p style="color:red;">'+"w:" + str(bb[2]) + " " + "h:" + str(bb[3])+'</p></h4>' +"</td>")

                            cv2.putText(img, "False positive", (bb[0],bb[1] - 5), font, 0.6, light_red, 1, cv2.LINE_AA)


                            f_html.write("<td>"'<h4>False Positive</h4>'"</td>")
                            cv2.rectangle(img,(bb[0],bb[1]),(bb[2],bb[3]),light_red,2)
                            fp[idx] = 1
                
            
                        cv2.rectangle(img_cumulative,(bb[0],bb[1]),(bb[2],bb[3]),color,2)
                        cv2.putText(img_cumulative, class_name, (bb[0],bb[1] - 5), font, 0.6, color, 1, cv2.LINE_AA)
                        
                        # show image
                        cv2.imshow("Animation", img)
                        cv2.waitKey(20) # show for 20 ms
                        # save image to output
                        output_img_path = output_files_path + "/images/detections_one_by_one/" + class_name + "_detection" + str(idx) + ".jpg"
                
                        cv2.imwrite(output_img_path, img)
                        
                        # save the image with all the objects drawn to it
                        cv2.imwrite(img_cumulative_path, img_cumulative)
                        
                        f_html.write("<td>" + '<a><img src="'+ str(output_img_path) +'"></a>' + "</td>")
                        f_html.write("</tr>")
                        
            
            # compute precision/recall
            cumsum = 0
            for idx, val in enumerate(fp):
                fp[idx] += cumsum
                cumsum += val
            cumsum = 0
            for idx, val in enumerate(tp):
                tp[idx] += cumsum
                cumsum += val
      
            rec = tp[:]
            for idx, val in enumerate(tp):
                rec[idx] = float(tp[idx]) / gt_counter_per_class[class_name]
            #print(rec)
            prec = tp[:]
            for idx, val in enumerate(tp):
                prec[idx] = float(tp[idx]) / (fp[idx] + tp[idx])
   

            ap, mrec, mprec = voc_ap(rec[:], prec[:])
            sum_AP += ap
            text = "{0:.2f}%".format(ap*100) + " = " + class_name + " AP " #class_name + " AP = {0:.2f}%".format(ap*100)
            """
            Write to output.txt
            """
            rounded_prec = [ '%.2f' % elem for elem in prec ]
            rounded_rec = [ '%.2f' % elem for elem in rec ]
            output_file.write(text + "\n Precision: " + str(rounded_prec) + "\n Recall :" + str(rounded_rec) + "\n\n")
            if not args.quiet:
                print(text)
            ap_dictionary[class_name] = ap

            n_images = counter_images_per_class[class_name]
            lamr, mr, fppi = log_average_miss_rate(np.array(prec), np.array(rec), n_images)
            lamr_dictionary[class_name] = lamr

            """
            Draw plot
            """
            if draw_plot:
                plt.plot(rec, prec, '-o')
                # add a new penultimate point to the list (mrec[-2], 0.0)
                # since the last line segment (and respective area) do not affect the AP value
                area_under_curve_x = mrec[:-1] + [mrec[-2]] + [mrec[-1]]
                area_under_curve_y = mprec[:-1] + [0.0] + [mprec[-1]]
                plt.fill_between(area_under_curve_x, 0, area_under_curve_y, alpha=0.2, edgecolor='r')
                # set window title
                fig = plt.gcf() # gcf - get current figure
                # fig.canvas.set_window_title('AP ' + class_name)
                fig.canvas.manager.set_window_title('AP ' + class_name)
                # set plot title
                plt.title('class: ' + text)
                #plt.suptitle('This is a somewhat long figure title', fontsize=16)
                # set axis titles
                plt.xlabel('Recall')
                plt.ylabel('Precision')
                # optional - set axes
                axes = plt.gca() # gca - get current axes
                axes.set_xlim([0.0,1.0])
                axes.set_ylim([0.0,1.05]) # .05 to give some extra space
                # Alternative option -> wait for button to be pressed
                #while not plt.waitforbuttonpress(): pass # wait for key display
                # Alternative option -> normal display
                #plt.show()
                # save the plot
                fig.savefig(output_files_path + "/classes/" + class_name + ".png")
                plt.cla() # clear axes for next plot

        if show_animation:
            cv2.destroyAllWindows()

        output_file.write("\n# mAP of all classes\n")
        mAP = sum_AP / n_classes
        text = "mAP = {0:.2f}%".format(mAP*100)
        output_file.write(text + "\n")


    """
    Draw false negatives
    """
    count_new_false_negative = {}

    #name_of_classes = "scripts/extra/classes.txt"
    name_of_classes = root_dir + "/" + "classes.txt"
    lines = file_lines_to_list(name_of_classes)

    for line in range(0, len(lines)):
        #print(lines[line])
        count_new_false_negative[lines[line]] = 0


    """
    Create a ".temp_files/" and "output/" directory
    """
    # TEMP_FN_PATH = root_dir + "\\" + "False_Negative"
    TEMP_FN_PATH = root_dir + "/" + "False_Negative"
    if not os.path.exists(TEMP_FN_PATH):
        os.makedirs(TEMP_FN_PATH)


    if show_animation:

        purple = (128,0,128)
        for tmp_file in gt_files:
            ground_truth_data = json.load(open(tmp_file))

            # get name of corresponding image
            start = TEMP_FILES_PATH + '/'
            img_id = tmp_file[tmp_file.find(start)+len(start):tmp_file.rfind('_ground_truth.json')]
            img_cumulative_path = output_files_path + "/images/" + img_id + ".jpg"
            img = cv2.imread(img_cumulative_path)

            if img is None:
                img_path = IMG_PATH + '/' + img_id + ".jpg"
                img = cv2.imread(img_path)
            # draw false negatives
            for obj in ground_truth_data:

                if not obj['used']:

                    TEMP_FN_PATH2 = TEMP_FN_PATH + "/" + str(img_cumulative_path.split("/")[-1].split(".")[-2]) + ".jpg"
                    TEMP_FN_PATH3 = root_dir + "/images/" + str(img_cumulative_path.split("/")[-1].split(".")[-2]) + ".jpg"
                    img_fn = cv2.imread(TEMP_FN_PATH3)

                    cv2.imwrite(TEMP_FN_PATH2, img_fn)

                    """
                        take the xml according to the image name
                    """


                    format = {'xml','txt'}
    
                    # xml_dir = root_dir + "\\" + "backup"
                    # image_dir = root_dir + "\\" + "False_Negative"

                    xml_dir = root_dir + "/" + "backup"
                    image_dir = root_dir + "/" + "False_Negative"
            
                    # Check if xml_dir exists, and create it if not
                    if not os.path.exists(xml_dir):
                        os.makedirs(xml_dir, exist_ok=True)
                        print(f"Created directory: {xml_dir}")


                    for obj1 in os.scandir(xml_dir):
                        if obj1.is_dir():
                            print("Dir:",obj1.name)
                        elif obj1.is_file():
                            if obj1.name.split(".")[-1] in format:
                                
                                if obj1.name.split(".")[-1] == "xml":

                                    xml_name = str(img_cumulative_path.split("/")[-1].split(".")[-2]) + ".xml"

                                    if obj1.name == xml_name:
                                        # xml_file_dir = xml_dir + "\\" + xml_name
                                        # image_file_dir = image_dir + "\\" + xml_name

                                        xml_file_dir = xml_dir + "/" + xml_name
                                        image_file_dir = image_dir + "/" + xml_name
                                        shutil.copy(xml_file_dir, image_file_dir)
                    
                                        break

                                if obj1.name.split(".")[-1] == "txt":
                                
                                    xml_name = str(img_cumulative_path.split("/")[-1].split(".")[-2]) + ".txt"

                                    if obj1.name == xml_name:
                                        xml_file_dir = xml_dir + "/" + xml_name
                                        image_file_dir = image_dir + "/" + xml_name
                                        shutil.copy(xml_file_dir, image_file_dir)
                                        #print("Work done...")
                                        break

                    bbgt = [ int(round(float(x))) for x in obj["bbox"].split() ]
                    cv2.rectangle(img,(bbgt[0],bbgt[1]),(bbgt[2],bbgt[3]),purple,2)
                    
                    
                    f_html.write("<tr><td>"'<h4>' + str(img_cumulative_path.split("/")[-1].split(".")[-2]) + '</h4>'"</td>")


                    file_id_txt = img_cumulative_path.split("/")[-1].split(".")[-2]
                    temp_path = os.path.join(DR_PATH, (file_id_txt + ".txt"))

                    if not os.path.exists(temp_path):

                        class_name_font = str(obj["class_name"])
                        f_html.write("<td>"'<h4>'+ class_name_font +'</h4>'"</td>")
                        f_html.write("<td>"'<h4>N/A</h4>'"</td>")
                        f_html.write("<td>"'<h4>N/A</h4>'"</td>")

                        f_html.write("<td>" + '<h4><p style="color:purple;">'+"x:" + str(bbgt[0]) + " " + "y:" + str(bbgt[1])+'</p></h4>')
                        f_html.write('<h4><p style="color:purple;">'+"w:" + str(bbgt[2]) + " " + "h:" + str(bbgt[3])+'</p></h4>' +"</td>")
                    
                    
                        f_html.write("<td>" + '<h4><p style="color:black;">'+"x:" + str("N/A") + " " + "y:" + str("N/A")+'</p></h4>')
                        f_html.write('<h4><p style="color:black;">'+"w:" + str("N/A") + " " + "h:" + str("N/A")+'</p></h4>' +"</td>")

                    else:

                        class_name_font = str(obj["class_name"])
                        f_html.write("<td>"'<h4>'+ class_name_font +'</h4>'"</td>")

                        lines = file_lines_to_list(temp_path)
                        for line in lines:
                            tmp_class_name, confidence, left, top, right, bottom = line.split()

                        f_html.write("<td>"'<h4>'+ tmp_class_name +'</h4>'"</td>")

                        f_html.write("<td>"'<h4>N/A</h4>'"</td>")

                        f_html.write("<td>" + '<h4><p style="color:purple;">'+"x:" + str(bbgt[0]) + " " + "y:" + str(bbgt[1])+'</p></h4>')
                        f_html.write('<h4><p style="color:purple;">'+"w:" + str(bbgt[2]) + " " + "h:" + str(bbgt[3])+'</p></h4>' +"</td>")
                        
                        f_html.write("<td>" + '<h4><p style="color:black;">'+"x:" + str(left) + " " + "y:" + str(top)+'</p></h4>')
                        f_html.write('<h4><p style="color:black;">'+"w:" + str(right) + " " + "h:" + str(bottom)+'</p></h4>' +"</td>")


                    f_html.write("<td>"'<h4>False Negative</h4>'"</td>")
                    count_new_false_negative[str(obj["class_name"])] += 1

                    cv2.putText(img, "False Negative", (bbgt[0],bbgt[1] - 5), font, 0.6, light_red, 1, cv2.LINE_AA)

                    f_html.write("<td>" + '<a><img src="'+ str(img_cumulative_path) +'"></a>' + "</td>")
                    f_html.write("</tr>")
                    
            cv2.imwrite(img_cumulative_path, img)

    f_html.write("</tbody>")
    f_html.write("</table>")

    f_html.write("<script>"' window.onload = () => {console.log(document.querySelector("#emp-table > tbody > tr:nth-child(1) > td:nth-child(2) ").innerHTML);}; getUniqueValuesFromColumn() '"</script>")

    f_html.write("</body></html>")

    # remove the temp_files directory
    shutil.rmtree(TEMP_FILES_PATH)


    """
    Count total of detection-results
    """
    # iterate through all the files
    det_counter_per_class = {}
    for txt_file in dr_files_list:
        # get lines to list
        lines_list = file_lines_to_list(txt_file)
        for line in lines_list:
            class_name = line.split()[0]
            # check if class is in the ignore list, if yes skip
            if class_name in args.ignore:
                continue
            # count that object
            if class_name in det_counter_per_class:
                det_counter_per_class[class_name] += 1
            else:
                # if class didn't exist yet
                det_counter_per_class[class_name] = 1

    dr_classes = list(det_counter_per_class.keys())

    """
    Plot the total number of occurences of each class in the ground-truth
    """
    if draw_plot:
        window_title = "ground-truth-info"
        plot_title = "ground-truth\n"
        plot_title += "(" + str(len(ground_truth_files_list)) + " files and " + str(n_classes) + " classes)"
        x_label = "Number of objects per class"
        output_path = output_files_path + "/ground-truth-info.png"
        message = "<pre><h1>" + "Detection result" + "</h1></pre> <br>\n"
        f_html.write(message)
        f_html.write('<a><img src="'+ str(output_path) +'"></a>')
        f_html.write("</body></html>")
        to_show = False
        plot_color = 'forestgreen'
        draw_plot_func(
            gt_counter_per_class,
            n_classes,
            window_title,
            plot_title,
            x_label,
            output_path,
            to_show,
            plot_color,
            '',
            )

    """
    Write number of ground-truth objects per class to results.txt
    """
    with open(output_files_path + "/output.txt", 'a') as output_file:
        output_file.write("\n# Number of ground-truth objects per class\n")
    
        for class_name in sorted(gt_counter_per_class):
            output_file.write(class_name + ": " + str(gt_counter_per_class[class_name]) + "\n")

    """
    Finish counting true positives
    """
    for class_name in dr_classes:
        # if class exists in detection-result but not in ground-truth then there are no true positives in that class
        if class_name not in gt_classes:
            count_true_positives[class_name] = 0


    """"
    Check the updated one which doesnot have false positive
    """
    tp2 = []
    fp2 = []

    new_graph = {}

    def append_value(dict_obj, key, value):
        # Check if key exist in dict or not
        if key in dict_obj:
            # Key exist in dict.
            # Check if type of value of key is list or not
            if not isinstance(dict_obj[key], list):
                # If type is not list then make it list
                dict_obj[key] = [dict_obj[key]]
            # Append the value in list
            dict_obj[key].append(value)
        else:
            # As key is not in dict,
            # so, add key-value pair
            dict_obj[key] = value

    with open(output_files_path + "/output.txt", 'a') as output_file:
        output_file.write("\n# Number of detected objects per class\n")
        for class_name in sorted(dr_classes):
            n_det = det_counter_per_class[class_name]
            text = class_name + ": " + str(n_det)
            text += " (tp:" + str(count_new_true_positives[class_name]) + ""
            text += ", fp:" + str(n_det - count_new_true_positives[class_name]) + ")\n"

            tp3 = str(count_new_true_positives[class_name])
            fp3 = str(n_det - count_new_true_positives[class_name])
            fn3 = str(count_new_false_negative[class_name])
            tp2.append(tp3)
            fp2.append(fp3)
            output_file.write(text)
            append_value(new_graph, class_name, int(tp3))
            append_value(new_graph, class_name, int(fp3))

            append_value(new_graph, class_name, int(fn3))


    

    """
    Draw mAP plot (Show AP's of all classes in decreasing order)
    """
    if draw_plot:
        window_title = "mAP"
        plot_title = "mAP = {0:.2f}%".format(mAP*100)
        x_label = "Average Precision"
        output_path = output_files_path + "/mAP.png"
        message = "<pre><h1>" + "MAP(Mean average precision)" + "</h1></pre> <br>\n"
        f_html.write(message)
        f_html.write('<a><img src="'+ str(output_path) +'"></a>')
        f_html.write("</body></html>")
        to_show = False
        plot_color = 'royalblue'
        draw_plot_func(
            ap_dictionary,
            n_classes,
            window_title,
            plot_title,
            x_label,
            output_path,
            to_show,
            plot_color,
            ""
            )



    message = "<pre><h1>" + "Precision Recall Curve" + "</h1></pre> <br>\n"
    f_html.write(message)

    pr_directory = "output/classes/"

    for filename in os.listdir(pr_directory):
        f = os.path.join(pr_directory, filename)
        f_html.write('<a><img src="'+ f  +'"></a>')


    """ 
    Ploting conf-matrix
    """


    results = {}
    category_names = []


    def make_metric_graph_for_three_matric(results, category_names):
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

        return fig, ax

    def make_metric_graph_for_two_matric(results, category_names):
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

        return fig, ax


    """
    Loop through the category_list
    """

    check_false_positive_cnt = 0


    for key,value in new_graph.items():
        if value[1] == 0:
            check_false_positive_cnt += 1

    dict2 = {}
    for key,value in new_graph.items():

        if check_false_positive_cnt == 4 or check_false_positive_cnt == 2:
            append_value(dict2, key, value[0])
            append_value(dict2, key, value[2])
            category_names = ['True Positives', 'False Negatives']
            make_metric_graph_for_two_matric(dict2, category_names)

        elif check_false_positive_cnt != 4:
            append_value(dict2, key, value[0])
            append_value(dict2, key, value[1])
            append_value(dict2, key, value[2])
            category_names = ['True Positives', 'False Positives', 'False Negatives']
            make_metric_graph_for_three_matric(dict2, category_names)



    plt.savefig("output" + "/detection_results.jpg")
    #plt.show()

    message = "<pre><h1>" + "True positve, False positive, False negetive Graph" + "</h1></pre> <br>\n"
    f_html.write(message)
    f_html.write('<a><img src="'+ str("output" + "/detection_results.jpg") +'"></a>')
    f_html.write("</body></html>")


    """Class wise accuracy"""
    message_new = "<pre><h1>" + "Class-wise accuracy" + "</h1></pre> <br>\n"
    f_html.write(message_new)

    for key,value in new_graph.items():
        class_ac = value[0] / (value[0] + value[2])
        message_new = "<pre><h3>" + key + " : " + '%.2f' % (class_ac * 100) + "%" + "</h3></pre> <br>\n"
        f_html.write(message_new)

    f_html.write("</body></html>")



    class_wise_accuracy = {}
    for class_name in new_graph.keys():
        tp = new_graph[class_name][0]
        fp = new_graph[class_name][1]
        fn = new_graph[class_name][2]
        class_wise_accuracy[class_name] = tp / (tp + fp + fn)

    # Calculate overall precision and recall
    total_tp = sum(new_graph[class_name][0] for class_name in new_graph.keys())
    total_fp = sum(new_graph[class_name][1] for class_name in new_graph.keys())
    total_fn = sum(new_graph[class_name][2] for class_name in new_graph.keys())

    overall_precision = total_tp / (total_tp + total_fp) if (total_tp + total_fp) > 0 else 0
    overall_recall = total_tp / (total_tp + total_fn) if (total_tp + total_fn) > 0 else 0

    # Add overall precision and recall to the HTML report
    f_html.write("<pre><h1>" + "Overall Precision and Recall" + "</h1></pre> <br>\n")
    f_html.write(f"<pre><h3>Overall Precision: {overall_precision * 100:.2f}%</h3></pre> <br>\n")
    f_html.write(f"<pre><h3>Overall Recall: {overall_recall * 100:.2f}%</h3></pre> <br>\n")
    f_html.write("</body></html>")

    print("FINISH.........")


if __name__ == "__main__":
    root_dir = "/home/limonubuntu/Work/Limon/other_task/driver_anomaly/test/testing/eval_demo/ob_eval_da/da_part1"
    
    # Path to the classes.txt file
    class_file_path = os.path.join(root_dir, "classes.txt")
    
    # Read classes from the file
    gt_classes = read_classes_from_file(class_file_path)
    
    # Now you can use gt_classes dynamically in your code
    calculate_mean_avg_pres(root_dir, gt_classes)