#include <stdio.h>
#include <assert.h>
#include "raylib.h"

int main(int argc, char *argv[]) {

    assert(argc > 1);

    InitWindow(800,600, "Music Visualizer");
    SetTargetFPS(60);

    InitAudioDevice();
    Music music = LoadMusicStream(argv[1]);
    PlayMusicStream(music);

    while (!WindowShouldClose()) {
        if (IsMusicStreamPlaying(music)) {
            UpdateMusicStream(music);
        }

        BeginDrawing();
        ClearBackground(BLACK);
        EndDrawing();

    }

    CloseAudioDevice();

    return 0;

}
