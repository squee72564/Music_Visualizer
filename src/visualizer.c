#include <math.h>
#include <complex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "raylib.h"

#define N (1 << 14)
#define SB (1 << 10)

typedef struct
{
    char *file_path;
} Track;

typedef struct
{
    size_t count;
    int currIdx;
    Music current;
    Track tracks[100];
} TrackList;

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

TrackList *tl = NULL;

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

bool isExtensionValid(const char *s)
{
    
    bool v = (strcmp(s, ".wav") == 0 || strcmp(s, ".ogg") == 0 || strcmp(s, ".mp3") == 0) ;
    return v;
}

void tracklist_init()
{
    tl = (TrackList*)malloc(sizeof(TrackList));
    memset(tl, 0, sizeof(TrackList));
}

void tracklist_add(char* s)
{
    tl->tracks[tl->count].file_path = strdup(s);
    tl->count += 1;
}

void tracklist_free()
{
    size_t n = tl->count;
    for (size_t i = 0; i < n; i++)
    {
        free(tl->tracks[i].file_path);
    }
}

void audioBuff_clean()
{
    memset(fft, 0, sizeof(*fft));
    memset(aBuff, 0, sizeof(*aBuff));
}

void tracklist_play(int i)
{
    tl->currIdx = i;
    if (tl->currIdx > (int)tl->count-1) tl->currIdx = 0;
    if (tl->currIdx < 0) tl->currIdx = tl->count-1;

    StopMusicStream(tl->current);
    UnloadMusicStream(tl->current);
    
    tl->current = LoadMusicStream(tl->tracks[tl->currIdx].file_path);

    audioBuff_clean();
    
    AttachAudioStreamProcessor(tl->current.stream, fft_callback);
    PlayMusicStream(tl->current);
}

void audioBuff_init()
{
    fft = (FFT_Analyzer *)malloc(sizeof(FFT_Analyzer));
    memset(fft, 0, sizeof(FFT_Analyzer));

    aBuff = (Audio_Buffer *)malloc(sizeof(Audio_Buffer));
    memset(aBuff, 0, sizeof(Audio_Buffer));
}

void audioBuff_free()
{
    free(fft);
    free(aBuff);
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
        float f1 = ceil(f * step);  // Next freq
        int maxLi = 0;              // Max Left idx
        int maxRi = 0;              // Max Right idx
        float maxL = 0.0f;          // Max Left amp
        float maxR = 0.0f;          // Max Right amp

        size_t q = (size_t)f;
        while (q < N / 2 && q < (size_t)f1)
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
            q++; 
        }

        // Scale logarithmically
        fft->out_logR[s] = log10f(1.0f + cabsf(fft->out_rawL[maxRi]));
        fft->out_logL[s] = log10f(1.0f + cabsf(fft->out_rawR[maxLi]));
        s++;
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

void fft_visualize(size_t frames, int w, int h)
{
    float d = (float)w / frames;

    Vector2 ptsL_end[N / 2] = {0};
    Vector2 ptsL_start[N / 2] = {0};
    Vector2 ptsR_end[N / 2] = {0};
    Vector2 ptsR_start[N / 2] = {0};

    Color cL = (Color){100, 0, 255, 255};
    Color cR = (Color){255, 0, 100, 255};


    for (size_t i = 0; i < frames; i++)
    {
        ptsL_end[i] = (Vector2) {
            .x = i * d,
            .y = ((float)h / 2) + fft->out_logL[i] * h/2
        };

        ptsL_start[i] = (Vector2) {
            .x = i * d,
            .y = (float)h / 2
        };

        ptsR_end[i] = (Vector2) {
            .x = i * d,
            .y = ((float)h / 2) - fft->out_logR[i] * h/2
        };

        ptsR_start[i] = (Vector2) {
            .x = i * d,
            .y = (float)h / 2
        };

        // Draw Bars with alpha values based on amplituded / display height
        Color cL2 = (Color){
            100,
            0,
            255,
            ((ptsL_end[i].y - ptsL_start[i].y) / (h/2))*255
        };
        
        Color cR2 = (Color){
            255,
            0,
            100,
            ((ptsR_start[i].y - ptsR_end[i].y) / (h/2))*255
        };

        DrawLineEx(ptsR_start[i], ptsR_end[i], d, cR2);
        DrawLineEx(ptsL_start[i], ptsL_end[i], d, cL2);

    }

    DrawLineStrip(ptsL_end, frames, cL);
    DrawLineStrip(ptsR_end, frames, cR);
}

