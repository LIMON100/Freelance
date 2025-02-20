import os
import shutil
import json
import random
from collections import defaultdict

def collect_json_paths(parent_dir):
    json_paths = defaultdict(list)  
    for root, _, files in os.walk(parent_dir):
        for file in files:
            if file.endswith(".json"):
                class_name = os.path.basename(root) 
                json_paths[class_name].append(os.path.join(root, file))
    return json_paths

def create_dataset(actual_dir, control_dir, train_config, test_config, output_dir):

    train_dir = os.path.join(output_dir, "train")
    test_dir = os.path.join(output_dir, "test")
    os.makedirs(train_dir, exist_ok=True)
    os.makedirs(test_dir, exist_ok=True)


    actual_json_paths = collect_json_paths(actual_dir)
    control_json_paths = collect_json_paths(control_dir)

    for class_name, count in train_config.items():
        combined_paths = actual_json_paths[class_name] + control_json_paths[class_name]
        random.shuffle(combined_paths)  # Ensure diversity

       # added control and actual distribution
        actual_count = count // 2  # Integer division to get half for actual
        control_count = count - actual_count  # Remaining half for control

        actual_selected = combined_paths[:min(actual_count, len(actual_json_paths[class_name]))]
        control_selected = combined_paths[len(actual_json_paths[class_name]):][:min(control_count, len(control_json_paths[class_name]))]

        selected_paths = actual_selected+control_selected
        class_train_dir = os.path.join(train_dir, class_name)
        os.makedirs(class_train_dir, exist_ok=True)
        for path in selected_paths:
            shutil.copy(path, class_train_dir)


    for class_name, count in test_config.items():
        combined_paths = actual_json_paths[class_name] + control_json_paths[class_name]
        random.shuffle(combined_paths)

        actual_count = count // 2  # Integer division to get half for actual
        control_count = count - actual_count  # Remaining half for control

        actual_selected = combined_paths[:min(actual_count, len(actual_json_paths[class_name]))]
        control_selected = combined_paths[len(actual_json_paths[class_name]):][:min(control_count, len(control_json_paths[class_name]))]

        selected_paths = actual_selected+control_selected

        class_test_dir = os.path.join(test_dir, class_name)
        os.makedirs(class_test_dir, exist_ok=True)
        for path in selected_paths:
             shutil.copy(path, class_test_dir)


if __name__ == "__main__":
    actual_dir = "path_to_actual_environment"  
    control_dir = "path_to_controlled_environment"  
    output_dir = "path_to_the_output_directory"

    train_config = {
      "Cigar": 11000, "Phone": 11000, "Face": 11000, "Leye_Open": 11000, "Leye_Closed": 11000,
      "Reye_Open": 11000,"Reye_Closed": 11000,"Mouth_Open": 11000,"Mouth_Closed": 11000
    }
    test_config = {
       "Cigar": 1200, "Phone": 1200, "Face": 1200, "Leye_Open": 1200, "Leye_Closed": 1200,
      "Reye_Open": 1200,"Reye_Closed": 1200,"Mouth_Open": 1200,"Mouth_Closed": 1200
    }

    create_dataset(actual_dir, control_dir, train_config, test_config, output_dir)
    print("Dataset created successfully!")