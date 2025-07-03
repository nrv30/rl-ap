#include "raylib.h"

#include <iostream>
#include <list>
#include <vector>
#include <string.h>
#include <iterator>

float SCREEN_WIDTH = 600;
float SCREEN_HEIGHT = 800;

typedef enum {
    START,
    PLAYING,
} STATE;
STATE state = START;

void loadSongs(FilePathList droppedFiles, std::vector<std::string>* titles, std::list<Music>*songs);

void parseNameFromPath(std::string* src);



int main(void) {
    SetTraceLogLevel(LOG_DEBUG);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Audio Player");
    InitAudioDevice();
    SetMasterVolume(.5);

    // Data structures
    std::vector<std::string> titles;
    std::list<Music> songs;

    std::list<Music>::iterator it_song;
    std::vector<std::string>::iterator it_path;

    // Flags
    bool songStarted, isPause, autoplay;
    songStarted = isPause = false;

    // TODO: Embedd in the exe
    // Assets
    Texture play = LoadTexture("../../assets/black-icon/Music-On.png");

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
            for (auto& s : titles) {
                std::cout << s << '\n';
            }
        }      

        if (state == PLAYING) {
            if (!songStarted) {
                PlayMusicStream(*it_song);
                songStarted = true;
            } else {
                if (IsKeyPressed(KEY_RIGHT) && it_song != std::prev(songs.end())) {
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
        ClearBackground(DARKGRAY);

        // DrawRectangleRec(menubar, BLACK); // Menu Bar
        //GuiGroupBox((Rectangle) {SCREEN_WIDTH+SCREEN_WIDTH*.25f, 100, SCREEN_WIDTH*.075f, SCREEN_HEIGHT-100}, "Queue");

        if (state == START) {
            const char* text = "DROP MUSIC";
            DrawText(text, SCREEN_WIDTH/2 - MeasureText(text, 25)/2, SCREEN_HEIGHT/2 - 25, 25, SKYBLUE);
        } else {
            // TODO: Create a full-screen vs.playlist view state 
            // Full-Screen
            // --------------------------------------------------
            // Playlist
            // DO the button is psd and export as png
            // Border?


            float borderpad = SCREEN_WIDTH/10.0f;
            
            Rectangle content = {
                .x = borderpad,
                .y = borderpad,
                .width = SCREEN_WIDTH-borderpad*2.0f,
                .height = SCREEN_HEIGHT-borderpad*2.0f,
            };

            // TraceLog(LOG_DEBUG, TextFormat("conw: %f conh: %f", content.width, content.height));

            float songpad = (content.height/5.0f)/10.0f;
            auto it = it_song;
            const float songw = content.width - songpad*2.0f;
            const float songh = content.height/5.0f - songpad*2.0f;
            // TraceLog(LOG_DEBUG, TextFormat("songw: %f songh: %f", songw, songh));
            DrawRectangleLines(content.x, content.y, content.width, content.height, BLACK);

            for (float y = content.y; it != songs.end(); ++it, y+=songpad+songh) {
                DrawRectangleRounded((Rectangle) {content.x + songpad, y, songw, songh}, .3f, 10, SKYBLUE);

            }    
            
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
                parseNameFromPath((&(*titles)[titles->size()-1]));
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

// has to be legit music file
void parseNameFromPath(std::string* src) {
    std::string temp;
    std::string::reverse_iterator it_s = (*src).rbegin();
    it_s--;
    bool before = true;
    while(*it_s != '\\' && *it_s != '/') {

        if (!before) {
            temp.insert(0, std::string(1, *it_s));
        } else if (*it_s == '.') {
            before = !before;
        }
        
        it_s++;
    }
    *src = temp;
}