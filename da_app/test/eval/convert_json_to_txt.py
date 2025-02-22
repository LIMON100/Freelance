import json
import os
import shutil

def convert_json_to_txt_format(json_filepath, output_image_dir, output_gt_dir, class_names, image_source_dir):
    """
    Converts JSON annotation to text format: class_name left top right bottom
    and saves images and text files.

    Args:
        json_filepath (str): Path to the JSON annotation file.
        output_image_dir (str): Directory to save images.
        output_gt_dir (str): Directory to save ground-truth text files.
        class_names (list): List of class names (not used for class IDs in this format, but for filtering if needed).
        image_source_dir (str): Directory where original images are located.
    """

    with open(json_filepath, 'r') as f:
        json_data = json.load(f)

    filename = json_data['FileInfo']['FileName']

    txt_lines = []
    object_info = json_data['ObjectInfo']['BoundingBox']

    for class_name_json, bbox_data in object_info.items():
        if bbox_data['isVisible']: 
            position = bbox_data['Position']
            if not all(p == 0 for p in position):
                left, top, right, bottom = map(int, map(float, position)) 

                class_name = class_name_json.capitalize() 

                txt_line = f"{class_name} {left} {top} {right} {bottom}"
                txt_lines.append(txt_line)

    # Save annotations to a text file
    txt_filename = os.path.splitext(filename)[0] + '.txt'
    output_txt_filepath = os.path.join(output_gt_dir, txt_filename)

    os.makedirs(output_gt_dir, exist_ok=True)

    with open(output_txt_filepath, 'w') as outfile:
        for line in txt_lines:
            outfile.write(line + '\n')

    # Copy the image to the output_image_dir
    image_source_path = os.path.join(image_source_dir, filename) 
    output_image_path = os.path.join(output_image_dir, filename)
    os.makedirs(output_image_dir, exist_ok=True)
    try:
        shutil.copy(image_source_path, output_image_path)
    except FileNotFoundError:
        print(f"Warning: Image file not found at: {image_source_path}. Not copying image.")


if __name__ == "__main__":
    json_annotation_dir = '/home/limonubuntu/Work/Limon/other_task/driver_anomaly/test/test_dataset/output_g/all'  # Directory containing your JSON annotation files
    output_images_directory = 'images'        # Directory to save images (same as source in this case)
    output_ground_truth_directory = 'ground-truth' # Directory to save ground-truth text files
    original_images_directory = '/home/limonubuntu/Work/Limon/other_task/driver_anomaly/test/test_dataset/output_g/all' # Directory where original images are located - **SET TO 'images' NOW**

    # Class name
    da_classes = ["Face", "Leye", "Reye", "Mouth", "Cigar", "Phone"]

    # Create da_classes.txt file - Not needed for this format
    classes_txt_path = os.path.join('da_classes.txt') 
    os.makedirs(output_ground_truth_directory, exist_ok=True)
    with open(classes_txt_path, 'w') as f:
        for class_name in da_classes:
            f.write(class_name + '\n')
    print(f"da_classes.txt created at: {classes_txt_path}")

    # Create output image directory if it doesn't exist
    os.makedirs(output_images_directory, exist_ok=True)
    os.makedirs(output_ground_truth_directory, exist_ok=True) 


    json_files = [f for f in os.listdir(json_annotation_dir) if f.endswith('.json')]

    for json_file in json_files:
        json_file_path = os.path.join(json_annotation_dir, json_file)
        convert_json_to_txt_format(json_file_path, output_images_directory, output_ground_truth_directory, da_classes, original_images_directory) # Pass original_images_directory
        print(f"Converted {json_file} to text format and copied image.")

    print("Conversion and image copying complete!")
    print(f"Ground-truth text files saved in: {output_ground_truth_directory}")
    print(f"Images saved in: {output_images_directory}")