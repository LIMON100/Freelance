import sys
import os
from rknn.api import RKNN

# --- CONFIGURATION ---
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
        'mean_values': [],
        'std_values': []
    }
    
    if do_quant:
        config['quantized_dtype'] = 'asymmetric_quantized-8'
        print("Mode: INT8 Quantization")
    else:
        config['float_dtype'] = 'float16'
        print("Mode: FP16 Conversion")

    rknn.config(**config)
    print('done')

    print('--> Loading ONNX model...')
    
    # =========================================================================
    # THE FIX IS HERE: We must provide a fixed shape for the dynamic input.
    # The input name is 'input' and the shape is [batch, channels, height, width]
    # For single-clip inference, batch size is always 1.
    # =========================================================================
    ret = rknn.load_onnx(model=onnx_path, 
                          inputs=['input'], 
                          input_size_list=[[1, 1, 171, 560]])
                          
    if ret != 0:
        print('Load model failed!')
        rknn.release()
        exit(ret)
    print('done')

    dataset_for_build = None
    if do_quant:
        print(f"INT8 quantization enabled. Using dataset list: {QUANT_DATASET_LIST}")
        if not os.path.exists(QUANT_DATASET_LIST):
            print(f"FATAL ERROR: Quantization dataset list not found at '{QUANT_DATASET_LIST}'")
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
    
    rknn.release()