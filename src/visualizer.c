#include <math.h>
#include <complex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "raylib.h"

#define N (1 << 14)
#define SB (1 << 11)

typedef struct
{
    char *file_path;
    Music music;
} Track;

typedef struct
{
    float complex in_rawL[N];  // Raw data from audio stream buffer
    float complex in_hannL[N]; // Data with hann function applied
    float complex out_rawL[N];
    float out_logL[N];

    float complex in_rawR[N];  // Raw data from audio stream buffer
    float complex in_hannR[N]; // Data with hann function applied
    float complex out_rawR[N];
    float out_logR[N];
} FFT_Analyzer;

typedef struct
{
    float complex right[SB];
    float complex left[SB];
} Audio_Buffer;

Audio_Buffer *aBuff = NULL;

FFT_Analyzer *fft = NULL;

void audioBuff_init()
{
    fft = (FFT_Analyzer *)malloc(sizeof(FFT_Analyzer));
    memset(fft, 0, sizeof(*fft));

    aBuff = (Audio_Buffer *)malloc(sizeof(Audio_Buffer));
    memset(aBuff, 0, sizeof(*aBuff));
}

void audioBuff_free()
{
    free(fft);
    free(aBuff);
}

void audioBuff_clean()
{
    memset(fft, 0, sizeof(*fft));
    memset(aBuff, 0, sizeof(*aBuff));
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

size_t fft_process()
{
    // Apply hann windowing function
    for (size_t i = 0; i < N; i++)
    {
        float t = (float)i / (N - 1);
        float hann = 0.5 - 0.5 * cosf(2 * PI * t);
        fft->in_hannL[i] = fft->in_rawL[i] * hann;
        fft->in_hannR[i] = fft->in_rawR[i] * hann;
    }

    memcpy(fft->out_rawL, fft->in_hannL, sizeof(fft->out_rawL));
    memcpy(fft->out_rawR, fft->in_hannR, sizeof(fft->out_rawR));

    // Perform FFT
    _fft(fft->out_rawL, fft->in_hannL, N, 1);
    _fft(fft->out_rawR, fft->in_hannR, N, 1);

    // Squash Frequencies
    // Provides for more resolution in lower frequency bins
    // step variable will also determine the resultion of the resulting fft for display; lower step
    // means higher resolution
    float step = 1.01f;
    float lowf = 1.0f;
    size_t s = 0;
    float max_ampL = 1.0f;
    float max_ampR = 1.0f;

    for (float f = lowf; (size_t)f < N / 2; f = ceil(f * step))
    {
        float f1 = ceil(f * step);
        int maxLi = 0;
        int maxRi = 0;
        float maxL = 0.0f;
        float maxR = 0.0f;
        for (size_t q = (size_t)f; q < N / 2 && q < (size_t)f1; q++)
        {
            float r = cabsf(fft->out_rawR[q]);
            float l = cabsf(fft->out_rawL[q]);
            if (r > maxR)
            {
                maxR = r;
                maxRi = q;
            }
            if (l > maxL)
            {
                maxL = l;
                maxLi = q;
            }
        }

        // Scale lagarithmically
        fft->out_logR[s++] = log10f(1.0f + cabsf(fft->out_rawL[maxRi]));
        fft->out_logL[s++] = log10f(1.0f + cabsf(fft->out_rawR[maxLi]));
    }

    // Get max and normalize
    for (size_t i = 0; i < s; i++)
    {
        float l = fft->out_logL[i];
        float r = fft->out_logR[i];
        if (l > max_ampL)
            max_ampL = l;
        if (r > max_ampR)
            max_ampR = r;
    }

    for (size_t i = 0; i < s; i++)
    {
        fft->out_logL[i] = fft->out_logL[i] / max_ampL;
        fft->out_logR[i] = fft->out_logR[i] / max_ampR;
    }

    return s;
}

void fft_callback(void *bufferData, unsigned int frames)
{
    float(*fs)[2] = bufferData; // L and R channels are the two floats

    for (size_t i = 0; i < frames; i++)
    {
        memmove(fft->in_rawL, fft->in_rawL + 1, (N - 1) * sizeof(fft->in_rawL[0]));
        fft->in_rawL[N - 1] = fs[i][0] + 0.0f * I; // Left channel for fft

        memmove(fft->in_rawR, fft->in_rawR + 1, (N - 1) * sizeof(fft->in_rawR[0]));
        fft->in_rawR[N - 1] = fs[i][1] + 0.0f * I; // Right channel for fft

        memmove(aBuff->left, aBuff->left + 1, (SB - 1) * sizeof(aBuff->left[0]));
        aBuff->left[SB - 1] = fs[i][0] + 0.0f * I; // Left channel

        memmove(aBuff->right, aBuff->right + 1, (SB - 1) * sizeof(aBuff->right[0]));
        aBuff->right[SB - 1] = fs[i][1] + 0.0f * I; // Right channel
    }
}

void fft_visualize(size_t frames)
{
    int w = GetRenderWidth();
    int h = GetRenderHeight() / 2;
    float d = (float)w / frames;
    float scale = 170.0f;

    Vector2 ptsL[N / 2] = {0};
    Vector2 ptsR[N / 2] = {0};

    for (size_t i = 0; i < frames; i++)
    {
        ptsL[i] = (Vector2) {
            .x = i * d,
            .y = ((float)h / 2) + fft->out_logL[i] * h/2
        };

        ptsR[i] = (Vector2) {
            .x = i * d,
            .y = ((float)h / 2) - fft->out_logR[i] * h/2
        };
    }

    Color c1 = (Color){100, 0, 255, 65};
    Color cR = (Color){255, 0, 100, 65};

    DrawLineStrip(ptsL, frames, c1);
    DrawLineStrip(ptsR, frames, cR);
}

void drawWave()
{
    int w = GetRenderWidth();
    int h = GetRenderHeight() / 2;
    float rectw = (float)w / SB;
    int scale = 300;

    Color c2 = (Color){0, 0, 255, 0};
    Color c3 = (Color){200, 0, 55, 165};

    if (rectw < 1.0f)
    {
        rectw = 1.0f;
    }

    float max = 0.0f;

    for (int i = 0; i < SB; i++)
    {
        if (cabsf(aBuff->left[i]) > max)
            max = cabsf(aBuff->left[i]);
        if (cabsf(aBuff->right[i]) > max)
            max = cabsf(aBuff->right[i]);
    }

    for (size_t i = 0; i < SB; i++)
    {
        Rectangle rec = (Rectangle) {
            .x = i * rectw,
            .y = h + h / 2 - 1,
            .width = rectw,
            .height = h/2 * (cabsf(aBuff->left[i]) / max)
        };
        
        Rectangle rec2 = (Rectangle) {
            .x = i * rectw,
            .y = h + h / 2,
            .width = rectw,
            .height = h/2 * (cabsf(aBuff->right[i]) / max)
        };

        DrawRectangleGradientV(rec.x, rec.y, rec.width, rec.height, c3, c2);
        DrawRectangleGradientV(rec2.x, rec2.y - rec2.height, rec2.width, rec2.height, c2, c3);
    }
}

int main(int argc, char *argv[])
{
    InitWindow(1024, 900, "Music Visualizer");
    SetTargetFPS(60);
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(800,600);

    audioBuff_init();
    InitAudioDevice();

    Track current_track;

    if ( argc > 1) {
        Track current_track = (Track){
            .file_path = argv[1],
            .music = LoadMusicStream(argv[1])
        };

        AttachAudioStreamProcessor(current_track.music.stream, fft_callback);

        PlayMusicStream(current_track.music);
    }

    bool showWave = true;
    bool showFFT = true;
    bool isPaused = false;


    while (!WindowShouldClose())
    {
        // Handle Key Press
        int key = GetKeyPressed();
        switch (key)
        {
        case KEY_Q:
            showWave = !showWave;
            break;
        case KEY_W:
            showFFT = !showFFT;
            break;
        case KEY_A:
            SeekMusicStream(current_track.music, (GetMusicTimePlayed(current_track.music) - 5.0f));
            break;
        case KEY_S:
            SeekMusicStream(current_track.music, (GetMusicTimePlayed(current_track.music) + 60.0f));
            break;
        case KEY_P:
            (isPaused) ? ResumeMusicStream(current_track.music) : PauseMusicStream(current_track.music);
            isPaused = !isPaused;
            break;
        default:
            break;
        }

        // Handle File Drop
        if (IsFileDropped()) {
            FilePathList fl = LoadDroppedFiles();
            printf("INFO: FILES DROPPED:\n");
            for (size_t i = 0; i < fl.count; i++) {
                char * p = fl.paths[i];
                printf("\t%s\n", p);
            }

            StopMusicStream(current_track.music);
            UnloadMusicStream(current_track.music);
            audioBuff_clean();

            current_track = (Track) {
                .file_path = fl.paths[0],
                .music = LoadMusicStream(fl.paths[0])
            };

            AttachAudioStreamProcessor(current_track.music.stream, fft_callback);
            PlayMusicStream(current_track.music);

            UnloadDroppedFiles(fl);
        } 

        BeginDrawing();

        ClearBackground(BLACK);

        if (IsMusicStreamPlaying(current_track.music))
            UpdateMusicStream(current_track.music);

        if (showWave)
            drawWave();

        if (showFFT)
        {
            size_t frames = fft_process();
            fft_visualize(frames);
        }

        EndDrawing();
    }

    CloseAudioDevice();
    audioBuff_free();

    return 0;
}
