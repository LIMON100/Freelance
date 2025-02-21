# import unittest
# import os
# import cv2
# import main
# from html_reporter import HTMLTestReporter  # Import HTMLTestReporter


# class TestFacialBehaviorAnalysisCountsHTMLReport(unittest.TestCase):

#     @classmethod
#     def setUpClass(cls):
#         cls.image_folder = "/home/limonubuntu/Work/Limon/other_task/driver_anomaly/test/test_dataset/output_g/images"
#         cls.yawn_test_folder =  "/home/limonubuntu/Work/Limon/other_task/driver_anomaly/test/test_dataset/output_g/no_yawn"
#         cls.output_folder = "test_output"
#         cls.report_file = "test_report.html"
#         cls.test_results = []

#         # Define ground truth counts directly here
#         cls.ground_truth_counts = {
#             "Face Visible": 298,
#             "Eyes Open": 176,
#             "Eyes Closed": 114,
#             "Mouth Closed": 210,
#             "Mouth Open": 88,
#         }

#         #same initialize method
#         cls.face_mesh, cls.mp_face_mesh, cls.mp_drawing, cls.mp_drawing_styles = main.initialize_mediapipe()
#         cls.blink_detector, cls.head_pose_tracker, cls.yawn_detector = main.initialize_models(cls.mp_face_mesh, cls.mp_drawing, cls.mp_drawing_styles)

#         if not os.path.exists(cls.output_folder):
#             os.makedirs(cls.output_folder)


#     def test_initialize_mediapipe(self):
#         result = "PASSED"
#         try:
#             self.assertIsNotNone(self.face_mesh)
#             self.assertIsNotNone(self.mp_face_mesh)
#             self.assertIsNotNone(self.mp_drawing)
#             self.assertIsNotNone(self.mp_drawing_styles)
#         except AssertionError:
#             result = "FAILED"
#         self.test_results.append({"test_case": "Initialization", "test_suite": "Mediapipe", "expected_output": "Objects Initialized", "actual_output": "Objects Initialized" if result == "PASSED" else "Initialization Error", "result": result})


#     def test_initialize_models(self):
#         result = "PASSED"
#         try:
#             self.assertIsNotNone(self.blink_detector)
#             self.assertIsNotNone(self.head_pose_tracker)
#             self.assertIsNotNone(self.yawn_detector)
#         except AssertionError:
#             result = "FAILED"
#         self.test_results.append({"test_case": "Initialization", "test_suite": "Models", "expected_output": "Models Initialized", "actual_output": "Models Initialized" if result == "PASSED" else "Initialization Error", "result": result})


#     def test_process_image_counts(self):
#         face_visible_count = 0
#         eye_open_count = 0
#         eye_closed_count = 0
#         mouth_open_count = 0
#         mouth_closed_count = 0

#         test_results_counts = [] # Store results for count-based tests

#         for filename in os.listdir(self.image_folder):
#             if filename.lower().endswith(('.png', '.jpg', '.jpeg')):
#                 image_path = os.path.join(self.image_folder, filename)
#                 try:
#                     output_data, _ = main.process_image(
#                         image_path, self.face_mesh, self.blink_detector, self.head_pose_tracker, self.yawn_detector
#                     )

#                     if output_data:
#                         face_visible_count += 1

#                         if output_data["eye"] == "open":
#                             eye_open_count += 1
#                         elif output_data["eye"] == "closed":
#                             eye_closed_count += 1

#                         if output_data["mouth"] == "open":
#                             mouth_open_count += 1
#                         elif output_data["mouth"] == "closed":
#                             mouth_closed_count += 1


#                 except Exception as e:
#                     print(f"Error processing image {filename}: {str(e)}")

#         # Test and Result for Face Visible
#         face_visible_result = "PASSED"
#         try:
#             self.assertEqual(face_visible_count, self.ground_truth_counts["Face Visible"], "Face Visible count mismatch")
#         except AssertionError:
#             face_visible_result = "FAILED"
#         test_results_counts.append({"test_case": "Face Detection", "test_suite": "Face Visibility", "expected_output": self.ground_truth_counts["Face Visible"], "actual_output": face_visible_count, "result": face_visible_result})


