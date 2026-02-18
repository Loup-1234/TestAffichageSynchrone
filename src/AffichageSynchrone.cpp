#include "../include/AffichageSynchrone/AffichageSynchrone.h"
#include "../include/SynchroniseurMultiVideo/SynchroniseurMultiVideo.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <stdexcept>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <thread>

namespace fs = filesystem;

AffichageSynchrone::AffichageSynchrone() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 450, "Test affichage synchrone");
    SetWindowMinSize(800, 450);
    InitAudioDevice();
    chargerListeVideos();
    SetTargetFPS(60);

    miseAJourDisposition();
}

AffichageSynchrone::~AffichageSynchrone() {
    if (IsMediaValid(video)) {
        UnloadMedia(&video);
    }
    CloseAudioDevice();
    CloseWindow();
}

void AffichageSynchrone::miseAJourDisposition() {
    const auto largeurEcran = static_cast<float>(GetScreenWidth());
    const auto hauteurEcran = static_cast<float>(GetScreenHeight());

    // Initialisation de la mise en page
    rectangles[0] = (Rectangle){0, 0, 150, hauteurEcran - 48}; // Vue en liste
    rectangles[1] = (Rectangle){0, hauteurEcran - 48, 150, 48}; // Bouton Générer
    rectangles[2] = (Rectangle){150, 0, largeurEcran - 150, hauteurEcran - 72}; // Zone vidéo
    rectangles[3] = (Rectangle){150, hauteurEcran - 72, largeurEcran - 150, 2}; // Ligne de délimitation (sous la vidéo)
    rectangles[4] = (Rectangle){158, hauteurEcran - 40, 32, 32}; // Bouton Lecture/Pause
    rectangles[5] = (Rectangle){198, hauteurEcran - 72, largeurEcran - 382, 32}; // Étiquette d'horodatage
    rectangles[6] = (Rectangle){198, hauteurEcran - 40, largeurEcran - 382, 32}; // Curseur de progression
    rectangles[7] = (Rectangle){largeurEcran - 176, hauteurEcran - 40, 32, 32}; // Bouton Muet
    rectangles[8] = (Rectangle){largeurEcran - 136, hauteurEcran - 72, 128, 32}; // Étiquette de volume
    rectangles[9] = (Rectangle){largeurEcran - 136, hauteurEcran - 40, 128, 32}; // Curseur de volume
}

