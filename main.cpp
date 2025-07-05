#include "raylib.h"
#include "raymath.h"

#include <list>
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

void seek_by_amount(std::list<Track>::iterator* it_pt, const float length, const float abs_timeplayed, const float amount);


void drawTitle(std::string title, Rectangle progbar, Rectangle controlpanel);
void drawProgbar(const float abs_timeplayed, const float timeplayed, Rectangle progbar);
void drawVolbar(const float vol, Rectangle volbar);


int main(void) {
    SetTraceLogLevel(LOG_DEBUG);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Audio Player");
    InitAudioDevice();

    // Data structures
    std::list<Track> tracklist;
    std::list<Track>::iterator it;

    // Flags & vars
    bool songStarted, isPause, autoplay;
     songStarted = isPause = false;
    float length, abs_timeplayed, scroll_offset, timeplayed;
    length = abs_timeplayed = scroll_offset = timeplayed = 0;
    float vol = 0.5;

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

    const int progheight = 25.0f;
    Rectangle progbar = {
        .x = borderpad,
        .y = controlpanel.y + controlpanel.height*.75f - progheight/2.0f,
        .width = SCREEN_WIDTH-borderpad*2.0f,
        .height = progheight
    };

    const float vol_width = 12.5;
    const float vertpad = 20.0f;
    Rectangle volbar = {
        .x = SCREEN_WIDTH - (borderpad/2.0f - vol_width/2.0f),
        .y = controlpanel.y + vertpad,
        .width = vol_width,
        .height = SCREEN_HEIGHT - (controlpanel.y + vertpad) - (SCREEN_HEIGHT-(progbar.y + progbar.height)),
    };

    SetTargetFPS(60);
    SetMasterVolume(.5);
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
                length = GetMusicTimeLength(((*it).music));
                PlayMusicStream((*it).music);
                songStarted = true;
            } else {
                Vector2 mousepos = GetMousePosition();
                if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_RIGHT)
                    && it != std::prev(tracklist.end())) {
                    StopMusicStream((*it).music);
                    songStarted = false;
                    it++; 
                } else if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_LEFT)
                           && it != tracklist.begin()) {
                    StopMusicStream((*it).music);
                    songStarted = false;
                    it--; 
                } else if (IsKeyPressed(KEY_SPACE)) {
                    if (isPause) {
                        ResumeMusicStream((*it).music);
                    }
                    else {
                        PauseMusicStream((*it).music);
                    }
                    isPause = !isPause;
                } else if (IsKeyPressed(KEY_RIGHT)) {
                    seek_by_amount(&it, length, abs_timeplayed, 10.0f);
                } else if (IsKeyPressed(KEY_LEFT)) {
                    seek_by_amount(&it, length, abs_timeplayed, -10.0f);
                } else if (CheckCollisionPointRec(mousepos, progbar)) {
                    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                        float pos = (mousepos.x - progbar.x) / progbar.width;
                        SeekMusicStream((*it).music, length*pos);
                    }
                } else if (CheckCollisionPointRec(mousepos, content)) {                    
                    float msmove = GetMouseWheelMove();
                    if (msmove != 0) scroll_offset += msmove * 20.0f;
                } else if (CheckCollisionPointRec(mousepos, volbar)) {
                    if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                        float pos = (mousepos.y - volbar.y) / volbar.height;
                        vol = pos;
                        SetMasterVolume(pos);
                    }
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

            BeginTextureMode(songqueue);
            ClearBackground(BLACK);
            if (it != std::prev(tracklist.end())) {
                auto temp_it = std::next(it, 1);
                for (float y = 0; temp_it != tracklist.end(); temp_it++, y+=songpad+songh) {
                    Rectangle songbox = {songpad, y, songw, songh};
                    const char* songname = (*temp_it).title.c_str();
                    DrawRectangleRounded(songbox, .3f, 10, DARKBLUE);
                    BeginScissorMode(songbox.x, songbox.y, songbox.width, songbox.height);
                        DrawText(songname, songbox.x + 5,songbox.y + songbox.height/2.0f - 10, 25, SKYBLUE);
                    EndScissorMode();
                }
            }
            else 
            {
                const char* text = "Queue is EMPTY"; 
                const int fontsize = 50;
                DrawText(text, content.width/2.0f - MeasureText(text, fontsize)/2.0f, content.height/2.0f, 
                         fontsize, SKYBLUE);    
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
            // draw control panel
            DrawRectangle(0, SCREEN_HEIGHT - controlpanel.height, SCREEN_WIDTH, controlpanel.height, LIGHTGRAY);

            drawTitle((*it).title, progbar, controlpanel);
            drawProgbar(abs_timeplayed, timeplayed, progbar);
            drawVolbar(vol, volbar);
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

void seek_by_amount(std::list<Track>::iterator* it_pt, const float length, const float abs_timeplayed, const float amount) {
    Clamp(abs_timeplayed + amount, 0, length);
    SeekMusicStream((*(*it_pt)).music, abs_timeplayed + amount); 
}

void drawTitle(std::string title, Rectangle progbar, Rectangle controlpanel) {
        const char* text = title.c_str();
        const int fontsize = 24;
        const float vertpad = 20;
        BeginScissorMode(progbar.x, controlpanel.y, progbar.width, controlpanel.height);
            DrawText(text, progbar.x + progbar.width/2.0f - MeasureText(text, fontsize)/2.0f, 
                     controlpanel.y+vertpad, fontsize, DARKGRAY);
        EndScissorMode();
}
void drawProgbar(const float abs_timeplayed, const float timeplayed, Rectangle progbar) {
        int sec = (int)abs_timeplayed % 60; 
        int min = ((int)abs_timeplayed - sec);
        if (min >= 60) min /= 60; 
        DrawText(TextFormat("%02d:%02d", min, sec), 5, progbar.y+5, 20, DARKGRAY);
        float fill = progbar.width*timeplayed;
        DrawRectangleRec((Rectangle) {progbar.x, progbar.y, fill, progbar.height}, SKYBLUE);
        DrawRectangleLines(progbar.x, progbar.y, progbar.width, progbar.height, DARKGRAY);
}

void drawVolbar(const float vol, Rectangle volbar) {
    DrawRectangleRec((Rectangle){volbar.x, volbar.y, volbar.width, volbar.height*vol}, SKYBLUE);
    DrawRectangleLines(volbar.x, volbar.y, volbar.width, volbar.height, DARKGRAY);
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