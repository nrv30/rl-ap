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
        ClearBackground(RAYWHITE);

        // DrawRectangleRec(menubar, BLACK); // Menu Bar
        //GuiGroupBox((Rectangle) {SCREEN_WIDTH+SCREEN_WIDTH*.25f, 100, SCREEN_WIDTH*.075f, SCREEN_HEIGHT-100}, "Queue");

        if (state == START) {
            const char* text = "DROP MUSIC";
            DrawText(text, SCREEN_WIDTH/2 - MeasureText(text, 25)/2, SCREEN_HEIGHT/2 - 25, 25, SKYBLUE);
        } else {
            // TODO: Create a full-screen vs.playlist view state 
            // Full-Screen
            // --------------------------------------------------

            DrawTexture(play, SCREEN_WIDTH/2.0f-play.width, SCREEN_HEIGHT/2.0f-play.height, WHITE);
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
            
            // DrawRectangle(0, SCREEN_HEIGHT-100, SCREEN_WIDTH, 100, BLACK);
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