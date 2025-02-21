import html

class HTMLTestReporter:
    """
    Generates an HTML report for test results.
    """
    @staticmethod
    def generate_html_report(test_results, report_file):
        html_content = """
        <!DOCTYPE html>
        <html>
        <head>
            <title>Test Report</title>
            <style>
                table {
                    width: 100%;
                    border-collapse: collapse;
                }
                th, td {
                    border: 1px solid black;
                    padding: 8px;
                    text-align: left;
                    vertical-align: top; /* Align text to the top in merged cells */
                }
                th {
                    background-color: #f2f2f2;
                }
                .pass {
                    background-color: #ccffcc;
                }
                .fail {
                    background-color: #ffcccc;
                }
            </style>
        </head>
        <body>
            <h1>Facial Behavior Analysis Test Report</h1>
            <table>
                <thead>
                    <tr>
                        <th>Test Case</th>
                        <th>Test Suite</th>
                        <th>Expected Output</th>
                        <th>Actual Output</th>
                        <th>Result</th>
                    </tr>
                </thead>
                <tbody>
        """
        last_test_case = None
        rowspan_count = 1 
        for i, result in enumerate(test_results):
            test_case = html.escape(result['test_case'])
            test_suite = html.escape(result['test_suite'])
            expected_output = html.escape(str(result['expected_output']))
            actual_output = html.escape(str(result['actual_output']))
            test_result = html.escape(result['result'])
            result_class = result['result'].lower()

            html_content += f"""
                    <tr>
            """
            if test_case != last_test_case: 
                if last_test_case is not None: 
                    html_content = html_content.replace(f' rowspan_placeholder_{i-1}', f' rowspan="{rowspan_count}"') 
                rowspan_count = 1
                html_content += f"""        <td rowspan_placeholder_{i}>{test_case}</td>""" 
                last_test_case = test_case
            else:
                rowspan_count += 1 
                html_content += f"""        <td></td>""" 


            html_content += f"""
                        <td>{test_suite}</td>
                        <td>{expected_output}</td>
                        <td>{actual_output}</td>
                        <td class="{result_class}">{test_result}</td>
                    </tr>
            """
        if test_results:
            html_content = html_content.replace(f' rowspan_placeholder_{len(test_results)-1}', f' rowspan="{rowspan_count}"')


        html_content += """
                </tbody>
            </table>
        </body>
        </html>
        """
        with open(report_file, "w") as f:
            f.write(html_content)