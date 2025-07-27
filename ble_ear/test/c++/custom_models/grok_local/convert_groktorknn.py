# # convert_grok3_final.py
# import sys
# from rknn.api import RKNN

# # --- Paths ---
# ONNX_MODEL_PATH = 'grok_model.onnx'
# RKNN_OUTPUT_PATH = 'grok3_quantized.rknn'
# # --- THE FIX: Point to the list of pre-processed .npy files ---
# QUANT_DATASET_LIST = 'spectrogram_quant_list.txt' 
# TARGET_PLATFORM = 'rk3588'


# if __name__ == '__main__':
#     rknn = RKNN(verbose=True)

#     print('--> Config model')
#     rknn.config(
#         target_platform=TARGET_PLATFORM,
#         quantized_dtype='asymmetric_quantized-8',
#         optimization_level=3,
#         # Model was trained on spectrograms, mean/std likely not needed
#         mean_values=[],
#         std_values=[]
#     )
#     print('done')

#     print('--> Loading ONNX model')
#     ret = rknn.load_onnx(model=ONNX_MODEL_PATH)
#     if ret != 0:
#         print('Load model failed!')
#         exit(ret)
#     print('done')

#     print('--> Building model with proper quantization')
#     # The dataset parameter now points to a list of .npy files.
#     # The toolkit's built-in loader can handle these perfectly.
#     ret = rknn.build(do_quantization=True, dataset=QUANT_DATASET_LIST)
#     if ret != 0:
#         print('Build model failed!')
#         exit(ret)
#     print('done')

#     print('--> Exporting RKNN')
#     ret = rknn.export_rknn(RKNN_OUTPUT_PATH)
#     if ret != 0:
#         print('Export rknn model failed!')
#         exit(ret)
#     print(f'Model exported successfully to → {RKNN_OUTPUT_PATH}')
#     rknn.release()




# convert_grok3_final.py
import sys
import os
from rknn.api import RKNN

# --- CONFIGURATION ---
# The path to the text file listing your pre-processed .npy files.
# This is ONLY used for INT8 quantization.
QUANT_DATASET_LIST = './spectrogram_quant_list.txt' 
TARGET_PLATFORM = 'rk3588'


def parse_args():
    """
    Parses command-line arguments to get the model path and desired data type.
    """
    if len(sys.argv) < 3:
        print(f"Usage: python3 {sys.argv[0]} <onnx_model_path> <dtype>")
        print("       dtype can be 'i8' (for INT8 quantization) or 'fp' (for FP16).")
        exit(1)
    
    model_path = sys.argv[1]
    dtype = sys.argv[2].lower()
    
    if dtype not in ['i8', 'fp']:
        print("Invalid dtype. Please choose 'i8' or 'fp'.")
        exit(1)
        
    # Automatically generate the output file name
    output_path = model_path.replace('.onnx', f'_{dtype}.rknn')
    
    return model_path, dtype, output_path


if __name__ == '__main__':
    onnx_path, dtype, rknn_path = parse_args()
    
    do_quant = (dtype == 'i8')

    # Create the RKNN object
    rknn = RKNN(verbose=True)

    print('--> Configuring model...')
    config = {
        'target_platform': TARGET_PLATFORM,
        'optimization_level': 3,
        'mean_values': [],  # Spectrograms are usually normalized differently, not with mean/std
        'std_values': []
    }
    
    if do_quant:
        # Configuration for INT8 quantization
        config['quantized_dtype'] = 'asymmetric_quantized-8'
        print("Mode: INT8 Quantization")
    else:
        # Configuration for FP16 conversion
        config['float_dtype'] = 'float16'
        print("Mode: FP16 Conversion")

    rknn.config(**config)
    print('done')

    print('--> Loading ONNX model...')
    # The grok3.onnx model has a fixed input shape, so we don't need to specify input_size_list
    ret = rknn.load_onnx(model=onnx_path)
    if ret != 0:
        print('Load model failed!')
        rknn.release()
        exit(ret)
    print('done')

    # The dataset is ONLY needed for INT8 quantization
    dataset_for_build = None
    if do_quant:
        print(f"INT8 quantization enabled. Using dataset list: {QUANT_DATASET_LIST}")
        if not os.path.exists(QUANT_DATASET_LIST):
            print(f"FATAL ERROR: Quantization dataset list not found at '{QUANT_DATASET_LIST}'")
            print("Please run the 'preprocess_for_quant.py' script first, then create the list file.")
            rknn.release()
            exit(1)
        dataset_for_build = QUANT_DATASET_LIST

    print('--> Building model...')
    ret = rknn.build(do_quantization=do_quant, dataset=dataset_for_build)
    if ret != 0:
        print('Build model failed!')
        rknn.release()
        exit(ret)
    print('done')

    print('--> Exporting RKNN model...')
    ret = rknn.export_rknn(rknn_path)
    if ret != 0:
        print('Export rknn model failed!')
        rknn.release()
        exit(ret)
    print(f'\nModel exported successfully to → {rknn_path}')
    
    # Clean up
    rknn.release()