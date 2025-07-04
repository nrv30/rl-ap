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

typedef struct Track {
    Music music;
    std::string title;
} Track;

void loadSongs(FilePathList droppedFiles, std::list<Track>* tracklist_pt);
std::string parseNameFromPath(std::string src);

void seek(std::list<Track>::iterator* it_pt, const float length, const float abs_timeplayed, const float amount);
void drawControlPanel(float duration, float timeplayed, Rectangle controlpanel, Rectangle progbar);


int main(void) {
    SetTraceLogLevel(LOG_DEBUG);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Audio Player");
    InitAudioDevice();
    SetMasterVolume(.5);

    // Data structures
    std::list<Track> tracklist;
    std::list<Track>::iterator it;

    // Flags
    bool songStarted, isPause, autoplay;
     songStarted = isPause = false;
    float length, abs_timeplayed, scroll_offset, timeplayed;
    length = abs_timeplayed = scroll_offset = timeplayed = 0;

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

    // UI
    const float borderpad = SCREEN_WIDTH/10.0f;

    Rectangle controlpanel = {
        .x = 0,
        .y = SCREEN_HEIGHT - SCREEN_HEIGHT/5.0f,
        .width = SCREEN_WIDTH,
        .height = SCREEN_HEIGHT/5.0f,
    };
            
    Rectangle content = {
        .x = borderpad,
        .y = borderpad,
        .width = SCREEN_WIDTH-borderpad*2.0f,
        .height = SCREEN_HEIGHT-controlpanel.height-borderpad*2.0f,
    };
    RenderTexture2D songqueue = LoadRenderTexture(content.width, content.height);

    Rectangle progbar = {
        .x = borderpad,
        .y = controlpanel.y + controlpanel.height*.75f - 25.0f/2.0f,
        .width = SCREEN_WIDTH-borderpad*2.0f,
        .height = 25.0f
    };

    SetTargetFPS(60);
    while(!WindowShouldClose()) {

        if (IsFileDropped()) {   
            FilePathList droppedFiles = LoadDroppedFiles();
            loadSongs(droppedFiles, &tracklist);  
            UnloadDroppedFiles(droppedFiles);
            if (state == START && tracklist.size() > 0) {
                it = tracklist.begin();
                state = PLAYING;
            }
        }      

        if (state == PLAYING) {
            // TraceLog(LOG_DEBUG, TextFormat("length: %f", abslength));
            if (!songStarted) {
                length = GetMusicTimeLength(((*it).music));
                PlayMusicStream((*it).music);
                songStarted = true;
            } else {
                if (IsKeyDown(KEY_RIGHT) && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) 
                    && it != std::prev(tracklist.end())) {
                    StopMusicStream((*it).music);
                    it++; 
                    songStarted = false;
                } else if (IsKeyDown(KEY_RIGHT) && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
                           && it != tracklist.begin()) {
                    StopMusicStream((*it).music);
                    it--; 
                    songStarted = false;
                } else if (IsKeyPressed(KEY_SPACE)) {
                    if (isPause) {
                        ResumeMusicStream((*it).music);
                    }
                    else {
                        PauseMusicStream((*it).music);
                    }
                    isPause = !isPause;
                } else if (IsKeyPressed(KEY_RIGHT)) {
                    seek(&it, length, abs_timeplayed, 10.0f);
                } else if (IsKeyPressed(KEY_LEFT)) {
                    seek(&it, length, abs_timeplayed, -10.0f);
                }
                
                if (CheckCollisionPointRec(GetMousePosition(), content)) {                    
                    float msmove = GetMouseWheelMove();
                    if (msmove != 0) scroll_offset += msmove * 20.0f;
                }

                abs_timeplayed = GetMusicTimePlayed((*it).music);
                timeplayed = abs_timeplayed/length;
                
                // if (timeplayed >= .99) {
                //     skipForward(&track, &songStarted);
                UpdateMusicStream((*it).music);
            }
        }

        BeginDrawing();
        ClearBackground(DARKGRAY);

        if (state == START) {
            const char* text = "DROP MUSIC";
            const int fontsize = 50;
            DrawText(text, SCREEN_WIDTH/2.0f - MeasureText(text, fontsize)/2, SCREEN_HEIGHT/3.0f - fontsize, fontsize, SKYBLUE);
            DrawTexture(drop_text, SCREEN_WIDTH/2.0f-drop.width/2.0f, SCREEN_HEIGHT/2.0f, WHITE);
        } else {
            const float songpad = (content.height/5.0f)/10.0f;
            const float songw = content.width - songpad*2.0f;
            const float songh = content.height/5.0f - songpad;
            Color color = SKYBLUE;
            DrawRectangleLines(content.x, content.y, content.width, content.height, BLACK);

            // avoid dereference last elem
            if (it != std::prev(tracklist.end())) {
                BeginTextureMode(songqueue);
                    auto temp_it = std::next(it, 1);
                    for (float y = 0; temp_it != tracklist.end(); temp_it++, y+=songpad+songh) {
                        Rectangle songbox = {songpad, y, songw, songh};
                        const char* songname = (*temp_it).title.c_str();
                        DrawRectangleRounded(songbox, .3f, 10, SKYBLUE);
                        BeginScissorMode(songbox.x, songbox.y, songbox.width, songbox.height);
                            DrawText(songname, songbox.x + 5,songbox.y + songbox.height/2.0f - 10, 25, WHITE);
                        EndScissorMode();
                    }
                EndTextureMode();
                
                float listsize = (tracklist.size()*songpad + tracklist.size()*songh);
                float maxScroll =  listsize - content.height; 
                scroll_offset = Clamp(scroll_offset, -maxScroll, 0);

                Vector2 origin = { 0, 0 };
                Rectangle source = { 0, scroll_offset, (float)songqueue.texture.width, (float)-songqueue.texture.height }; // flip Y
                Rectangle dest = content;
                DrawTexturePro(songqueue.texture, source, dest,
                               origin, 0.0f, WHITE);
            }
            drawControlPanel(abs_timeplayed, timeplayed, controlpanel, progbar);
        }
        EndDrawing();
    }
    for (auto& s : tracklist) {
        UnloadMusicStream(s.music);
    }
    UnloadRenderTexture(songqueue);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

