import numpy as np
import onnxruntime
import librosa
from rknn.api import RKNN

# --- CONFIG ---
ONNX_PATH = "/home/limonubuntu/Work/Limon/other_task/ble_project/training/models/06_22/rknn/test_korean_small/models/k3_150epoch.onnx"
RKNN_PATH = "/home/limonubuntu/Work/Limon/other_task/ble_project/training/models/06_22/rknn/test_korean_small/models/korean_command_classifier.rknn"
QUANT_DATASET = 'embedding_quant_list.txt' # Use the .npy files
TARGET_PLATFORM = 'rk3588'

if __name__ == '__main__':
    rknn = RKNN(verbose=True)

    # Config
    rknn.config(target_platform=TARGET_PLATFORM)

    # Load ONNX - this model already has a fixed input shape
    print('--> Loading classifier model')
    ret = rknn.load_onnx(model=ONNX_PATH)
    if ret != 0:
        print('Load failed!')
        exit(ret)
    print('done')

    # Build with quantization using the generated embeddings
    print('--> Building classifier model')
    ret = rknn.build(do_quantization=True, dataset=QUANT_DATASET)
    if ret != 0:
        print('Build failed!')
        exit(ret)
    print('done')

    # Export
    rknn.export_rknn(RKNN_PATH)
    rknn.release()