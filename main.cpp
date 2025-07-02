#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <iostream>
#include <list>
#include <vector>
#include <string.h>
#include <iterator>

float SCREEN_WIDTH = 1920;
float SCREEN_HEIGHT = 1280;

typedef enum {
    START,
    PLAYING,
} STATE;
STATE state = START;

void loadSongs(FilePathList droppedFiles, std::vector<std::string>* titles, std::list<Music>*songs);


int main(void) {
    SetTraceLogLevel(LOG_DEBUG);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Audio Player");
    InitAudioDevice();
    SetMasterVolume(.5);

    std::vector<std::string> titles;
    std::list<Music> songs;

    std::list<Music>::iterator it_song;
    std::vector<std::string>::iterator it_path;

    bool songStarted, isPause, autoplay;
    songStarted = isPause = false;

    const int vertpad = 100;
    Rectangle menubar {0, 0, SCREEN_WIDTH, vertpad};
    Rectangle panelRec = {0, menubar.height, SCREEN_WIDTH*.25f, SCREEN_HEIGHT-vertpad*2};
    Rectangle panelContentRec = {0, 0, 340, 340 };
    Rectangle panelView = { 0 };
    Vector2 panelScroll = { 99, -20 };


    SetTargetFPS(60);
    while(!WindowShouldClose()) {

        if (IsFileDropped()) {   
            FilePathList droppedFiles = LoadDroppedFiles();
            loadSongs(droppedFiles, &titles, &songs);  
            UnloadDroppedFiles(droppedFiles);
            if (state == START && songs.size() > 0) {
                it_song = songs.begin();
                it_path = titles.begin();
                state = PLAYING;
            }
        }      

        if (state == PLAYING) {
            if (!songStarted) {
                PlayMusicStream(*it_song);
                songStarted = true;
            } else {
                if (IsKeyPressed(KEY_RIGHT) && it_song != songs.end()) {
                    StopMusicStream(*it_song);
                    it_song++; 
                    songStarted = false;
                } else if (IsKeyPressed(KEY_LEFT) && it_song != songs.begin()) {
                    StopMusicStream(*it_song);
                    it_song--; 
                    songStarted = false;
                } else if (IsKeyPressed(KEY_SPACE)) {
                    if (isPause) {
                        ResumeMusicStream(*it_song);
                    }
                    else {
                        PauseMusicStream(*it_song);
                    }
                    isPause = !isPause;
                }
                UpdateMusicStream(*it_song);
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawRectangleRec(menubar, BLACK); // Menu Bar
        //GuiGroupBox((Rectangle) {SCREEN_WIDTH+SCREEN_WIDTH*.25f, 100, SCREEN_WIDTH*.075f, SCREEN_HEIGHT-100}, "Queue");

        if (state == START) {
            const char* text = "DROP MUSIC";
            DrawText(text, SCREEN_WIDTH/2 - MeasureText(text, 25)/2, SCREEN_HEIGHT/2 - 25, 25, SKYBLUE);
        } else {
            // const char* text = (*it_path).c_str();
            // TraceLog(LOG_DEBUG, TextFormat("%s", text));
            // DrawText(text, SCREEN_WIDTH/2 - MeasureText(text, 10)/2, SCREEN_HEIGHT/2 - 25, 10, SKYBLUE);

            // auto it = std::next(it_song);
            // const float songw = SCREEN_WIDTH/6.0f;
            // const float songh = SCREEN_HEIGHT/10.0f;
            // Rectangle panelContentRec = {0, 0, songw, songh+10};

            // const int pad = 10;

            // for (float y = 0; it != songs.end(); ++it, y+=pad) {
            //     DrawRectangle(0, y, songw, songh,BLACK);
            //     y+=songh;
            //     DrawRectangle(0, y, songw, (float)pad, GRAY);
            // }

            GuiScrollPanel(panelRec, NULL, panelContentRec, &panelScroll, &panelView);

            BeginScissorMode(panelView.x, panelView.y, panelView.width, panelView.height);
                GuiGrid((Rectangle){panelRec.x + panelScroll.x, panelRec.y + panelScroll.y, panelContentRec.width, panelContentRec.height}, NULL, 16, 3, NULL);
            EndScissorMode();            
        }
        EndDrawing();
    }
    for (auto& s : songs) {
        UnloadMusicStream(s);
    }
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

void loadSongs(FilePathList droppedFiles, std::vector<std::string>* titles, std::list<Music>* songs) {
    for (int i = 0; i < (int)droppedFiles.count; i++)
    {
        if (IsPathFile(droppedFiles.paths[i])) {
            Music song = LoadMusicStream(droppedFiles.paths[i]);
            if (IsMusicValid(song)) {

                (*titles).emplace_back(droppedFiles.paths[i]);
                song.looping = false;
                (*songs).emplace_back(song);
            } else {
                UnloadMusicStream(song);
            }
        } else {
            FilePathList dir = LoadDirectoryFiles(droppedFiles.paths[i]);
            loadSongs(dir, titles, songs);
            UnloadDirectoryFiles(dir);
        }
    }
}