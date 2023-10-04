#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "raylib.h"

#define FFT_SIZE 1024

typedef struct {
    char *file_path;
    Music music;
} Track;

int main(int argc, char *argv[]) {

    assert(argc > 1);

    InitWindow(800,600, "Music Visualizer");
    SetTargetFPS(60);

    InitAudioDevice();
    
    Track current_track = (Track) {
        .file_path = argv[1],
        .music = LoadMusicStream(argv[1])
    };

    PlayMusicStream(current_track.music);

    float *audioData = (float *)malloc(sizeof(float)*FFT_SIZE);

    while (!WindowShouldClose()) {
        if (IsMusicStreamPlaying(current_track.music)) {
            UpdateMusicStream(current_track.music);
        }

        BeginDrawing();
        ClearBackground(BLACK);
        EndDrawing();

    }

    CloseAudioDevice();

    return 0;

}
