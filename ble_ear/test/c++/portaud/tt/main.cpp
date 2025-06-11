#include <stdlib.h>
#include <stdio.h>
#include <cstring>

#include <portaudio.h>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512

static void checkErr(PaError err) {
    if (err != paNoError) {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
        exit(EXIT_FAILURE);
    }
}

static inline float max(float a, float b) {
    return a > b ? a : b;
}

static int patestCallback(
    const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags,
    void* userData
) {
    float* in = (float*)inputBuffer;
    (void)outputBuffer;

    int dispSize = 100;
    printf("\r");

    float vol_l = 0;
    float vol_r = 0;

    for (unsigned long i = 0; i < framesPerBuffer * 2; i += 2) {
        vol_l = max(vol_l, std::abs(in[i]));
        vol_r = max(vol_r, std::abs(in[i+1]));
    }

    for (int i = 0; i < dispSize; i++) {
        float barProportion = i / (float)dispSize;
        if (barProportion <= vol_l && barProportion <= vol_r) {
            printf("█");
        } else if (barProportion <= vol_l) {
            printf("▀");
        } else if (barProportion <= vol_r) {
            printf("▄");
        } else {
            printf(" ");
        }
    }

    fflush(stdout);

    return 0;
}

int main() {
    PaError err;
    err = Pa_Initialize();
    checkErr(err);

    // [Previous device listing code remains the same...]

    int device = 0;
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(device);

    // Verify device supports input
    if (deviceInfo->maxInputChannels < 2) {
        printf("Device %d doesn't support stereo input!\n", device);
        Pa_Terminate();
        return EXIT_FAILURE;
    }

    PaStreamParameters inputParameters;
    memset(&inputParameters, 0, sizeof(inputParameters));
    inputParameters.channelCount = 2;
    inputParameters.device = device;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    // Open stream with ONLY input parameters (no output)
    PaStream* stream;
    err = Pa_OpenStream(
        &stream,
        &inputParameters,  // Input only
        NULL,              // No output
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paNoFlag,
        patestCallback,
        NULL
    );
    checkErr(err);

    err = Pa_StartStream(stream);
    checkErr(err);

    Pa_Sleep(10 * 1000);

    err = Pa_StopStream(stream);
    checkErr(err);

    err = Pa_CloseStream(stream);
    checkErr(err);

    err = Pa_Terminate();
    checkErr(err);

    return EXIT_SUCCESS;
}