void fft_visualize2(size_t frames, int w, int h)
{
    frames -= 250;
    float radius = h/2;

    Vector2 ptsL_end[N/2] = {0};

    for (size_t i = 0; i < frames; i++)
    {
        float angle = (2.1 * PI * i) / frames;
        
        Vector2 v = (Vector2) {
            .x = w/2 + radius *  fft->out_logL[i] * cosf(angle),
            .y = h/2 + radius *  fft->out_logL[i] * sinf(angle)
        };        

        ptsL_end[i] = v;
    }

    DrawLineStrip(ptsL_end, frames, RED);
}

void drawWave(int w, int h)
{
    float rectw = (float)w / SB;

    Color c2 = (Color){0, 0, 255, 0};
    Color c3 = (Color){200, 0, 55, 235};

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

int main()
{
    InitWindow(1024, 900, "Music Visualizer");
    SetTargetFPS(60);
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(1024,900);

    audioBuff_init();
    tracklist_init();
    InitAudioDevice();

    size_t vis = 0;
    bool showWave = true;
    bool showFFT = true;
    bool showFFT2 = false;

    bool isPaused = true;
    bool isMusicLoaded = false;

    while (!WindowShouldClose())
    {
        int w = GetRenderWidth();
        int h = GetRenderHeight();

        // Handle Key Press
        int key = GetKeyPressed();
        switch (key)
        {
            case KEY_Q:
                vis = (vis+1) % 4;

                switch(vis)
                {
                    case 0:
                        showWave = true;
                        showFFT = true;
                        showFFT2 = false;
                        break;
                    case 1:
                        showFFT = true;
                        showWave = false;
                        break;
                    case 2:
                        showFFT = false;
                        showWave = true;
                        break;
                    case 3:
                        showWave = false;
                        showFFT2 = true;
                        break;
                    default:
                        break;
                }

                break;
            case KEY_A:
                if (tl->count > 0)
                    SeekMusicStream(tl->current, (GetMusicTimePlayed(tl->current) - 5.0f));
                break;
            case KEY_S:
                if (tl->count > 0)
                    SeekMusicStream(tl->current, (GetMusicTimePlayed(tl->current) + 60.0f));
                break;
            case KEY_P:
                (isPaused) ? ResumeMusicStream(tl->current) : PauseMusicStream(tl->current);
                isPaused = !isPaused;
                break;
            case KEY_X:
                if (tl->count > 0)
                    tracklist_play(tl->currIdx+1);
                break;
            case KEY_Z:
                if (tl->count > 0)
                    tracklist_play(tl->currIdx-1);
                break;
            default:
                break;
        }

        // Handle File Drop
        if (IsFileDropped()) {
            FilePathList fl = LoadDroppedFiles();
            printf("INFO: %d FILES DROPPED:\n", fl.count);
            size_t success = 0;
            for (size_t i = 0; i < fl.count; i++) {
                char *path = fl.paths[i];

                printf("INFO:\t  > %s\n", path);
                
                if (isExtensionValid(GetFileExtension(path))) {
                    tracklist_add(path);
                    isMusicLoaded = true;
                    success++;
                } else {
                    printf("INFO: Invalid file extension %s\n", GetFileExtension(fl.paths[i]));
                }

            }
            UnloadDroppedFiles(fl);
            if (success >= 1 && !IsMusicStreamPlaying(tl->current)) {
                isPaused = false;
                ResumeMusicStream(tl->current);
                tracklist_play(tl->count-1);  
            }
        }

        BeginDrawing();

        ClearBackground(BLACK);

        if (isMusicLoaded && IsMusicStreamPlaying(tl->current)) {
            UpdateMusicStream(tl->current);
        }

        if (showWave)
            drawWave(w, h/2);

        size_t frames = 0;
        if (showFFT || showFFT2)
            frames = fft_process();

        if (showFFT)
        {
            fft_visualize(frames, w, h/2);
        }

        if (showFFT2)
        {
            fft_visualize2(frames, w, h);
        }

        if (isMusicLoaded) {
            Font font = GetFontDefault();
            int fontSize = 24;
            int spacing = 1;
            const char *fileName = GetFileNameWithoutExt(tl->tracks[tl->currIdx].file_path);
            const char *fileExt = GetFileExtension(tl->tracks[tl->currIdx].file_path);
            
            Vector2 fileNameV = MeasureTextEx(font, fileName, fontSize, spacing);
            Vector2 fileExtV = MeasureTextEx(font, fileExt, fontSize, spacing);

            DrawTextEx(
                font,
                fileName,
                (Vector2){GetRenderWidth()/2-fileNameV.x/2, GetRenderHeight()/2},
                fontSize,
                spacing,
                WHITE
            );
            DrawTextEx(
                font,
                fileExt,
                (Vector2){GetRenderWidth()/2-fileExtV.x/2, GetRenderHeight()/2+fileNameV.y},
                fontSize,
                spacing,
                WHITE
            );
        }

        EndDrawing();
    }

    CloseAudioDevice();
    audioBuff_free();
    tracklist_free();
        
    return 0;
}
