import os

def save_specific_files_to_txt(root_folder, output_file, extensions):

    all_content = []

    for foldername, _, filenames in os.walk(root_folder):
        for filename in filenames:
            if any(filename.endswith(ext) for ext in extensions):
                file_path = os.path.join(foldername, filename)
                try:
                    with open(file_path, "r", encoding="utf-8") as file:
                        all_content.append(f"# File: {filename}\n" + file.read() + "\n\n")
                except Exception as e:
                    print(f"Error reading {file_path}: {e}")

    if all_content:
        with open(output_file, "w", encoding="utf-8") as txt_file:
            txt_file.writelines(all_content)
        print(f"Saved all content to {output_file}")
    else:
        print("No files with the specified extensions were found.")

if __name__ == "__main__":
    root_folder = "/home/limonubuntu/Work/Limon/other_task/driver_anomaly/test/rknn/Projects/rknn_model_zoo/examples/RetinaFace/cpp"
    output_file = "/home/limonubuntu/Work/Limon/other_task/driver_anomaly/test/rknn/Projects/rknn_model_zoo/examples/RetinaFace/all_files.txt"
    extensions = ['.cpp', '.hpp', '.h', '.c', '.cc', '.txt']

    if os.path.isdir(root_folder):
        save_specific_files_to_txt(root_folder, output_file, extensions)
    else:
        print("Invalid folder path. Please try again.")