# convert_feature_extractor_final.py (or export_feature_extractor_rknn.py)
import sys
from rknn.api import RKNN

# --- CONFIGURATION ---
INPUT_NAME = 'input_values' 
FIXED_SHAPE = [1, 16000 * 4] # Shape of raw audio input: [1, 64000]

# --- THE FIX: Make sure this path is correct! ---
# This MUST be the text file listing the pre-processed RAW AUDIO .npy files.
# The default name from the previous step was 'audio_quant_npy_list.txt'.
QUANT_DATASET_PATH = 'audio_quant_npy_list.txt' 

def parse_arg():
    # ... (this function is correct) ...
    if len(sys.argv) < 3:
        print(f"Usage: python3 {sys.argv[0]} onnx_model_path [platform] [dtype(optional)] [output_rknn_path(optional)]")
        exit(1)
    model_path = sys.argv[1]
    platform = sys.argv[2]
    do_quant = True
    if len(sys.argv) > 3 and sys.argv[3] == 'fp':
        do_quant = False
    if len(sys.argv) > 4:
        output_path = sys.argv[4]
    else:
        output_path = model_path.replace('.onnx', '.rknn')
    return model_path, platform, do_quant, output_path

if __name__ == '__main__':
    model_path, platform, do_quant, output_path = parse_arg()

    rknn = RKNN(verbose=True)

    print('--> Config model')
    rknn.config(
        target_platform=platform,
        mean_values=[], 
        std_values=[],
        optimization_level=3
    )
    print('done')

    print('--> Loading feature extractor model')
    ret = rknn.load_onnx(
        model=model_path,
        inputs=[INPUT_NAME],
        input_size_list=[FIXED_SHAPE]
    )
    if ret != 0:
        print('Load model failed!')
        exit(ret)
    print('done')

    print('--> Building model')
    # This dataset parameter now correctly points to a list of .npy files,
    # each containing a numpy array of shape [1, 64000].
    ret = rknn.build(do_quantization=do_quant, dataset=QUANT_DATASET_PATH if do_quant else None)
    if ret != 0:
        print('Build model failed!')
        exit(ret)
    print('done')

    print('--> Exporting RKNN')
    ret = rknn.export_rknn(output_path)
    if ret != 0:
        print('Export rknn model failed!')
        exit(ret)
    print(f'Model exported → {output_path}')
    rknn.release()