#         # Test and Result for Eye Detection - Eye Open
#         eye_open_result = "PASSED"
#         try:
#             self.assertEqual(eye_open_count, self.ground_truth_counts["Eyes Open"], "Eyes Open count mismatch")
#         except AssertionError:
#             eye_open_result = "FAILED"
#         test_results_counts.append({"test_case": "Eye Detection", "test_suite": "Eye Open", "expected_output": self.ground_truth_counts["Eyes Open"], "actual_output": eye_open_count, "result": eye_open_result})


#         # Test and Result for Eye Detection - Eye Closed
#         eye_closed_result = "PASSED"
#         try:
#             self.assertEqual(eye_closed_count, self.ground_truth_counts["Eyes Closed"], "Eyes Closed count mismatch")
#         except AssertionError:
#             eye_closed_result = "FAILED"
#         test_results_counts.append({"test_case": "Eye Detection", "test_suite": "Eye Closed", "expected_output": self.ground_truth_counts["Eyes Closed"], "actual_output": eye_closed_count, "result": eye_closed_result})


#         # Test and Result for Mouth Detection - Mouth Closed
#         mouth_closed_result = "PASSED"
#         try:
#             self.assertEqual(mouth_closed_count, self.ground_truth_counts["Mouth Closed"], "Mouth Closed count mismatch")
#         except AssertionError:
#             mouth_closed_result = "FAILED"
#         test_results_counts.append({"test_case": "Mouth Detection", "test_suite": "Mouth Closed", "expected_output": self.ground_truth_counts["Mouth Closed"], "actual_output": mouth_closed_count, "result": mouth_closed_result})


#         # Test and Result for Mouth Open
#         mouth_open_result = "PASSED"
#         try:
#             self.assertEqual(mouth_open_count, self.ground_truth_counts["Mouth Open"], "Mouth Open count mismatch")
#         except AssertionError:
#             mouth_open_result = "FAILED"
#         test_results_counts.append({"test_case": "Mouth Detection", "test_suite": "Mouth Open", "expected_output": self.ground_truth_counts["Mouth Open"], "actual_output": mouth_open_count, "result": mouth_open_result})

#         self.test_results.extend(test_results_counts) # Append count-based test results


#     @classmethod
#     def tearDownClass(cls):
#         HTMLTestReporter.generate_html_report(cls.test_results, cls.report_file) # Call generate_html_report from HTMLTestReporter class
#         print(f"HTML report generated: {cls.report_file}")


# if __name__ == '__main__':
#     unittest.main()



import unittest
import os
import cv2
import main
from html_reporter import HTMLTestReporter  # Import HTMLTestReporter