void AffichageSynchrone::chargerListeVideos() {
    fichiersVideo.clear();
    videosSelectionnees.clear();
    ordreSelection.clear();

    if (string chemin = "videos"; fs::exists(chemin) && fs::is_directory(chemin)) {
        for (const auto &entree: fs::directory_iterator(chemin)) {
            if (entree.is_regular_file()) {
                string ext = entree.path().extension().string();
                // Vérification simple des extensions vidéo courantes
                if (ext == ".mp4" || ext == ".avi" || ext == ".mkv" || ext == ".mov") {
                    fichiersVideo.push_back(entree.path().filename().string());
                    videosSelectionnees.push_back(false);
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
    if (generationEnCours) return;

    vector<string> videos;
    string chemin = "videos";

    for (int index: ordreSelection) {
        if (index >= 0 && index < fichiersVideo.size()) {
            videos.push_back(chemin + "/" + fichiersVideo[index]);
        }
    }

    if (videos.size() >= 2) {
        // Décharger la vidéo actuelle avant de commencer la génération
        if (IsMediaValid(video)) {
            UnloadMedia(&video);
            // Réinitialiser les variables liées à la vidéo
            duree = 0.0f;
            valeurSliderProgression = 0.0f;
        }

        generationEnCours = true;
        thread([this, videos]() {
            SynchroniseurMultiVideo synchroniseur;
            synchroniseur.configurerAnalyse(60.0, 30.0, 100);
            if (synchroniseur.genererVideoSynchronisee(videos, cheminVideoComplexe)) {
                videoGeneree = true;
            }
            generationEnCours = false;
        }).detach();
    }
}

void AffichageSynchrone::lecturePause() const {
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

void AffichageSynchrone::barreProgression(bool &enGlissement, bool &etaitEnLecture, float &delaiRecherche) {
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
    GuiSliderBar(rectangles[6], BARRE_PROGRESSION, nullptr, &valeurSliderProgression, 0.0f, duree);

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

void AffichageSynchrone::barreVolume() {
    float nouveauVolume = valeurSliderSon;
    GuiSliderBar(rectangles[9], BARRE_SON, nullptr, &nouveauVolume, 0.0f, 100.0f);
    if (nouveauVolume != valeurSliderSon) {
        valeurSliderSon = nouveauVolume;
        if (valeurSliderSon > 0.0f) {
            estMuet = false;
        }
        setVolume(valeurSliderSon);
    }
}

void AffichageSynchrone::afficherVideo() const {
    const auto largeurVideo = static_cast<float>(video.videoTexture.width);
    const auto hauteurVideo = static_cast<float>(video.videoTexture.height);

    if (largeurVideo > 0 && hauteurVideo > 0) {
        const float echelleX = rectangles[2].width / largeurVideo;
        const float echelleY = rectangles[2].height / hauteurVideo;

        // On prend la plus petite échelle pour que ça rentre sans être étiré
        const float echelle = (echelleX < echelleY) ? echelleX : echelleY;

        const float destLargeur = largeurVideo * echelle;
        const float destHauteur = hauteurVideo * echelle;

        // Centrage dans la zone
        const float destX = rectangles[2].x + (rectangles[2].width - destLargeur) / 2.0f;
        const float destY = rectangles[2].y + (rectangles[2].height - destHauteur) / 2.0f;

        const Rectangle source = {0.0f, 0.0f, largeurVideo, hauteurVideo};
        const Rectangle dest = {destX, destY, destLargeur, destHauteur};
        constexpr Vector2 origine = {0.0f, 0.0f};

        DrawTexturePro(video.videoTexture, source, dest, origine, 0.0f, WHITE);
    }
}

void AffichageSynchrone::afficherListeFichiers() {
    Rectangle vue = {0};

    GuiScrollPanel(rectangles[0], nullptr,
                   (Rectangle){0, 0,
                       rectangles[0].width - 16,
                       static_cast<float>(fichiersVideo.size()) * 30}, &positionDefilement, &vue);

    BeginScissorMode(vue.x, vue.y, vue.width, vue.height);

    for (size_t i = 0; i < fichiersVideo.size(); ++i) {
        Rectangle itemRect = {
            rectangles[0].x + 10 + positionDefilement.x, rectangles[0].y + 10 + i * 30 + positionDefilement.y, 20, 20
        };

        int ordre = 0;
        if (videosSelectionnees[i]) {
            auto it = find(ordreSelection.begin(), ordreSelection.end(), i);
            if (it != ordreSelection.end()) {
                ordre = distance(ordreSelection.begin(), it) + 1;
            }
        }

        string etiquette = fichiersVideo[i];
        if (ordre > 0) {
            etiquette += " (" + to_string(ordre) + ")";
        }

        bool coche = videosSelectionnees[i];
        bool etaitCoche = coche;
        GuiCheckBox(itemRect, etiquette.c_str(), &coche);

        if (coche != etaitCoche) {
            videosSelectionnees[i] = coche;
            if (coche) {
                ordreSelection.push_back(i);
            } else {
                auto it = find(ordreSelection.begin(), ordreSelection.end(), i);
                if (it != ordreSelection.end()) {
                    ordreSelection.erase(it);
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
    float rotationChargement = 0.0f;

    while (!WindowShouldClose()) {
        if (IsWindowResized()) miseAJourDisposition();

        if (videoGeneree) {
            chargerVideo();
            videoGeneree = false;
        }

        if (IsMediaValid(video)) UpdateMedia(&video);

        if (delaiRecherche > 0) delaiRecherche -= GetFrameTime();

        if (!enGlissement && delaiRecherche <= 0 && IsMediaValid(video))
            valeurSliderProgression = static_cast<float>(GetMediaPosition(video));

        BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        if (IsMediaValid(video)) afficherVideo();

        DrawRectangleRec(rectangles[3], GRAY);

        afficherListeFichiers();

        if (generationEnCours) {
            GuiDisable();
        }
        if (GuiButton(rectangles[1], BOUTON_GENERER)) generer();
        if (generationEnCours) {
            GuiEnable();
        }

        bool estEnLecture = false;

        if (IsMediaValid(video)) estEnLecture = (GetMediaState(video) == MEDIA_STATE_PLAYING);

        if (GuiButton(rectangles[4], estEnLecture ? "#132#" : "#131#")) lecturePause();

        const int minutes = static_cast<int>(valeurSliderProgression) / 60;
        const int secondes = static_cast<int>(valeurSliderProgression) % 60;
        const int dureeMinutes = static_cast<int>(duree) / 60;
        const int dureeSecondes = static_cast<int>(duree) % 60;
        GuiLabel(rectangles[5], TextFormat("%02d:%02d / %02d:%02d", minutes, secondes, dureeMinutes, dureeSecondes));

        barreProgression(enGlissement, etaitEnLecture, delaiRecherche);

        if (GuiButton(rectangles[7], estMuet ? "#128#" : "#122#")) son();

        GuiLabel(rectangles[8], TextFormat("%d%%", static_cast<int>(valeurSliderSon)));

        barreVolume();

        // Écran de chargement
        if (generationEnCours) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.5f));

            const char* texteChargement = "Génération en cours...";
            int tailleTexte = MeasureText(texteChargement, 20);
            DrawText(texteChargement, GetScreenWidth() / 2 - tailleTexte / 2, GetScreenHeight() / 2, 20, WHITE);

            rotationChargement += 4.0f;
            Rectangle rectChargement = {
                (float)GetScreenWidth() / 2,
                (float)GetScreenHeight() / 2 - 40,
                20, 20
            };
            DrawRectanglePro(rectChargement, {10, 10}, rotationChargement, WHITE);
        }

        EndDrawing();
    }
}

void AffichageSynchrone::setTailleTampon(const int taille) {
    tailleTampon = taille;
    chargerVideo();
}

void AffichageSynchrone::setVolume(const float volume) {
    valeurSliderSon = volume;
    if (IsMediaValid(video)) SetAudioStreamVolume(video.audioStream, valeurSliderSon / 100.0f);
}
