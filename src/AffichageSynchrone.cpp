#include "../include/AffichageSynchrone/AffichageSynchrone.h"
#include "../include/SynchroniseurMultiVideo/SynchroniseurMultiVideo.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <stdexcept>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>

namespace fs = filesystem;

AffichageSynchrone::AffichageSynchrone() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(784, 432, "Test affichage synchrone");
    InitAudioDevice();
    chargerListeVideos();
    SetTargetFPS(60);

    // Initialisation de la mise en page
    rectangles[0] = (Rectangle){0, 0, 144, 384}; // Vue en liste
    rectangles[1] = (Rectangle){0, 384, 144, 48}; // Bouton Générer
    rectangles[2] = (Rectangle){144, 0, 640, 360}; // Zone vidéo
    rectangles[3] = (Rectangle){144, 360, 640, 2}; // Ligne de délimitation (sous la vidéo)
    rectangles[4] = (Rectangle){152, 392, 32, 32}; // Bouton Lecture/Pause
    rectangles[5] = (Rectangle){192, 360, 408, 32}; // Étiquette d'horodatage
    rectangles[6] = (Rectangle){192, 392, 408, 32}; // Curseur de progression
    rectangles[7] = (Rectangle){608, 392, 32, 32}; // Bouton Muet
    rectangles[8] = (Rectangle){648, 360, 128, 32}; // Étiquette de volume
    rectangles[9] = (Rectangle){648, 392, 128, 32}; // Curseur de volume
}

AffichageSynchrone::~AffichageSynchrone() {
    if (IsMediaValid(video)) {
        UnloadMedia(&video);
    }
    CloseAudioDevice();
    CloseWindow();
}

void AffichageSynchrone::chargerListeVideos() {
    videoFiles.clear();
    videoSelected.clear();
    selectionOrder.clear();

    if (string path = "videos"; fs::exists(path) && fs::is_directory(path)) {
        for (const auto &entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                string ext = entry.path().extension().string();
                // Vérification simple des extensions vidéo courantes
                if (ext == ".mp4" || ext == ".avi" || ext == ".mkv" || ext == ".mov") {
                    videoFiles.push_back(entry.path().filename().string());
                    videoSelected.push_back(false);
                }
            }
        }
    }
}

void AffichageSynchrone::chargerVideo() {
    if (IsMediaValid(video)) {
        UnloadMedia(&video);
    }

    SetMediaFlag(MEDIA_VIDEO_QUEUE, tailleTampon);
    SetMediaFlag(MEDIA_AUDIO_QUEUE, tailleTampon);

    video = LoadMedia(cheminVideoComplexe.c_str());

    try {
        if (!IsMediaValid(video)) {
            throw runtime_error("Echec du chargement de la vidéo");
        }

        SetMediaState(video, MEDIA_STATE_PAUSED);

        const MediaProperties props = GetMediaProperties(video);
        duree = static_cast<float>(props.durationSec);

        SetAudioStreamVolume(video.audioStream, valeurSliderSon / 100.0f);
    } catch (const exception &e) {
        duree = 0.0f;
    }
}

void AffichageSynchrone::generer() {
    vector<string> videos;
    string path = "videos";

    for (int index : selectionOrder) {
        if (index >= 0 && index < videoFiles.size()) {
            videos.push_back(path + "/" + videoFiles[index]);
        }
    }

    if (videos.size() >= 2) {
        SynchroniseurMultiVideo synchroniseur;
        synchroniseur.configurerAnalyse(60.0, 30.0, 100);
        if (synchroniseur.genererVideoSynchronisee(videos, cheminVideoComplexe)) {
            chargerVideo();
        }
    }
}

void AffichageSynchrone::playPause() const {
    if (IsMediaValid(video)) {
        if (GetMediaState(video) == MEDIA_STATE_PLAYING) {
            SetMediaState(video, MEDIA_STATE_PAUSED);
        } else {
            SetMediaState(video, MEDIA_STATE_PLAYING);
        }
    }
}

void AffichageSynchrone::son() {
    estMuet = !estMuet;
    if (estMuet) {
        valeurSliderSonPrecedent = valeurSliderSon;
        valeurSliderSon = 0.0f;
    } else {
        valeurSliderSon = valeurSliderSonPrecedent;
    }
    setVolume(valeurSliderSon);
}

void AffichageSynchrone::sliderProgression(bool &enGlissement, bool &etaitEnLecture, float &delaiRecherche) {
    bool estEnLecture = false;
    if (IsMediaValid(video)) {
        estEnLecture = (GetMediaState(video) == MEDIA_STATE_PLAYING);
    }

    if (CheckCollisionPointRec(GetMousePosition(), rectangles[6])) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            enGlissement = true;
            etaitEnLecture = estEnLecture;
            if (IsMediaValid(video)) {
                SetMediaState(video, MEDIA_STATE_PAUSED);
                SetAudioStreamVolume(video.audioStream, 0.0f);
            }
        }
    }

    float ancienneValeur = valeurSliderProgression;
    GuiSliderBar(rectangles[6], SLIDER_PROGRESSION, nullptr, &valeurSliderProgression, 0.0f, duree);

    if (enGlissement) {
        if (valeurSliderProgression != ancienneValeur) {
            if (IsMediaValid(video)) {
                SetMediaPosition(video, valeurSliderProgression);
                SetMediaState(video, MEDIA_STATE_PLAYING);
                UpdateMedia(&video);
                SetMediaState(video, MEDIA_STATE_PAUSED);
            }
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || !IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            enGlissement = false;
            if (IsMediaValid(video)) {
                if (etaitEnLecture) {
                    SetMediaState(video, MEDIA_STATE_PLAYING);
                }
                SetAudioStreamVolume(video.audioStream, valeurSliderSon / 100.0f);
            }
            delaiRecherche = 0.2f;
        }
    }
}

