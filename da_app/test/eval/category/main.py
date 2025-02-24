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
import cv2
from html_report_generator import generate_html_report 
from report_utils import read_classes_from_file, log_average_miss_rate, error, is_float_between_0_and_1, voc_ap, file_lines_to_list, draw_text_in_image
from generate_graphs import adjust_axes, draw_plot_func, make_metric_graph_for_three_matric, make_metric_graph_for_two_matric

def calculate_mean_avg_pres(root_dir, gt_classes):

    MINOVERLAP = 0.4

    parser = argparse.ArgumentParser()
    parser.add_argument('-na', '--no-animation', help="no animation is shown.", action="store_true")
    parser.add_argument('-np', '--no-plot', help="no plot is shown.", action="store_true")
    parser.add_argument('-q', '--quiet', help="minimalistic console output.", action="store_true")
    parser.add_argument('-i', '--ignore', nargs='+', type=str, help="ignore a list of classes.")
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
    IMG_PATH = os.path.join(os.getcwd(), root_dir, 'images')

    n_classes = len(gt_classes)
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
            import cv2 # Import OpenCV here
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
        file_id = txt_file.split(".txt", 1)[0]
        file_id = os.path.basename(os.path.normpath(file_id))

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

    image_results_for_html = [] # List to store image-wise results for HTML report
    fn_image_results_for_html = [] # List for FN image results

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
                    else:
                        # Load image
                        img = cv2.imread(IMG_PATH + "/" + ground_truth_img[0])
                        # load image with draws of multiple detections
                        img_cumulative_path = os.path.join(output_files_path, "images", ground_truth_img[0])
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
                if ovmax >= 0: # if there is intersections between the bounding-boxes
                    bbgt_html = [ int(round(float(x))) for x in gt_match["bbox"].split() ]
                    bb_html = [int(i) for i in bb]
                    image_results_for_html.append({
                        'img_no': ground_truth_img[0].split(".")[-2],
                        'actual_class': ground_truth_data[0]["class_name"] if (ground_truth_data[0]["class_name"] == class_name) else class_name, # Show actual GT class
                        'predicted_class': class_name,
                        'confidence_level': detection["confidence"],
                        'actual_bbox': f"x:{bbgt_html[0]} y:{bbgt_html[1]} w:{bbgt_html[2]} h:{bbgt_html[3]}",
                        'predicted_bbox': f"x:{bb_html[0]} y:{bb_html[1]} w:{bb_html[2]} h:{bb_html[3]}",
                        'result': "True Positive" if ovmax >= 0.1 else "False Positive", # Simplified result for HTML
                        'image_path': os.path.join("output", "images", "detections_one_by_one", class_name + "_detection" + str(idx) + ".jpg") # Path for HTML img src
                    })

                """
                Draw image to show animation
                """
                if show_animation:
                    height, widht = img.shape[:2]
                    white = (255,255,255)
                    light_blue = (255,0,0) 
                    green = (0,128,0)
                    light_red = (30,30,255)

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
                        img, _ = draw_text_in_image(img, text, (margin + line_width, v_pos), color, line_width)
                    # 2nd line
                    v_pos += int(bottom_border / 2.0)
                    rank_pos = str(idx+1) # rank position (idx starts at 0)
                    text = "Detection #rank: " + rank_pos + " confidence: {0:.2f}% ".format(float(detection["confidence"])*100)
                    img, line_width = draw_text_in_image(img, text, (margin, v_pos), white, 0)


                    color = light_red
                    if status == "MATCH!":
                        color = green
                    text = "Result: " + status + " "
                    img, line_width = draw_text_in_image(img, text, (margin + line_width, v_pos), color, line_width)

                    font = cv2.FONT_HERSHEY_SIMPLEX
                    if  ovmax <0:
                        tp[idx] = 1
                        count_new_true_positives[class_name] += 1

                    if ovmax >= 0.1:
                        bb_int = [int(coord) for coord in bb]  # Convert all bb coords to int
                        if len(bb_int) == 4:
                            cv2.putText(img, "True positive", (bb_int[0], bb_int[1] - 5), font, 0.6, green, 1, cv2.LINE_AA)
                            cv2.rectangle(img,(bb_int[0],bb_int[1]),(bb_int[2],bb_int[3]),green,2)
                            tp[idx] = 1
                            count_new_true_positives[class_name] += 1

                    else:
                        bb_int = [int(coord) for coord in bb] # Convert all bb coords to int
                        if len(bb_int) == 4: 
                            cv2.putText(img, "False positive", (bb_int[0], bb_int[1] - 5), font, 0.6, light_red, 1, cv2.LINE_AA)
                            cv2.rectangle(img,(bb_int[0],bb_int[1]),(bb_int[2],bb_int[3]),light_red,2)
                            fp[idx] = 1


                    if isinstance(gt_match, dict): # Check if gt_match is a dictionary
                        bbgt = [ int(round(float(x))) for x in gt_match["bbox"].split() ]
                        bbgt_int = [int(coord) for coord in bbgt] 
                        if len(bbgt_int) == 4: 
                            cv2.rectangle(img,(bbgt_int[0],bbgt_int[1]),(bbgt_int[2],bbgt_int[3]),light_blue,2)
                            cv2.rectangle(img_cumulative,(bbgt_int[0],bbgt_int[1]),(bbgt_int[2],bbgt_int[3]),light_blue,2)
                            cv2.putText(img_cumulative, class_name, (bbgt_int[0],bbgt_int[1] - 5), font, 0.6, light_blue, 1, cv2.LINE_AA)

                    bb = [int(i) for i in bb]
                    cv2.rectangle(img_cumulative,(bb[0],bb[1]),(bb[2],bb[3]),color,2)
                    cv2.putText(img_cumulative, class_name, (bb[0],bb[1] - 5), font, 0.6, color, 1, cv2.LINE_AA)

                    # show image
                    cv2.imshow("Animation", img)
                    cv2.waitKey(20) # show for 20 ms
                    # save image to output
                    output_img_path = os.path.join(output_files_path, "images", "detections_one_by_one", class_name + "_detection" + str(idx) + ".jpg")
                    cv2.imwrite(output_img_path, img)

                    # save the image with all the objects drawn to it
                    cv2.imwrite(img_cumulative_path, img_cumulative)

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
                fig = plt.gcf()
                fig.canvas.manager.set_window_title('AP ' + class_name)
                # set plot title
                plt.title('class: ' + text)
                plt.xlabel('Recall')
                plt.ylabel('Precision')
                # optional - set axes
                axes = plt.gca() # gca - get current axes
                axes.set_xlim([0.0,1.0])
                axes.set_ylim([0.0,1.05]) # .05 to give some extra space

                fig.savefig(os.path.join(output_files_path, "classes", class_name + ".png"))
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
    fn_image_results_for_html = [] # List for FN image results


    name_of_classes = os.path.join(root_dir, "classes.txt")
    lines = file_lines_to_list(name_of_classes)

    for line in range(0, len(lines)):
        count_new_false_negative[lines[line]] = 0

    """
    Create a ".temp_files/" and "output/" directory
    """
    # TEMP_FN_PATH = root_dir + "\\" + "False_Negative"
    TEMP_FN_PATH = os.path.join(root_dir, "False_Negative")
    if not os.path.exists(TEMP_FN_PATH):
        os.makedirs(TEMP_FN_PATH)

    if show_animation:
        purple = (128,0,128)
        for tmp_file in gt_files:
            ground_truth_data = json.load(open(tmp_file))

            # get name of corresponding image
            start = TEMP_FILES_PATH + '/'
            img_id = tmp_file[tmp_file.find(start)+len(start):tmp_file.rfind('_ground_truth.json')]
            img_cumulative_path = os.path.join(output_files_path, "images", img_id + ".jpg")
            img = cv2.imread(img_cumulative_path)

            if img is None:
                img_path = os.path.join(IMG_PATH, img_id + ".jpg")
                img = cv2.imread(img_path)
            # draw false negatives
            for obj in ground_truth_data:
                if not obj['used']:
                    TEMP_FN_PATH2 = os.path.join(TEMP_FN_PATH, str(img_cumulative_path.split(os.sep)[-1].split(".")[-2]) + ".jpg")
                    TEMP_FN_PATH3 = os.path.join(root_dir, "images", str(img_cumulative_path.split(os.sep)[-1].split(".")[-2]) + ".jpg")
                    img_fn = cv2.imread(TEMP_FN_PATH3)
                    cv2.imwrite(TEMP_FN_PATH2, img_fn)

                    """
                        take the xml according to the image name
                    """
                    format = {'xml','txt'}
       
                    xml_dir = os.path.join(root_dir, "backup")
                    image_dir = os.path.join(root_dir, "False_Negative")

                    # Check if xml_dir exists, and create it if not
                    if not os.path.exists(xml_dir):
                        os.makedirs(xml_dir, exist_ok=True)

                    for obj1 in os.scandir(xml_dir):
                        if obj1.is_dir():
                            print("Dir:",obj1.name)
                        elif obj1.is_file():
                            if obj1.name.split(".")[-1] in format:
                                if obj1.name.split(".")[-1] == "xml":
                                    xml_name = str(img_cumulative_path.split(os.sep)[-1].split(".")[-2]) + ".xml"
                                    if obj1.name == xml_name:
                                        # xml_file_dir = xml_dir + "\\" + xml_name
                                        # image_file_dir = image_dir + "\\" + xml_name
                                        xml_file_dir = os.path.join(xml_dir, xml_name)
                                        image_file_dir = os.path.join(image_dir, xml_name)
                                        shutil.copy(xml_file_dir, image_file_dir)
                                        break
                                if obj1.name.split(".")[-1] == "txt":
                                    xml_name = str(img_cumulative_path.split(os.sep)[-1].split(".")[-2]) + ".txt"
                                    if obj1.name == xml_name:
                                        xml_file_dir = os.path.join(xml_dir, xml_name)
                                        image_file_dir = os.path.join(image_dir, xml_name)
                                        shutil.copy(xml_file_dir, image_file_dir)
                                        break

                    bbgt = [ int(round(float(x))) for x in obj["bbox"].split() ]
                    cv2.rectangle(img,(bbgt[0],bbgt[1]),(bbgt[2],bbgt[3]),purple,2)
                    cv2.putText(img, "False Negative", (bbgt[0],bbgt[1] - 5), font, 0.6, light_red, 1, cv2.LINE_AA)
                    count_new_false_negative[str(obj["class_name"])] += 1

                    TEMP_FN_OUTPUT_IMG_PATH = os.path.join("output", "images", str(img_cumulative_path.split("/")[-1].split(".")[-2]) + ".jpg") # Correct path for FN image

                    fn_image_results_for_html.append({ # Append FN image info for HTML report
                        'img_no': img_cumulative_path.split("/")[-1].split(".")[-2],
                        'actual_class': obj["class_name"],
                        'predicted_class': "N/A",
                        'confidence_level': "N/A",
                        'actual_bbox': f"x:{bbgt[0]} y:{bbgt[1]} w:{bbgt[2]} h:{bbgt[3]}",
                        'predicted_bbox': "N/A",
                        'result': "False Negative",
                        'image_path': TEMP_FN_OUTPUT_IMG_PATH # Use correct FN image path
                    })

                    cv2.imwrite(img_cumulative_path, img)

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
    gt_info_plot_path = None
    if draw_plot:
        window_title = "ground-truth-info"
        plot_title = "ground-truth\n"
        plot_title += "(" + str(len(ground_truth_files_list)) + " files and " + str(n_classes) + " classes)"
        x_label = "Number of objects per class"
        gt_info_plot_path = os.path.join(output_files_path, "ground-truth-info.png")
        to_show = False
        plot_color = 'forestgreen'
        draw_plot_func(
            gt_counter_per_class,
            n_classes,
            window_title,
            plot_title,
            x_label,
            gt_info_plot_path,
            to_show,
            plot_color,
            '',
            )

    """
    Write number of ground-truth objects per class to results.txt
    """
    with open(os.path.join(output_files_path, "output.txt"), 'a') as output_file:
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

    with open(os.path.join(output_files_path, "output.txt"), 'a') as output_file:
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
    map_plot_path = None
    if draw_plot:
        window_title = "mAP"
        plot_title = "mAP = {0:.2f}%".format(mAP*100)
        x_label = "Average Precision"
        map_plot_path = os.path.join(output_files_path, "mAP.png")
        to_show = False
        plot_color = 'royalblue'
        draw_plot_func(
            ap_dictionary,
            n_classes,
            window_title,
            plot_title,
            x_label,
            map_plot_path,
            to_show,
            plot_color,
            ""
            )

    pr_curve_dir = os.path.join(output_files_path, "classes") if draw_plot else None

    """
    Ploting conf-matrix
    """
    results = {}
    category_names = []
    detection_results_graph_path = os.path.join(output_files_path, "detection_results.jpg")

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
            make_metric_graph_for_two_matric(dict2, category_names, detection_results_graph_path)

        elif check_false_positive_cnt != 4:
            append_value(dict2, key, value[0])
            append_value(dict2, key, value[1])
            append_value(dict2, key, value[2])
            category_names = ['True Positives', 'False Positives', 'False Negatives']
            make_metric_graph_for_three_matric(dict2, category_names, detection_results_graph_path)

    class_wise_accuracy = {}
    for class_name in new_graph.keys():
        tp = new_graph[class_name][0]
        fp = new_graph[class_name][1]
        fn = new_graph[class_name][2]
        class_wise_accuracy[class_name] = tp / (tp + fn) if (tp + fn) > 0 else 0

    # Calculate overall precision and recall
    total_tp = sum(new_graph[class_name][0] for class_name in new_graph.keys())
    total_fp = sum(new_graph[class_name][1] for class_name in new_graph.keys())
    total_fn = sum(new_graph[class_name][2] for class_name in new_graph.keys())

    overall_precision = total_tp / (total_tp + total_fp) if (total_tp + total_fp) > 0 else 0
    overall_recall = total_tp / (total_tp + total_fn) if (total_tp + total_fn) > 0 else 0

    report_data = {
        "gt_info_plot_path": gt_info_plot_path,
        "map_plot_path": map_plot_path,
        "pr_curve_dir": pr_curve_dir,
        "detection_results_graph_path": detection_results_graph_path,
        "class_wise_accuracy": class_wise_accuracy,
        "overall_precision": overall_precision,
        "overall_recall": overall_recall,
        "image_results": image_results_for_html,
        "fn_image_results": fn_image_results_for_html
    }

    generate_html_report("validation_report.html", report_data)
    print("FINISH.........")
    return mAP, ap_dictionary, class_wise_accuracy, overall_precision, overall_recall, new_graph

if __name__ == "__main__":
    root_dir = "/home/limonubuntu/Work/Limon/other_task/driver_anomaly/test/testing/eval_demo/ob_eval_da/categorize/da_part1"

    class_file_path = os.path.join(root_dir, "classes.txt")
    gt_classes = read_classes_from_file(class_file_path)
    mAP, ap_dictionary, class_wise_accuracy, overall_precision, overall_recall, metrics_data = calculate_mean_avg_pres(root_dir, gt_classes)

    print(f"Mean Average Precision (mAP): {mAP*100:.2f}%")
    print(f"Overall Precision: {overall_precision*100:.2f}%")
    print(f"Overall Recall: {overall_recall*100:.2f}%")
    print(f"Class-wise Accuracy: {class_wise_accuracy}")
    print(f"Metrics Data (TP, FP, FN): {metrics_data}")