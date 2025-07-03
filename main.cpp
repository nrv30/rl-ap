#include "raylib.h"
#include "raymath.h"

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

void loadSongs(FilePathList droppedFiles, std::list<std::string>* titles, std::list<Music>*songs);
void parseNameFromPath(std::string* src);



int main(void) {
    SetTraceLogLevel(LOG_DEBUG);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Audio Player");
    InitAudioDevice();
    SetMasterVolume(.5);

    // Data structures
    std::list<std::string> titles;
    std::list<Music> songs;

    std::list<Music>::iterator it_song;
    std::list<std::string>::iterator it_titles;

    // Flags
    bool songStarted, isPause, autoplay;
     songStarted = isPause = false;
    float scroll_offset = 0;

    // TODO: Embedd in the exe
    // Assets
    //Texture play = LoadTexture("../../assets/black-icon/Music-On.png");
    Image drop = LoadImage("../../assets/black-icon/Home.png");
    ImageResize(&drop, drop.width*2, drop.height*2);
    ImageRotate(&drop, 180);
    Texture drop_text = LoadTextureFromImage(drop);

    // Texture play = LoadTexture("../../assets/black-icon/Play.png");
    // Texture pause = LoadTexture("../../assets/black-icon/Pause.png");
    // Texture left_skip = LoadTexture("../../assets/black-icon/SolidArrow_Left.png");
    // Texture right_skip = LoadTexture("../../assets/black-icon/SolidArrow_Right.png");


    const float borderpad = SCREEN_WIDTH/10.0f;
    const float control_panel = SCREEN_HEIGHT/5.0f;
            
    Rectangle content = {
        .x = borderpad,
        .y = borderpad,
        .width = SCREEN_WIDTH-borderpad*2.0f,
        .height = SCREEN_HEIGHT-control_panel-borderpad*2.0f,
    };
    RenderTexture2D songqueue = LoadRenderTexture(content.width, content.height);

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
                } else if (CheckCollisionPointRec(GetMousePosition(), content)) {                    
                    float msmove = GetMouseWheelMove();
                    if (msmove != 0) scroll_offset += msmove * 20.0f;
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

            
            const float songpad = (content.height/5.0f)/10.0f;
            const float songw = content.width - songpad*2.0f;
            const float songh = content.height/5.0f - songpad;
            Color color = SKYBLUE;
            DrawRectangleLines(content.x, content.y, content.width, content.height, BLACK);

            // avoid dereference last elem
            if (it_song != std::prev(songs.end())) {
                BeginTextureMode(songqueue);
                    ClearBackground(BLANK);
                    auto temp_it_titles = std::next(it_titles, 1);
                    for (float y = 0; temp_it_titles != titles.end(); temp_it_titles++, y+=songpad+songh) {
                        Rectangle songbox = {songpad, y, songw, songh};
                        const char* songname = (*temp_it_titles).c_str();
                        DrawRectangleRounded(songbox, .3f, 10, SKYBLUE);
                        BeginScissorMode(songbox.x, songbox.y, songbox.width, songbox.height);
                            DrawText(songname, songbox.x + 5,songbox.y + songbox.height/2.0f - 10, 25, WHITE);
                        EndScissorMode();
                    }
                EndTextureMode();
                
                float listsize = (songs.size()*songpad + songs.size()*songh);
                float maxScroll =  listsize - content.height; 
                scroll_offset = Clamp(scroll_offset, -maxScroll, 0);

                Vector2 origin = { 0, 0 };
                Rectangle source = { 0, scroll_offset, (float)songqueue.texture.width, (float)-songqueue.texture.height }; // flip Y
                Rectangle dest = content;
                DrawTexturePro(songqueue.texture, source, dest,
                               origin, 0.0f, WHITE);
            }
            DrawRectangle(0, SCREEN_HEIGHT - control_panel, SCREEN_WIDTH, control_panel, BLACK);
        }
        EndDrawing();
    }
    for (auto& s : songs) {
        UnloadMusicStream(s);
    }
    UnloadRenderTexture(songqueue);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

void loadSongs(FilePathList droppedFiles, std::list<std::string>* titles, std::list<Music>* songs) {
    for (int i = 0; i < (int)droppedFiles.count; i++)
    {
        if (IsPathFile(droppedFiles.paths[i])) {
            Music song = LoadMusicStream(droppedFiles.paths[i]);
            if (IsMusicValid(song)) {

                (*titles).emplace_back(droppedFiles.paths[i]);
                parseNameFromPath((&(*titles).back()));
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