class TestFacialBehaviorAnalysisCountsHTMLReport(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.image_folder = "/home/limonubuntu/Work/Limon/other_task/driver_anomaly/test/test_dataset/output_g/images" 
        cls.yawn_image_folder = "/home/limonubuntu/Work/Limon/other_task/driver_anomaly/test/test_dataset/no_yawn" 
        cls.drowsy_image_folder = "/home/limonubuntu/Work/Limon/other_task/driver_anomaly/test/test_dataset/sleep" 
        cls.output_folder = "test_output"
        cls.report_file = "test_report.html"
        cls.test_results = []

        # Define ground truth counts directly here
        cls.ground_truth_counts = {
            "Face Visible": 298,
            "Eyes Open": 176,
            "Eyes Closed": 114,
            "Mouth Closed": 210,
            "Mouth Open": 88,
            "Yawn Count": 1,  # Ground truth for yawn test case
            "Drowsiness Events": 2 # Ground truth for drowsiness test case
        }

        #same initialize method
        cls.face_mesh, cls.mp_face_mesh, cls.mp_drawing, cls.mp_drawing_styles = main.initialize_mediapipe()
        cls.blink_detector, cls.head_pose_tracker, cls.yawn_detector = main.initialize_models(cls.mp_face_mesh, cls.mp_drawing, cls.mp_drawing_styles)

        if not os.path.exists(cls.output_folder):
            os.makedirs(cls.output_folder)


    def test_initialize_mediapipe(self):
        result = "Pass"
        try:
            self.assertIsNotNone(self.face_mesh)
            self.assertIsNotNone(self.mp_face_mesh)
            self.assertIsNotNone(self.mp_drawing)
            self.assertIsNotNone(self.mp_drawing_styles)
        except AssertionError:
            result = "Fail"
        self.test_results.append({"test_case": "Initialization", "test_suite": "Mediapipe", "expected_output": "Objects Initialized", "actual_output": "Objects Initialized" if result == "Pass" else "Initialization Error", "result": result})


    def test_initialize_models(self):
        result = "Pass"
        try:
            self.assertIsNotNone(self.blink_detector)
            self.assertIsNotNone(self.head_pose_tracker)
            self.assertIsNotNone(self.yawn_detector)
        except AssertionError:
            result = "Fail"
        self.test_results.append({"test_case": "Initialization", "test_suite": "Models", "expected_output": "Models Initialized", "actual_output": "Models Initialized" if result == "Pass" else "Initialization Error", "result": result})


    def test_process_image_counts(self): #test for default image folder
        face_visible_count = 0
        eye_open_count = 0
        eye_closed_count = 0
        mouth_open_count = 0
        mouth_closed_count = 0

        test_results_counts = [] # Store results for count-based tests

        for filename in os.listdir(self.image_folder):
            if filename.lower().endswith(('.png', '.jpg', '.jpeg')):
                image_path = os.path.join(self.image_folder, filename)
                try:
                    output_data, _ = main.process_image(
                        image_path, self.face_mesh, self.blink_detector, self.head_pose_tracker, self.yawn_detector
                    )

                    if output_data:
                        face_visible_count += 1

                        if output_data["eye"] == "open":
                            eye_open_count += 1
                        elif output_data["eye"] == "closed":
                            eye_closed_count += 1

                        if output_data["mouth"] == "open":
                            mouth_open_count += 1
                        elif output_data["mouth"] == "closed":
                            mouth_closed_count += 1


                except Exception as e:
                    print(f"Error processing image {filename}: {str(e)}")

        # Test and Result for Face Visible
        face_visible_result = "Pass"
        try:
            self.assertEqual(face_visible_count, self.ground_truth_counts["Face Visible"], "Face Visible count mismatch")
        except AssertionError:
            face_visible_result = "Fail"
        test_results_counts.append({"test_case": "Face Detection", "test_suite": "Face Visibility", "expected_output": self.ground_truth_counts["Face Visible"], "actual_output": face_visible_count, "result": face_visible_result})


        # Test and Result for Eye Detection - Eye Open
        eye_open_result = "Pass"
        try:
            self.assertEqual(eye_open_count, self.ground_truth_counts["Eyes Open"], "Eyes Open count mismatch")
        except AssertionError:
            eye_open_result = "Fail"
        test_results_counts.append({"test_case": "Eye Detection", "test_suite": "Eye Open", "expected_output": self.ground_truth_counts["Eyes Open"], "actual_output": eye_open_count, "result": eye_open_result})


        # Test and Result for Eye Detection - Eye Closed
        eye_closed_result = "Pass"
        try:
            self.assertEqual(eye_closed_count, self.ground_truth_counts["Eyes Closed"], "Eyes Closed count mismatch")
        except AssertionError:
            eye_closed_result = "Fail"
        test_results_counts.append({"test_case": "Eye Detection", "test_suite": "Eye Closed", "expected_output": self.ground_truth_counts["Eyes Closed"], "actual_output": eye_closed_count, "result": eye_closed_result})


        # Test and Result for Mouth Detection - Mouth Closed
        mouth_closed_result = "Pass"
        try:
            self.assertEqual(mouth_closed_count, self.ground_truth_counts["Mouth Closed"], "Mouth Closed count mismatch")
        except AssertionError:
            mouth_closed_result = "Fail"
        test_results_counts.append({"test_case": "Mouth Detection", "test_suite": "Mouth Closed", "expected_output": self.ground_truth_counts["Mouth Closed"], "actual_output": mouth_closed_count, "result": mouth_closed_result})


        # Test and Result for Mouth Open
        mouth_open_result = "Pass"
        try:
            self.assertEqual(mouth_open_count, self.ground_truth_counts["Mouth Open"], "Mouth Open count mismatch")
        except AssertionError:
            mouth_open_result = "Fail"
        test_results_counts.append({"test_case": "Mouth Detection", "test_suite": "Mouth Open", "expected_output": self.ground_truth_counts["Mouth Open"], "actual_output": mouth_open_count, "result": mouth_open_result})

        self.test_results.extend(test_results_counts) # Append count-based test results


    def test_process_yawn_detection(self): # New test case for yawn detection
        yawn_detected_count = 0

        for filename in os.listdir(self.yawn_image_folder): # Iterate over yawn dataset
            if filename.lower().endswith(('.png', '.jpg', '.jpeg')):
                image_path = os.path.join(self.yawn_image_folder, filename)
                try:
                    output_data, _ = main.process_image(
                        image_path, self.face_mesh, self.blink_detector, self.head_pose_tracker, self.yawn_detector
                    )

                    if output_data and output_data.get("Yawning") == "Yes": # Check if yawn is detected
                        yawn_detected_count += 1

                except Exception as e:
                    print(f"Error processing image {filename} for yawn detection: {str(e)}")

        yawn_test_result = "Pass"
        try:
            self.assertEqual(yawn_detected_count, self.ground_truth_counts["Yawn Count"], "Yawn count mismatch")
        except AssertionError:
            yawn_test_result = "Fail"
        self.test_results.append({"test_case": "Yawn Detection Test", "test_suite": "Yawn Count", "expected_output": self.ground_truth_counts["Yawn Count"], "actual_output": yawn_detected_count, "result": yawn_test_result})


    def test_process_drowsiness_detection(self): # New test case for drowsiness detection
        drowsiness_event_count = 0
        consecutive_closed_eyes = 0 # Counter for consecutive closed eye frames

        for filename in os.listdir(self.drowsy_image_folder): # Iterate over drowsy dataset
            if filename.lower().endswith(('.png', '.jpg', '.jpeg')):
                image_path = os.path.join(self.drowsy_image_folder, filename)
                try:
                    output_data, _ = main.process_image(
                        image_path, self.face_mesh, self.blink_detector, self.head_pose_tracker, self.yawn_detector
                    )

                    if output_data:
                        if output_data["eye"] == "closed":
                            consecutive_closed_eyes += 1
                        else: # Eyes open, check for drowsiness event if we had consecutive closed eyes
                            if consecutive_closed_eyes >= 3: # Define drowsiness as 3 or more consecutive closed eye frames
                                drowsiness_event_count += 1
                            consecutive_closed_eyes = 0 # Reset counter when eyes open
                except Exception as e:
                    print(f"Error processing image {filename} for drowsiness detection: {str(e)}")
        # Check for a drowsiness event if the video ends with closed eyes
        if consecutive_closed_eyes >= 3:
             drowsiness_event_count += 1


        drowsiness_test_result = "Pass"
        try:
            self.assertEqual(drowsiness_event_count, self.ground_truth_counts["Drowsiness Events"], "Drowsiness event count mismatch")
        except AssertionError:
            drowsiness_test_result = "Fail"
        self.test_results.append({"test_case": "Drowsiness Detection Test", "test_suite": "Drowsiness Events", "expected_output": self.ground_truth_counts["Drowsiness Events"], "actual_output": drowsiness_event_count, "result": drowsiness_test_result})


    @classmethod
    def tearDownClass(cls):
        HTMLTestReporter.generate_html_report(cls.test_results, cls.report_file) # Call generate_html_report from HTMLTestReporter class
        print(f"HTML report generated: {cls.report_file}")


if __name__ == '__main__':
    unittest.main()