void AffichageSynchrone::sliderVolume() {
    float nouveauVolume = valeurSliderSon;
    GuiSliderBar(rectangles[9], SLIDER_SON, nullptr, &nouveauVolume, 0.0f, 100.0f);
    if (nouveauVolume != valeurSliderSon) {
        valeurSliderSon = nouveauVolume;
        if (valeurSliderSon > 0.0f) {
            estMuet = false;
        }
        setVolume(valeurSliderSon);
    }
}

void AffichageSynchrone::afficherVideo() const {
    const auto videoWidth = static_cast<float>(video.videoTexture.width);
    const auto videoHeight = static_cast<float>(video.videoTexture.height);

    if (videoWidth > 0 && videoHeight > 0) {
        const float scaleX = rectangles[2].width / videoWidth;
        const float scaleY = rectangles[2].height / videoHeight;

        // On prend la plus petite échelle pour que ça rentre sans être étiré
        const float scale = (scaleX < scaleY) ? scaleX : scaleY;

        const float destWidth = videoWidth * scale;
        const float destHeight = videoHeight * scale;

        // Centrage dans la zone
        const float destX = rectangles[2].x + (rectangles[2].width - destWidth) / 2.0f;
        const float destY = rectangles[2].y + (rectangles[2].height - destHeight) / 2.0f;

        const Rectangle source = {0.0f, 0.0f, videoWidth, videoHeight};
        const Rectangle dest = {destX, destY, destWidth, destHeight};
        constexpr Vector2 origin = {0.0f, 0.0f};

        DrawTexturePro(video.videoTexture, source, dest, origin, 0.0f, WHITE);
    }
}

void AffichageSynchrone::afficherListeFichiers() {
    Rectangle view = {0};
    GuiScrollPanel(rectangles[0], nullptr, (Rectangle){0, 0, rectangles[0].width - 16, static_cast<float>(videoFiles.size()) * 30}, &scrollPosition, &view);
    BeginScissorMode(view.x, view.y, view.width, view.height);
        for (size_t i = 0; i < videoFiles.size(); ++i) {
            Rectangle itemRect = {rectangles[0].x + 10 + scrollPosition.x, rectangles[0].y + 10 + i * 30 + scrollPosition.y, 20, 20};

            int order = 0;
            if (videoSelected[i]) {
                 auto it = std::find(selectionOrder.begin(), selectionOrder.end(), i);
                 if (it != selectionOrder.end()) {
                     order = std::distance(selectionOrder.begin(), it) + 1;
                 }
            }

            string label = videoFiles[i];
            if (order > 0) {
                label += " (" + to_string(order) + ")";
            }

            bool checked = videoSelected[i];
            bool prevChecked = checked;
            GuiCheckBox(itemRect, label.c_str(), &checked);

            if (checked != prevChecked) {
                videoSelected[i] = checked;
                if (checked) {
                    selectionOrder.push_back(i);
                } else {
                    auto it = std::find(selectionOrder.begin(), selectionOrder.end(), i);
                    if (it != selectionOrder.end()) {
                        selectionOrder.erase(it);
                    }
                }
            }
        }
    EndScissorMode();
}

void AffichageSynchrone::executer() {
    bool enGlissement = false;
    bool etaitEnLecture = false;
    float delaiRecherche = 0.0f;

    while (!WindowShouldClose()) {
        if (IsMediaValid(video)) {
            UpdateMedia(&video);
        }

        if (delaiRecherche > 0) delaiRecherche -= GetFrameTime();

        if (!enGlissement && delaiRecherche <= 0 && IsMediaValid(video)) {
            valeurSliderProgression = static_cast<float>(GetMediaPosition(video));
        }

        BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        if (IsMediaValid(video)) afficherVideo();

        DrawRectangleRec(rectangles[3], GRAY);

        afficherListeFichiers();

        if (GuiButton(rectangles[1], BOUTTON_GENERER)) generer();

        bool estEnLecture = false;
        if (IsMediaValid(video)) {
            estEnLecture = (GetMediaState(video) == MEDIA_STATE_PLAYING);
        }

        if (GuiButton(rectangles[4], estEnLecture ? "#133#" : "#131#")) playPause();

        const int minutes = static_cast<int>(valeurSliderProgression) / 60;
        const int seconds = static_cast<int>(valeurSliderProgression) % 60;
        const int dureeMinutes = static_cast<int>(duree) / 60;
        const int dureeSeconds = static_cast<int>(duree) % 60;
        GuiLabel(rectangles[5], TextFormat("%02d:%02d / %02d:%02d", minutes, seconds, dureeMinutes, dureeSeconds));

        sliderProgression(enGlissement, etaitEnLecture, delaiRecherche);

        if (GuiButton(rectangles[7], estMuet ? "#128#" : "#122#")) son();

        GuiLabel(rectangles[8], TextFormat("%d%%", static_cast<int>(valeurSliderSon)));

        sliderVolume();

        EndDrawing();
    }
}

void AffichageSynchrone::setTailleTampon(const int taille) {
    tailleTampon = taille;
    chargerVideo();
}

void AffichageSynchrone::setVolume(const float volume) {
    valeurSliderSon = volume;
    if (IsMediaValid(video)) {
        SetAudioStreamVolume(video.audioStream, valeurSliderSon / 100.0f);
    }
}
