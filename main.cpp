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

void drawControlPanel(float duration, float timePlayed, Rectangle controlpanel, Rectangle progbar);


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
    float duration, scroll_offset, timePlayed;
    duration = scroll_offset = timePlayed = 0;

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
        .y = controlpanel.y + controlpanel.height/2.0f - 50.0f/2.0f,
        .width = SCREEN_WIDTH,
        .height = 50.0f
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
            if (!songStarted) {
                PlayMusicStream((*it).music);
                songStarted = true;
            } else {
                if (IsKeyPressed(KEY_RIGHT) && it != std::prev(tracklist.end())) {
                    StopMusicStream((*it).music);
                    it++; 
                    songStarted = false;
                } else if (IsKeyPressed(KEY_LEFT) && it != tracklist.begin()) {
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
                } else if (CheckCollisionPointRec(GetMousePosition(), content)) {                    
                    float msmove = GetMouseWheelMove();
                    if (msmove != 0) scroll_offset += msmove * 20.0f;
                }
                duration = GetMusicTimePlayed((*it).music);
                timePlayed = duration/GetMusicTimeLength(((*it).music));
                
                // if (timePlayed >= .99) {
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
                    ClearBackground(BLANK);
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
            drawControlPanel(duration, timePlayed, controlpanel, progbar);
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

void drawControlPanel(const float duration, const float timePlayed, Rectangle controlpanel, Rectangle progbar) {
        DrawRectangle(0, SCREEN_HEIGHT - controlpanel.height, SCREEN_WIDTH, controlpanel.height, RAYWHITE);
        

        float pretty_duration = (int)duration % 60; 
        DrawText(TextFormat("%d:%2.0f", (int)duration, pretty_duration), 5, progbar.y+20, 10, BLACK);
        float fill = progbar.width*timePlayed;
        DrawRectangleRec((Rectangle) {progbar.x, progbar.y, fill, progbar.height}, BLACK);
        DrawRectangleLines(progbar.x, progbar.y, progbar.width, progbar.height, PINK);

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