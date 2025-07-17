import os
import random

def collect_wav_files(root_folder, output_file, num_files=100):
    """
    Traverse root folder, collect all .wav files, randomly select num_files, and save to output_file.
    
    Args:
        root_folder (str): Path to the root folder to start traversal.
        output_file (str): Path to the output text file to save .wav file names.
        num_files (int): Number of .wav files to randomly select (default: 100).
    """
    # List to store all .wav file paths
    wav_files = []
    
    # Walk through the root folder and its subfolders
    for dirpath, _, filenames in os.walk(root_folder):
        # Filter for .wav files and store their full paths
        for file in filenames:
            if file.lower().endswith('.wav'):
                wav_files.append(os.path.join(dirpath, file))
    
    # Check if enough .wav files are found
    if len(wav_files) < num_files:
        print(f"Warning: Only {len(wav_files)} .wav files found, less than requested {num_files}.")
        selected_files = wav_files
    else:
        # Randomly select num_files from the list
        selected_files = random.sample(wav_files, num_files)
    
    # Write the selected file paths to the output text file
    with open(output_file, 'w', encoding='utf-8') as f:
        for file_path in selected_files:
            f.write(file_path + '\n')
    
    print(f"Successfully saved {len(selected_files)} .wav file paths to {output_file}")

# Example usage
if __name__ == "__main__":
    root_folder = "/home/limonubuntu/Work/Limon/other_task/ble_project/audio_data/merge_korean/all_comand" 
    output_file = "quant_dataset.txt"    # Output text file name
    collect_wav_files(root_folder, output_file)