void seek(std::list<Track>::iterator* it_pt, const float length, const float abs_timeplayed, const float amount) {
    // TraceLog(LOG_DEBUG, TextFormat("amount: %f", amount));
    // TraceLog(LOG_DEBUG, TextFormat("abs_timeplayed: %f", abs_timeplayed));
    float position = Clamp(abs_timeplayed + amount, 0.0f, length);
    // TraceLog(LOG_DEBUG, TextFormat("position: %d", position));
    SeekMusicStream((*(*it_pt)).music, position); 
}

void drawControlPanel(const float duration, const float timeplayed, Rectangle controlpanel, Rectangle progbar) {
        DrawRectangle(0, SCREEN_HEIGHT - controlpanel.height, SCREEN_WIDTH, controlpanel.height, LIGHTGRAY);
        
        int sec = (int)duration % 60; 
        int min = ((int)duration - sec);
        if (min >= 60) min /= 60; 
        DrawText(TextFormat("%02d:%02d", min, sec), 5, progbar.y+5, 20, DARKGRAY);
        float fill = progbar.width*timeplayed;
        DrawRectangleRec((Rectangle) {progbar.x, progbar.y, fill, progbar.height}, SKYBLUE);
        DrawRectangleLines(progbar.x, progbar.y, progbar.width, progbar.height, DARKGRAY);

}
void loadSongs(FilePathList droppedFiles, std::list<Track>* tracklist_pt) {
    for (int i = 0; i < (int)droppedFiles.count; i++)
    {
        if (IsPathFile(droppedFiles.paths[i])) {
            Music song = LoadMusicStream(droppedFiles.paths[i]);
            if (IsMusicValid(song)) {
                std::string title = droppedFiles.paths[i];
                Track track = {0};
                track.title = parseNameFromPath(title);
                song.looping = false;
                track.music = song;
                (*tracklist_pt).emplace_back(track);
            } else {
                UnloadMusicStream(song);
            }
        } else {
            FilePathList dir = LoadDirectoryFiles(droppedFiles.paths[i]);
            loadSongs(dir, tracklist_pt);
            UnloadDirectoryFiles(dir);
        }
    }
}

// has to be legit music file
std::string parseNameFromPath(std::string src) {
    std::string temp;
    std::string::reverse_iterator it_s = src.rbegin();
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
    return temp;
}