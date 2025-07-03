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
    std::vector<std::string>::iterator it_titles;

    // Flags
    bool songStarted, isPause, autoplay;
    songStarted = isPause = false;

    // TODO: Embedd in the exe
    // Assets
    //Texture play = LoadTexture("../../assets/black-icon/Music-On.png");
    Image drop = LoadImage("../../assets/black-icon/Home.png");
    ImageResize(&drop, drop.width*2, drop.height*2);
    ImageRotate(&drop, 180);
    Texture drop_text = LoadTextureFromImage(drop);

    Texture play = LoadTexture("../../assets/black-icon/Play.png");
    Texture pause = LoadTexture("../../assets/black-icon/Pause.png");
    Texture left_skip = LoadTexture("../../assets/black-icon/SolidArrow_Left.png");
    Texture right_skip = LoadTexture("../../assets/black-icon/SolidArrow_Right.png");

    SetTargetFPS(60);
    while(!WindowShouldClose()) {

        if (IsFileDropped()) {   
            FilePathList droppedFiles = LoadDroppedFiles();
            loadSongs(droppedFiles, &titles, &songs);  
            UnloadDroppedFiles(droppedFiles);
            if (state == START && songs.size() > 0) {
                it_song = songs.begin();
                it_titles = titles.begin();
                state = PLAYING;
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
                    it_titles++;
                    songStarted = false;
                } else if (IsKeyPressed(KEY_LEFT) && it_song != songs.begin()) {
                    StopMusicStream(*it_song);
                    it_song--; 
                    it_titles--;
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
            const int fontsize = 50;
            DrawText(text, SCREEN_WIDTH/2.0f - MeasureText(text, fontsize)/2, SCREEN_HEIGHT/3.0f - fontsize, fontsize, SKYBLUE);
            DrawTexture(drop_text, SCREEN_WIDTH/2.0f-drop.width/2.0f, SCREEN_HEIGHT/2.0f, WHITE);
        } else {
            // TODO: Create a full-screen vs.playlist view state 
            // Full-Screen
            // --------------------------------------------------
            // Playlist
            // DO the button is psd and export as png
            // Border?


            float borderpad = SCREEN_WIDTH/10.0f;
            const float control_panel = SCREEN_HEIGHT/5.0f;
            
            Rectangle content = {
                .x = borderpad,
                .y = borderpad,
                .width = SCREEN_WIDTH-borderpad*2.0f,
                .height = SCREEN_HEIGHT-control_panel-borderpad*2,
            };

            const float songpad = (content.height/5.0f)/10.0f;
            const float songw = content.width - songpad*2.0f;
            const float songh = content.height/5.0f - songpad*2.0f;
            Color color = SKYBLUE;
            DrawRectangleLines(content.x, content.y, content.width, content.height, BLACK);

            BeginScissorMode(content.x, content.y, content.width, content.height);
            auto temp_it_songs = std::next(it_song, 2);
            auto temp_it_titles = std::next(it_titles, 2);
            for (float y = content.y; temp_it_songs != songs.end(); ++temp_it_songs, ++temp_it_titles, y+=songpad+songh) {
                Rectangle songbox = {content.x + songpad, y, songw, songh};
                const char* songname = (*temp_it_titles).c_str();
                DrawRectangleRounded(songbox, .3f, 10, SKYBLUE);
                BeginScissorMode(songbox.x, songbox.y, songbox.width, songbox.height);
                DrawText(songname, songbox.x + songbox.width/2.0f-MeasureText(songname, 25)/2.0f,
                        songbox.y + songbox.height/2.0f, 25, WHITE);
                EndScissorMode();

            }
            EndScissorMode();    

            DrawRectangle(0, SCREEN_HEIGHT - control_panel, SCREEN_WIDTH, control_panel, BLACK);
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