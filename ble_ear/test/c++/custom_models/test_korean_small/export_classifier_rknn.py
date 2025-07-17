import sys
from rknn.api import RKNN

# --- Model-specific constants for clarity ---
# These match the details from your working ONNX script
CLASSIFIER_INPUT_NAME = 'input'
CLASSIFIER_INPUT_SHAPE = [1, 768]


def parse_arg():
    """
    Parses command-line arguments. This is based on your provided script.
    """
    if len(sys.argv) < 3:
        print("Usage: python3 {} onnx_model_path [platform] [dtype(optional)] [output_rknn_path(optional)]".format(sys.argv[0]))
        print("       platform choose from [rk3562,rk3566,rk3568,rk3576,rk3588]")
        print("       dtype MUST be 'fp' for this script.")
        exit(1)

    model_path = sys.argv[1]
    platform = sys.argv[2]
    
    # For this script, we are specifically doing FP16 conversion.
    do_quant = False
    if len(sys.argv) > 3:
        model_type = sys.argv[3]
        if model_type.lower() != 'fp':
            print("ERROR: This script is for FP16 conversion. Please use 'fp' as the dtype.")
            exit(1)

    if len(sys.argv) > 4:
        output_path = sys.argv[4]
    else:
        # Generate a clear output name like 'korean_command_classifier_fp16.rknn'
        output_path = model_path.replace('.onnx', '_fp16.rknn')

    return model_path, platform, output_path


if __name__ == '__main__':
    model_path, platform, output_path = parse_arg()

    # Create RKNN object
    rknn = RKNN(verbose=True)

    # --- Pre-process config for FP16 ---
    print('--> Configuring model for FP16 conversion')
    rknn.config(
        target_platform=platform,
        # This is the key parameter for FP16 conversion
        float_dtype='float16',
        mean_values=[], 
        std_values=[], 
        optimization_level=3
    )
    print('done')

    # --- Load ONNX model with the correct name and shape ---
    print('--> Loading classifier model')
    ret = rknn.load_onnx(
        model=model_path, 
        inputs=[CLASSIFIER_INPUT_NAME], 
        input_size_list=[CLASSIFIER_INPUT_SHAPE]
    )
    if ret != 0:
        print('Load model failed!')
        exit(ret)
    print('done')

    # --- Build model (do_quantization must be False for FP16) ---
    print('--> Building model in FP16 mode')
    ret = rknn.build(do_quantization=False)
    if ret != 0:
        print('Build model failed!')
        exit(ret)
    print('done')

    # --- Export the final .rknn model ---
    print('--> Exporting RKNN model')
    ret = rknn.export_rknn(output_path)
    if ret != 0:
        print('Export rknn model failed!')
        exit(ret)
    print(f'\nModel exported successfully to → {output_path}')
    
    # Release the context
    rknn.release()