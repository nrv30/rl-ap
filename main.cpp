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

void seek_by_amount(std::list<Track>::iterator it, const float length, const float abs_timeplayed, const float amount);
void del(std::list<Track>::iterator temp, std::list<Track>* tracklist);


// UI
typedef enum BUTTON_STATE {
    NORMAL,
    SELECTED,
    HOVERED,
    HOVERED_EXIT,
    HOEVERED_EXIT_CLICK,
};

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

    // Flags
    bool songStarted, isPause, autoplay;
    songStarted = isPause = false;

    float length, abs_timeplayed, scroll_offset, timeplayed;
    length = abs_timeplayed = scroll_offset = timeplayed = 0;
    float vol = 0.5;

    Vector2 mspos;

    Texture2D atlas = LoadTexture("assets.png");

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

    Vector2 button_size = {454, 111};
    Rectangle exit_box = {
        .x = 395, 
        .y = 18,
        .width = 38,
        .height = 41,
    };

    Rectangle hovered_exit = {
        .x = 0,
        .y = 0,
        .width = button_size.x,
        .height = button_size.y,
    };

    Rectangle hovered_exit_click = {
        .x = button_size.x,
        .y = 0,
        .width = button_size.x,
        .height = button_size.y,
    };

    Rectangle selected = {
        .x = 0,
        .y = button_size.y,
        .width = button_size.x,
        .height = button_size.y,
    };

    Rectangle hovered = {
        .x = button_size.x,
        .y = button_size.y,
        .width = button_size.x,
        .height = button_size.y,
    };

    Rectangle normal = {
        .x = 0,
        .y = button_size.y*2.0f,
        .width = button_size.x,
        .height = button_size.y,
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
                mspos = GetMousePosition();
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
                    seek_by_amount(it, length, abs_timeplayed, 10.0f);
                } else if (IsKeyPressed(KEY_LEFT)) {
                    seek_by_amount(it, length, abs_timeplayed, -10.0f);
                } else if (CheckCollisionPointRec(mspos, progbar)) {
                    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                        float pos = (mspos.x - progbar.x) / progbar.width;
                        SeekMusicStream((*it).music, length*pos);
                    }
                } else if (CheckCollisionPointRec(mspos, content)) {                    
                    float msmove = GetMouseWheelMove();
                    if (msmove != 0) scroll_offset -= msmove * 20.0f;
                } else if (CheckCollisionPointRec(mspos, volbar)) {
                    if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                        float pos = (mspos.y - volbar.y) / volbar.height;
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
            // DrawTexturePro(drop, 
            //     Rectangle {0, 0, (float)drop.width, (float)-drop.height}, 
            //     (Rectangle) {SCREEN_WIDTH/2.0f-drop.width/2.0f, SCREEN_HEIGHT/2.0f-drop.height/2.0f, (float)drop.width*2, (float)drop.height*2}, 
            //     (Vector2) {0,0}, 0, 
            //     WHITE);

        } else {
            const float songpad = (content.height/5.0f)/10.0f;
            const float songw = normal.width;
            const float songh = normal.height;
            DrawRectangleLines(content.x, content.y, content.width, content.height, BLACK);
                float total_height = (tracklist.size()-1) * (songh + songpad);
                float maxScroll = fmaxf(total_height - content.height, 0);
                scroll_offset = Clamp(scroll_offset, 0.0f, maxScroll);

            BeginScissorMode(content.x, content.y, content.width, content.height);
            if (it != std::prev(tracklist.end())) {
                auto temp_it = std::next(it, 1);
                float y = content.y;
                for (int i = 0; temp_it != tracklist.end(); i++, temp_it++) {
                    Rectangle src;
                    BUTTON_STATE bs;
                    Rectangle song_box = {
                        content.x + songpad,
                        y + i * (songh + songpad) - scroll_offset,
                        songw, 
                        songh
                    };

                    // exit_box_size is the width and height of the exit_box
                    Rectangle song_box_exit_box = {
                        .x = song_box.x + exit_box.x,
                        .y = song_box.y + exit_box.y,
                        .width = exit_box.width,
                        .height = exit_box.height,
                    };

                    if (song_box.y + song_box.height < content.y || song_box.y > content.y + content.height)
                        continue;
                    if (CheckCollisionPointRec(mspos, song_box)) {
                        if (CheckCollisionPointRec(mspos, song_box_exit_box)) {
                            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                                std::list<Track>::iterator del_it = temp_it;
                                temp_it = std::prev(temp_it, 1);
                                del(del_it, &tracklist);
                                i--;
                                continue;
                            } else {
                                src = hovered_exit;
                            }
                        } else {
                            src = hovered;
                        }
                    } else {
                        src = normal;
                    }

                    const char* songname = (*temp_it).title.c_str();
                    const int fontsize = 24;
                    //DrawRectangleRounded(song_box, .3f, 10, DARKBLUE);
                    DrawTextureRec(atlas, src, (Vector2) {song_box.x, song_box.y,}, WHITE);
                    DrawText(songname, song_box.x + 5, song_box.y + song_box.height/2.0f-5, fontsize, LIGHTGRAY);
                    DrawRectangleLines(song_box_exit_box.x, 
                        song_box_exit_box.y, 
                        song_box_exit_box.width, 
                        song_box_exit_box.height, 
                        BLACK);
                }
            }
            else 
            {
                const char* text = "Queue is EMPTY"; 
                const int fontsize = 50;
                DrawText(text, content.x + content.width/2.0f - MeasureText(text, fontsize)/2.0f, content.height/2.0f, 
                         fontsize, SKYBLUE);    
            }
            EndScissorMode();
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
    UnloadTexture(atlas);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

void del(std::list<Track>::iterator temp, std::list<Track>* tracklist) {
    (*tracklist).erase(temp);
}

void seek_by_amount(std::list<Track>::iterator it, const float length, const float abs_timeplayed, const float amount) {
    Clamp(abs_timeplayed + amount, 0, length);
    SeekMusicStream((*it).music, abs_timeplayed + amount); 
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

// c:/hello.mp4 
// -> hello
std::string parseNameFromPath(std::string src) {
    const int maxchars = 35;
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
    
    if (temp.size() > maxchars) temp = temp.substr(0, maxchars+1);
    return temp;
}