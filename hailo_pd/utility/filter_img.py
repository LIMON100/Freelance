import os
import shutil

def is_non_empty(file_path):
    """Check if a text file is non-empty."""
    return os.path.getsize(file_path) > 0

def create_filtered_folders(root_folder, output_folder):
    """
    Read non-empty label files, match with images, and copy both to a new folder.
    
    Args:
        root_folder (str): Path to root folder containing 'images' and 'labels' folders
        output_folder (str): Path to output folder where files will be copied
    """
    # Define paths
    images_folder = os.path.join(root_folder, 'images')
    labels_folder = os.path.join(root_folder, 'labels')
    output_images = os.path.join(output_folder, 'images')
    output_labels = os.path.join(output_folder, 'labels')
    
    # Create output directories if they don't exist
    os.makedirs(output_images, exist_ok=True)
    os.makedirs(output_labels, exist_ok=True)
    
    # Get list of non-empty label files
    non_empty_labels = []
    for label_file in os.listdir(labels_folder):
        if label_file.endswith('.txt'):
            label_path = os.path.join(labels_folder, label_file)
            if is_non_empty(label_path):
                non_empty_labels.append(label_file)
    
    # Copy matching images and non-empty labels
    for label_file in non_empty_labels:
        # Get the base name without extension
        base_name = os.path.splitext(label_file)[0]
        
        # Find corresponding image (supporting common image extensions)
        image_extensions = ['.jpg', '.jpeg', '.png']
        image_path = None
        for ext in image_extensions:
            potential_image = os.path.join(images_folder, base_name + ext)
            if os.path.exists(potential_image):
                image_path = potential_image
                break
        
        if image_path:
            # Copy image
            shutil.copy2(image_path, os.path.join(output_images, os.path.basename(image_path)))
            # Copy label
            shutil.copy2(os.path.join(labels_folder, label_file), 
                        os.path.join(output_labels, label_file))
            print(f"Copied: {base_name} (image and label)")
        else:
            print(f"Warning: No matching image found for label {label_file}")

def main():
    # Define root and output folders
    root_folder = '/home/limonubuntu/Work/Limon/other_task/effiencedet_hailo/dataset/roboflow_dataset/test' 
    output_folder = '/home/limonubuntu/Work/Limon/other_task/effiencedet_hailo/dataset/sorted_dataset/test'  
    
    # Validate input folders
    if not os.path.exists(os.path.join(root_folder, 'images')) or \
       not os.path.exists(os.path.join(root_folder, 'labels')):
        print("Error: 'images' or 'labels' folder not found in root directory")
        return
    
    create_filtered_folders(root_folder, output_folder)
    print("Processing complete!")

if __name__ == "__main__":
    main()