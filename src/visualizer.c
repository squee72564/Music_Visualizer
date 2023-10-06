#include <math.h>
#include <complex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "raylib.h"

#define FFT_SIZE 1024
#define N (1<<13)

typedef struct {
    char *file_path;
    Music music;
} Track;

typedef struct {
    float complex in_raw[N]; // Raw data from audio stream buffer
    float complex in_hann[N]; // Data with hann function applied
    float complex out_raw[N]; 
} FFT_Analyzer;

FFT_Analyzer *fft = NULL;

void fft_init() {
    fft = (FFT_Analyzer*)malloc(sizeof(FFT_Analyzer));
    memset(fft, 0, sizeof(*fft));
}

void fft_clean() {
    memset(fft->in_raw, 0, sizeof(fft->in_raw));
    memset(fft->in_hann, 0, sizeof(fft->in_hann));
}

void fft_process() {
    // First we want to apply the hann windowing
    // function to reduce spectral leakage
    apply_hann_function();

    // Perform fft
    
    // Perform Additional Processing
}

void fft_visualize() {
} 

void apply_hann_function() {
    for (size_t i = 0; i < N; i++) {
        float t = (float)i/(N-1);
        float hann = 0.5 - 0.5*cosf(2*PI*t);
        fft->in_hann[i] = fft->in_raw[i]*hann;
    }
}

void fft_callback(void *bufferData, unsigned int frames) {
    float (*fs)[2] = bufferData;
    for (size_t i = 0; i < frames; i++) {
        memmove(fft->in_raw, fft->in_raw + 1, (N-1)*sizeof(fft->in_raw[0]));
        fft->in_raw[N-1] = fs[i][0] + 0.0f*I ; // Left channel
    } 
}

int main(int argc, char *argv[]) {

    assert(argc > 1);

    InitWindow(800,600, "Music Visualizer");
    SetTargetFPS(60);
    
    fft_init();
    InitAudioDevice();
    
    Track current_track = (Track) {
        .file_path = argv[1],
        .music = LoadMusicStream(argv[1])
    };
    
    AttachAudioStreamProcessor(current_track.music.stream, fft_callback);

    PlayMusicStream(current_track.music);

    while (!WindowShouldClose()) {
        BeginDrawing();

        if (IsMusicStreamPlaying(current_track.music)) {
            UpdateMusicStream(current_track.music);
            fft_visualize();
        }

        

        ClearBackground(BLACK);
        EndDrawing();

        fft_clean();
    }

    CloseAudioDevice();

    return 0;

}
