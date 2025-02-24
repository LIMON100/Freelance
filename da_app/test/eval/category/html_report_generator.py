import datetime
import os

def generate_html_report(output_file_path, report_data):
    """
    Generates an HTML report for object detection evaluation.

    Args:
        output_file_path (str): Path to save the HTML report.
        report_data (dict): Dictionary containing report data like metrics,
                             images, and other information to be included.
    """
    with open(output_file_path, "w") as f_html:

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
                    <td><h5>A true positive result is when model correctly labels or categorizes an image.</h5></td>
                </tr>
                <tr>
                    <td><h5>False Positive</h5></td>
                    <td><h5>A false positive result is when model labels or categorizes an image when it should not have.</h5></td>
                </tr>
                <tr>
                    <td><h5>False Negative</h5></td>
                    <td><h5>A false negative result is when model does not label or categorize an image, but should have.</h5></td>
                </tr>
                <tr>
                    <td><h5>Precision</h5></td>
                    <td><h5>Precision measures the accuracy of the model's positive predictions.</h5></td>
                </tr>
                <tr>
                    <td><h5>Recall</h5></td>
                    <td><h5>Recall measures the model's ability to detect all relevant objects (ground truths).</h5></td>
                </tr>
                <tr>
                    <td><h5>mAP(Mean Average Precision)</p></h5></td>
                    <td><h5>mAP is the average of the Average Precision (AP) scores across all classes in a dataset.</h5></td>
                </tr>
                <tr>
                    <td><h5>AP(Average precision)</h5></td>
                    <td><h5>AP is the area under the Precision-Recall curve for a single class. It summarizes the trade-off between precision and recall at different confidence thresholds.</h5></td>
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
                    <th col-index = 1>Img No.   </th>

                    <th col-index = 2>Actual Class
                        <select class="table-filter" onchange="filter_rows()">
                            <option value="all"></option>
                        </select>   </th>

                    <th col-index = 3>Predicted Class
                        <select class="table-filter" onchange="filter_rows()">
                            <option value="all"></option>
                        </select>   </th>

                    <th col-index = 4>Confidence level
                        <select class="table-filter" onchange="filter_rows()">
                            <option value="all"></option>
                        </select>   </th>

                    <th col-index = 5>Actual Bounding Box (x,y),(w,h)     </th>
                    <th col-index = 6>predicted Bounding Box (x,y), (w,h)     </th>

                    <th col-index = 7>Result
                        <select class="table-filter" onchange="filter_rows()">
                            <option value="all"></option>
                        </select>    </th>

                    <th col-index = 8>Image     </th>

                </thead>
                <tbody>
        '''
        f_html.write(message3)



        # Write image-wise detection results to HTML table (TP/FP results)
        for item in report_data.get("image_results", []): # Process TP/FP results
    
            f_html.write("<tr>")
            f_html.write(f"<td><h4>{item.get('img_no', '')}</h4></td>")
            f_html.write(f"<td><h4>{item.get('actual_class', '')}</h4></td>")
            f_html.write(f"<td><h4>{item.get('predicted_class', '')}</h4></td>")
            f_html.write(f"<td><h4>{item.get('confidence_level', '')}</h4></td>")
            f_html.write(f"<td><h4>{item.get('actual_bbox', '')}</h4></td>")
            f_html.write(f"<td><h4>{item.get('predicted_bbox', '')}</h4></td>")
            f_html.write(f"<td><h4>{item.get('result', '')}</h4></td>")
            f_html.write(f"<td><a href='{item.get('image_path', '')}'><img src='{item.get('image_path', '')}' width='400'></a></td>") # Adjust width as needed
            f_html.write("</tr>")


        # Write image-wise results for False Negatives (FNs)
        for item in report_data.get("fn_image_results", []): # Process FN results
            f_html.write("<tr>")
            f_html.write(f"<td><h4>{item.get('img_no', '')}</h4></td>")
            f_html.write(f"<td><h4>{item.get('actual_class', '')}</h4></td>")
            f_html.write(f"<td><h4>{item.get('predicted_class', '')}</h4></td>") # Should be "N/A" or similar for FN
            f_html.write(f"<td><h4>{item.get('confidence_level', '')}</h4></td>") # Should be "N/A" for FN
            f_html.write(f"<td><h4>{item.get('actual_bbox', '')}</h4></td>")
            f_html.write(f"<td><h4>{item.get('predicted_bbox', '')}</h4></td>") # Should be "N/A" for FN
            f_html.write(f"<td><h4>{item.get('result', '')}</h4></td>") # Should be "False Negative"
            f_html.write(f"<td><a href='{item.get('image_path', '')}'><img src='{item.get('image_path', '')}' width='400'></a></td>") # Adjust width as needed
            f_html.write("</tr>")


        f_html.write("</tbody>")
        f_html.write("</table>")
        f_html.write("<script>"' window.onload = () => {console.log(document.querySelector("#emp-table > tbody > tr:nth-child(1) > td:nth-child(2) ").innerHTML);}; getUniqueValuesFromColumn() '"</script>")
        f_html.write("</body></html>")


        # Include ground-truth info plot
        if "gt_info_plot_path" in report_data:
            f_html.write("<pre><h1>" + "Detection result" + "</h1></pre> <br>\n")
            f_html.write(f'<a><img src="{report_data["gt_info_plot_path"]}"></a>')
            f_html.write("</body></html>")

        # Include mAP plot
        if "map_plot_path" in report_data:
            f_html.write("<pre><h1>" + "MAP(Mean average precision)" + "</h1></pre> <br>\n")
            f_html.write(f'<a><img src="{report_data["map_plot_path"]}"></a>')
            f_html.write("</body></html>")

        # Include PR curves
        if "pr_curve_dir" in report_data:
            f_html.write("<pre><h1>" + "Precision Recall Curve" + "</h1></pre> <br>\n")
            pr_directory = report_data["pr_curve_dir"]
            for filename in os.listdir(pr_directory):
                f = os.path.join(pr_directory, filename)
                f_html.write(f'<a><img src="{f}"></a>')

        # Include detection results graph
        if "detection_results_graph_path" in report_data:
            f_html.write("<pre><h1>" + "True positve, False positive, False negetive Graph" + "</h1></pre> <br>\n")
            f_html.write(f'<a><img src="{report_data["detection_results_graph_path"]}"></a>')
            f_html.write("</body></html>")

        # Include class-wise accuracy
        if "class_wise_accuracy" in report_data:
            f_html.write("<pre><h1>" + "Class-wise accuracy" + "</h1></pre> <br>\n")
            for class_name, accuracy in report_data["class_wise_accuracy"].items():
                message_new = "<pre><h3>" + class_name + " : " + '%.2f' % (accuracy * 100) + "%" + "</h3></pre> <br>\n"
                f_html.write(message_new)
            f_html.write("</body></html>")

        # Include overall precision and recall
        if "overall_precision" in report_data and "overall_recall" in report_data:
            f_html.write("<pre><h1>" + "Overall Precision and Recall" + "</h1></pre> <br>\n")
            f_html.write(f"<pre><h3>Overall Precision: {report_data['overall_precision'] * 100:.2f}%</h3></pre> <br>\n")
            f_html.write(f"<pre><h3>Overall Recall: {report_data['overall_recall'] * 100:.2f}%</h3></pre> <br>\n")
            f_html.write("</body></html>")