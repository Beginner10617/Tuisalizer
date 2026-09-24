#include "audio_interface.h"
#include "logging.h"
#include "raylib/src/raylib.h"
int main() {
  const float ratio = 2340.0 / 1080.0;
  const int screenWidth = 400;
  const int screenHeight = screenWidth * ratio;

  InitWindow(screenWidth, screenHeight,
             "raylib [audio] example - music stream");

  LogInfo("Creating audio backend");
  AudioBackend *ab = audio_create();

  LogInfo("Initializing audio backend");
  audio_init(ab);

  LogInfo("Loading music");
  if (audio_load(ab, "/Users/zain/Music/youtube/music_deftones_7_words.mp3")) {
    audio_play(ab);
    LogInfo("Loading completed");
  } else
    LogError("Unable to load music");

  bool pause = false;

  float volume = 0.8f;
  audio_set_volume(ab, volume);

  SetTargetFPS(30);

  // Main loop
  while (!WindowShouldClose()) {
    // Update
    audio_update(ab);

    // Restart music playing (stop and play)
    if (IsKeyPressed(KEY_SPACE)) {
      audio_stop(ab);
      audio_play(ab);
    }

    // Pause/Resume music playing
    if (IsKeyPressed(KEY_P)) {
      pause = !pause;

      if (pause)
        audio_pause(ab);
      else
        audio_resume(ab);
    }

    // Set audio volume
    if (IsKeyDown(KEY_DOWN)) {
      volume -= 0.05f;
      if (volume < 0.0f)
        volume = 0.0f;
      audio_set_volume(ab, volume);
    } else if (IsKeyDown(KEY_UP)) {
      volume += 0.05f;
      if (volume > 1.0f)
        volume = 1.0f;
      audio_set_volume(ab, volume);
    }

    // Get normalized time played for current music stream
    float timePlayed = audio_position(ab) / audio_duration(ab);

    if (timePlayed > 1.0f)
      timePlayed = 1.0f; // Make sure time played is no longer than music

    // Draw
    BeginDrawing();

    ClearBackground(RAYWHITE);

    DrawText("MUSIC SHOULD BE PLAYING!", 255, 150, 20, LIGHTGRAY);

    DrawText("LEFT-RIGHT for PAN CONTROL", 320, 74, 10, DARKBLUE);
    DrawRectangle(300, 100, 200, 12, LIGHTGRAY);
    DrawRectangleLines(300, 100, 200, 12, GRAY);

    DrawRectangle(200, 200, 400, 12, LIGHTGRAY);
    DrawRectangle(200, 200, (int)(timePlayed * 400.0f), 12, MAROON);
    DrawRectangleLines(200, 200, 400, 12, GRAY);

    DrawText("PRESS SPACE TO RESTART MUSIC", 215, 250, 20, LIGHTGRAY);
    DrawText("PRESS P TO PAUSE/RESUME MUSIC", 208, 280, 20, LIGHTGRAY);

    DrawText("UP-DOWN for VOLUME CONTROL", 320, 334, 10, DARKGREEN);
    DrawRectangle(300, 360, 200, 12, LIGHTGRAY);
    DrawRectangleLines(300, 360, 200, 12, GRAY);
    DrawRectangle((int)(300 + volume * 200 - 5), 352, 10, 28, DARKGRAY);

    EndDrawing();
  }

  // De-Initialization
  audio_unload(ab); // Unload music stream buffers from RAM

  audio_shutdown(ab);

  CloseWindow();

  return 0;
}
