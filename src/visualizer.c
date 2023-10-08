#include <math.h>
#include <complex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "raylib.h"

#define N (1<<13)
#define SB (1<<12)

typedef struct
{
    char *file_path;
    Music music;
} Track;

typedef struct
{
    float complex in_raw[N];  // Raw data from audio stream buffer
    float complex in_hann[N]; // Data with hann function applied
    float complex out_raw[N];
} FFT_Analyzer;

typedef struct
{
    float right[SB];
    float left[SB];
} Audio_Buffer;

Audio_Buffer *aBuff = NULL;

FFT_Analyzer *fft = NULL;

void fft_init()
{
    fft = (FFT_Analyzer *)malloc(sizeof(FFT_Analyzer));
    memset(fft, 0, sizeof(*fft));

    // init other buffers for now
    aBuff = (Audio_Buffer *)malloc(sizeof(Audio_Buffer));
    memset(aBuff, 0, sizeof(*aBuff));
}

void fft_clean()
{
    memset(fft->in_raw, 0, sizeof(fft->in_raw));
    memset(fft->in_hann, 0, sizeof(fft->in_hann));
}

void _fft(float complex in[], float complex out[], int n, int step)
{
    if (step < n)
    {
        _fft(out, in, n, step * 2);
        _fft(out + step, in + step, n, step * 2);

        for (int i = 0; i < n; i += 2 * step)
        {
            float complex t = cexp(-I * PI * i / n) * out[i + step];
            in[i / 2] = out[i] + t;
            in[(i + n) / 2] = out[i] - t;
        }
    }
}

void apply_hann_function()
{
    for (size_t i = 0; i < N; i++)
    {
        float t = (float)i / (N - 1);
        float hann = 0.5 - 0.5 * cosf(2 * PI * t);
        fft->in_hann[i] = fft->in_raw[i] * hann;
    }
}

void fft_process()
{
    apply_hann_function();

    memcpy(fft->out_raw, fft->in_hann, sizeof(fft->out_raw));
    _fft(fft->out_raw, fft->in_hann, N, 1);

    // TODO: Additional Processing on fft wave to make it look nicer
}

void fft_callback(void *bufferData, unsigned int frames)
{
    float(*fs)[2] = bufferData; // L and R channels are the two floats

    for (size_t i = 0; i < frames; i++)
    {
        memmove(fft->in_raw, fft->in_raw + 1, (N - 1) * sizeof(fft->in_raw[0]));
        fft->in_raw[N - 1] = fs[i][0] + 0.0f * I; // Left channel for fft

        memmove(aBuff->left, aBuff->left + 1, (SB - 1) * sizeof(aBuff->left[0]));
        aBuff->left[SB - 1] = fs[i][0]; // Left channel

        memmove(aBuff->right, aBuff->right + 1, (SB - 1) * sizeof(aBuff->right[0]));
        aBuff->right[SB - 1] = fs[i][1]; // Right channel
    }
}

void fft_visualize() {


    // TODO: Scale x and y axis to better show waveform
    
    int w = GetRenderWidth();
    int h = GetRenderHeight();
    float d = (float)w / (N/2);
    float scale = 6.0f;

    Vector2 pts[N / 2] = {0};

    for (size_t i = 0; i < N/2; i++)
    {
        Color c1 = (Color){155, 100, 0, cabsf(fft->out_raw[i]) * 50};

        pts[i] = (Vector2){i * d, h / 2 + fft->out_raw[i] * scale};
        DrawCircleV(pts[i], 1.2f, c1);
    }
    Color c1 = (Color){246, 255, 32, 65};
    DrawLineStrip(&pts[0], N/2, c1);
}

void drawWave() {
    int w = GetRenderWidth();
    int h = GetRenderHeight();
    float rectw = (float)w / SB;
    int scale = 980;

    Color c2 = (Color){0, 0, 255, 0};
    Color c3 = (Color){200, 0, 55, 185};
    
    if (rectw < 1.0f)
    {
        rectw = 1.0f;
    }

    Rectangle rec;
    Rectangle rec2;

    for (size_t i = 0; i < SB; i++)
    {
        rec = (Rectangle){.x = i * rectw, .y = h / 2 - 1, .width = rectw, .height = scale*aBuff->left[i]};
        rec2 = (Rectangle){.x = i * rectw, .y = h / 2, .width = rectw, .height = scale*aBuff->right[i]};

        DrawRectangleGradientV(rec.x, rec.y, rec.width, rec.height, c3, c2);
        DrawRectangleGradientV(rec2.x, rec2.y - rec2.height, rec2.width, rec2.height, c2, c3);
    }
    
}

int main(int argc, char *argv[])
{

    assert(argc > 1);

    InitWindow(1024, 900, "Music Visualizer");
    SetTargetFPS(60);

    fft_init();
    InitAudioDevice();

    Track current_track = (Track){
        .file_path = argv[1],
        .music = LoadMusicStream(argv[1])};

    bool showWave = true;
    bool showFFT = true;

    AttachAudioStreamProcessor(current_track.music.stream, fft_callback);

    PlayMusicStream(current_track.music);

    SeekMusicStream(current_track.music, 40.0f + (GetMusicTimeLength(current_track.music) / 2.0f));

    while (!WindowShouldClose())
    {

        if (IsKeyPressed(KEY_Q))
            showWave = !showWave;
        if (IsKeyPressed(KEY_W))
            showFFT = !showFFT;

        BeginDrawing();
        ClearBackground(BLACK);

        if (IsMusicStreamPlaying(current_track.music))
        {
            UpdateMusicStream(current_track.music);

            if (showWave)
                drawWave();

            if (showFFT)
            {
                fft_process();
                fft_visualize();
            }
        }

        EndDrawing();
    }

    CloseAudioDevice();

    return 0;
}
