#define RAYGUI_IMPLEMENTATION

#include "AffichageSynchrone/AffichageSynchrone.h"
#include "SynchroniseurMultiVideo/SynchroniseurMultiVideo.h"
#include "raygui.h"
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cstdlib>

using namespace std;
namespace fs = filesystem;

// --- Callbacks VLC ---

static void *verrouiller(void *donnees, void **p_pixels) {
    auto *app = static_cast<AffichageSynchrone *>(donnees);
    app->mutexImage.lock();
    *p_pixels = app->pixelsVideo.data();
    return nullptr;
}

static void deverrouiller(void *donnees) {
    auto *app = static_cast<AffichageSynchrone *>(donnees);
    app->mutexImage.unlock();
    app->framePrete = true;
}

static unsigned configurerVideo(void **donnees, char *chroma, const unsigned *largeur, const unsigned *hauteur, unsigned *pas, unsigned *lignes) {
    auto *app = static_cast<AffichageSynchrone *>(*donnees);
    app->mutexImage.lock();
    app->largeurVideo = *largeur;
    app->hauteurVideo = *hauteur;
    // Redimensionne le buffer de pixels pour correspondre à la nouvelle taille vidéo
    app->pixelsVideo.resize(app->largeurVideo * app->hauteurVideo * 4);
    memcpy(chroma, "RGBA", 4);
    *pas = app->largeurVideo * 4;
    *lignes = app->hauteurVideo;
    app->textureDoitEtreRedimensionnee = true;
    app->mutexImage.unlock();
    return 1;
}

// --- Implémentation AffichageSynchrone ---

AffichageSynchrone::AffichageSynchrone() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 450, "Test affichage synchrone");
    SetWindowMinSize(800, 450);
    InitAudioDevice();
    chargerListeVideos();
    SetTargetFPS(30);

    // Initialisation de VLC
    instanceVLC = libvlc_new(0, nullptr);
    lecteurVLC = libvlc_media_player_new(instanceVLC);

    textureVideo = {0};

    miseAJourDisposition();

    if (fs::exists(cheminVideoComplexe)) {
        chargerVideo();
    }
}

AffichageSynchrone::~AffichageSynchrone() {
    if (threadGeneration.joinable()) {
        threadGeneration.join();
    }
    if (lecteurVLC) {
        libvlc_media_player_stop(lecteurVLC);
        libvlc_media_player_release(lecteurVLC);
    }
    if (instanceVLC) {
        libvlc_release(instanceVLC);
    }
    UnloadTexture(textureVideo);
    CloseAudioDevice();
    CloseWindow();
}

void AffichageSynchrone::miseAJourDisposition() {
    const auto largeurEcran = static_cast<float>(GetScreenWidth());
    const auto hauteurEcran = static_cast<float>(GetScreenHeight());

    // Définition des zones de l'interface (liste, boutons, vidéo, contrôles)
    zones[0] = {0, 48, 150, hauteurEcran - 96}; // Vue en liste
    zones[1] = {0, hauteurEcran - 48, 150, 48}; // Bouton Générer
    zones[2] = {150, 0, largeurEcran - 150, hauteurEcran - 72}; // Zone vidéo
    zones[3] = {150, hauteurEcran - 72, largeurEcran - 150, 2}; // Ligne de délimitation
    zones[4] = {158, hauteurEcran - 40, 32, 32}; // Bouton Lecture/Pause
    zones[5] = {198, hauteurEcran - 72, largeurEcran - 382, 32}; // Étiquette d'horodatage
    zones[6] = {198, hauteurEcran - 40, largeurEcran - 382, 32}; // Curseur de progression
    zones[7] = {largeurEcran - 176, hauteurEcran - 40, 32, 32}; // Bouton Muet
    zones[8] = {largeurEcran - 136, hauteurEcran - 72, 128, 32}; // Étiquette de volume
    zones[9] = {largeurEcran - 136, hauteurEcran - 40, 128, 32}; // Curseur de volume
    zones[10] = {0, 0, 150, 48}; // Bouton Ouvrir dossier
}

void AffichageSynchrone::chargerListeVideos() {
    fichiersVideo.clear();
    videosSelectionnees.clear();
    ordreSelection.clear();

    string chemin = "videos";
    if (fs::exists(chemin) && fs::is_directory(chemin)) {
        for (const auto &entree: fs::directory_iterator(chemin)) {
            if (entree.is_regular_file()) {
                string ext = entree.path().extension().string();
                if (ext == ".mp4" || ext == ".mp3") {
                    fichiersVideo.push_back(entree.path().filename().string());
                    videosSelectionnees.push_back(false);
                }
            }
        }
    }
}

void AffichageSynchrone::chargerVideo() {
    libvlc_media_t *media = libvlc_media_new_path(instanceVLC, cheminVideoComplexe.c_str());
    if (!media) {
        duree = 0.0f;
        return;
    }

    libvlc_media_player_set_media(lecteurVLC, media);
    libvlc_media_release(media);

    // Configuration des callbacks pour le rendu vidéo dans une texture Raylib
    libvlc_video_set_callbacks(lecteurVLC, verrouiller, reinterpret_cast<libvlc_video_unlock_cb>(deverrouiller), nullptr, this);
    libvlc_video_set_format_callbacks(lecteurVLC, reinterpret_cast<libvlc_video_format_cb>(configurerVideo), nullptr);

    libvlc_media_player_play(lecteurVLC);

    // Attendre un peu pour que VLC initialise la vidéo
    this_thread::sleep_for(chrono::milliseconds(200));
    libvlc_media_player_set_pause(lecteurVLC, 1);

    const libvlc_time_t len = libvlc_media_player_get_length(lecteurVLC);
    duree = (len != -1) ? static_cast<float>(len) / 1000.0f : 0.0f;

    libvlc_audio_set_volume(lecteurVLC, static_cast<int>(valeurSliderSon));
}

void AffichageSynchrone::generer() {
    if (generationEnCours) return;

    vector<string> videos;
    const fs::path cheminBase = "videos";

    for (const int index : ordreSelection) {
        if (index >= 0 && index < fichiersVideo.size()) {
            videos.push_back((cheminBase / fichiersVideo[index]).string());
        }
    }

    if (videos.empty()) return;

    string extension = fs::path(videos[0]).extension().string();
    bool estMp3 = (extension == ".mp3");

    // Vérification du nombre de fichiers requis
    // Si le premier est un MP3, il faut au moins 3 fichiers (1 MP3 + 2 vidéos)
    // Sinon, il faut au moins 2 fichiers (2 vidéos)
    if ((estMp3 && videos.size() < 3) || (!estMp3 && videos.size() < 2)) {
        return;
    }

    libvlc_media_player_stop(lecteurVLC);
    duree = 0.0f;
    valeurSliderProgression = 0.0f;

    generationEnCours = true;

    if (threadGeneration.joinable()) {
        threadGeneration.join();
    }

    // Lancement de la génération dans un thread séparé pour ne pas bloquer l'UI
    threadGeneration = thread([this, videos, estMp3]() {
        SynchroniseurMultiVideo synchroniseur;
        synchroniseur.configurerAnalyse(60.0, 30.0, 100);

        bool succes = false;
        if (estMp3) {
            const vector<string> videosSeules(videos.begin() + 1, videos.end());
            succes = synchroniseur.genererVideoSynchronisee(videos[0], videosSeules, cheminVideoComplexe);
        } else {
            succes = synchroniseur.genererVideoSynchronisee(videos, cheminVideoComplexe);
        }

        if (succes) {
            videoGeneree = true;
        }
        generationEnCours = false;
    });
}

void AffichageSynchrone::ouvrirDossierVideos() {
    const string chemin = "videos";
    if (!fs::exists(chemin)) {
        fs::create_directory(chemin);
    }

    #ifdef _WIN32
        string commande = "explorer " + chemin;
    #elif __APPLE__
        string commande = "open " + chemin;
    #else
        string commande = "xdg-open " + chemin;
    #endif

    system(commande.c_str());
}

void AffichageSynchrone::lecturePause() const {
    if (libvlc_media_player_is_playing(lecteurVLC)) {
        libvlc_media_player_set_pause(lecteurVLC, 1);
    } else {
        libvlc_media_player_play(lecteurVLC);
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
    const bool estEnLecture = libvlc_media_player_is_playing(lecteurVLC);

    // Gestion du clic sur la barre de progression
    if (CheckCollisionPointRec(GetMousePosition(), zones[6])) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            enGlissement = true;
            etaitEnLecture = estEnLecture;
            libvlc_media_player_set_pause(lecteurVLC, 1);
            libvlc_audio_set_volume(lecteurVLC, 0);
        }
    }

    const float ancienneValeur = valeurSliderProgression;
    GuiSliderBar(zones[6], "", nullptr, &valeurSliderProgression, 0.0f, duree);

    if (enGlissement) {
        if (valeurSliderProgression != ancienneValeur) {
            libvlc_media_player_set_time(lecteurVLC, static_cast<libvlc_time_t>(valeurSliderProgression * 1000));
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || !IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            enGlissement = false;
            if (etaitEnLecture) {
                libvlc_media_player_play(lecteurVLC);
            }
            libvlc_audio_set_volume(lecteurVLC, static_cast<int>(valeurSliderSon));
            delaiRecherche = 0.2f;
        }
    }
}

void AffichageSynchrone::barreVolume() {
    float nouveauVolume = valeurSliderSon;
    GuiSliderBar(zones[9], "", nullptr, &nouveauVolume, 0.0f, 100.0f);
    if (nouveauVolume != valeurSliderSon) {
        valeurSliderSon = nouveauVolume;
        estMuet = (valeurSliderSon <= 0.0f);
        setVolume(valeurSliderSon);
    }
}

void AffichageSynchrone::afficherVideo() {
    // Mise à jour de la texture si une nouvelle frame est prête
    if (framePrete) {
        mutexImage.lock();
        if (largeurVideo > 0 && hauteurVideo > 0) {
            UpdateTexture(textureVideo, pixelsVideo.data());
        }
        mutexImage.unlock();
        framePrete = false;
    }

    if (largeurVideo == 0 || hauteurVideo == 0) return;

    // Calcul de l'échelle pour garder le ratio d'aspect
    const float echelleX = zones[2].width / static_cast<float>(largeurVideo);
    const float echelleY = zones[2].height / static_cast<float>(hauteurVideo);
    const float echelle = min(echelleX, echelleY);

    const float destLargeur = static_cast<float>(largeurVideo) * echelle;
    const float destHauteur = static_cast<float>(hauteurVideo) * echelle;
    const float destX = zones[2].x + (zones[2].width - destLargeur) / 2.0f;
    const float destY = zones[2].y + (zones[2].height - destHauteur) / 2.0f;

    const Rectangle source = {0.0f, 0.0f, static_cast<float>(largeurVideo), static_cast<float>(hauteurVideo)};
    const Rectangle dest = {destX, destY, destLargeur, destHauteur};

    DrawTexturePro(textureVideo, source, dest, {0,0}, 0.0f, WHITE);
}

void AffichageSynchrone::afficherListeFichiers() {
    Rectangle vue = {0};

    // Calcul de la hauteur du contenu
    float contentHeight = static_cast<float>(fichiersVideo.size()) * 30;

    // Utilisation de GuiScrollPanel
    GuiScrollPanel(zones[0], nullptr,
                   (Rectangle){0, 0, zones[0].width - 16, contentHeight},
                   &positionDefilement, &vue);

    BeginScissorMode(static_cast<int>(vue.x), static_cast<int>(vue.y), static_cast<int>(vue.width), static_cast<int>(vue.height));

    for (size_t i = 0; i < fichiersVideo.size(); ++i) {
        Rectangle itemRect = {
            zones[0].x + 10 + positionDefilement.x,
            zones[0].y + 10 + static_cast<float>(i) * 30 + positionDefilement.y,
            20, 20
        };

        // Vérifier si l'élément est visible avant de le dessiner (optimisation)
        if (itemRect.y + itemRect.height < vue.y || itemRect.y > vue.y + vue.height) {
            continue;
        }

        int ordre = 0;
        if (videosSelectionnees[i]) {
            auto it = ranges::find(ordreSelection, static_cast<int>(i));
            if (it != ordreSelection.end()) {
                ordre = static_cast<int>(distance(ordreSelection.begin(), it)) + 1;
            }
        }

        string etiquette = fichiersVideo[i];
        if (ordre > 0) {
            etiquette += " (" + to_string(ordre) + ")";
        }

        bool coche = videosSelectionnees[i];
        const bool etaitCoche = coche;
        GuiCheckBox(itemRect, etiquette.c_str(), &coche);

        if (coche != etaitCoche) {
            videosSelectionnees[i] = coche;
            if (coche) {
                ordreSelection.push_back(static_cast<int>(i));
            } else {
                auto it = ranges::find(ordreSelection, static_cast<int>(i));
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

        // Redimensionnement de la texture si la vidéo change de résolution
        if (textureDoitEtreRedimensionnee) {
            mutexImage.lock();
            if (textureVideo.id > 0) {
                UnloadTexture(textureVideo);
            }
            if (largeurVideo > 0 && hauteurVideo > 0) {
                Image img = GenImageColor(static_cast<int>(largeurVideo), static_cast<int>(hauteurVideo), BLACK);
                textureVideo = LoadTextureFromImage(img);
                UnloadImage(img);
            } else {
                textureVideo = {0};
            }
            textureDoitEtreRedimensionnee = false;
            mutexImage.unlock();
        }

        if (delaiRecherche > 0) delaiRecherche -= GetFrameTime();

        if (!enGlissement && delaiRecherche <= 0) {
            libvlc_time_t tempsActuel = libvlc_media_player_get_time(lecteurVLC);
            if (tempsActuel != -1) {
                valeurSliderProgression = static_cast<float>(tempsActuel) / 1000.0f;
            }
        }

        BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        if (duree > 0 && textureVideo.id > 0) afficherVideo();

        DrawRectangleRec(zones[3], GRAY);
        afficherListeFichiers();

        GuiSetState(generationEnCours ? STATE_DISABLED : STATE_NORMAL);
        if (GuiButton(zones[1], TEXTE_BOUTON_GENERER)) generer();
        if (GuiButton(zones[10], TEXTE_BOUTON_OUVRIR_DOSSIER)) ouvrirDossierVideos();
        GuiSetState(STATE_NORMAL);

        bool estEnLecture = libvlc_media_player_is_playing(lecteurVLC);
        if (GuiButton(zones[4], estEnLecture ? "#132#" : "#131#")) lecturePause();

        const int minutes = static_cast<int>(valeurSliderProgression) / 60;
        const int secondes = static_cast<int>(valeurSliderProgression) % 60;
        const int dureeMinutes = static_cast<int>(duree) / 60;
        const int dureeSecondes = static_cast<int>(duree) % 60;
        GuiLabel(zones[5], TextFormat("%02d:%02d / %02d:%02d", minutes, secondes, dureeMinutes, dureeSecondes));

        barreProgression(enGlissement, etaitEnLecture, delaiRecherche);

        if (GuiButton(zones[7], estMuet ? "#128#" : "#122#")) son();

        GuiLabel(zones[8], TextFormat("%d%%", static_cast<int>(valeurSliderSon)));
        barreVolume();

        if (generationEnCours) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.5f));
            const auto texteChargement = "Génération en cours...";
            const int tailleTexte = MeasureText(texteChargement, 20);
            DrawText(texteChargement, GetScreenWidth() / 2 - tailleTexte / 2, GetScreenHeight() / 2, 20, WHITE);
            rotationChargement += 4.0f;
            const Rectangle rectChargement = {static_cast<float>(GetScreenWidth()) / 2, static_cast<float>(GetScreenHeight()) / 2 - 40, 20, 20};
            DrawRectanglePro(rectChargement, {10, 10}, rotationChargement, WHITE);
        }

        EndDrawing();
    }
}

void AffichageSynchrone::setVolume(const float volume) {
    valeurSliderSon = volume;
    libvlc_audio_set_volume(lecteurVLC, static_cast<int>(valeurSliderSon));
}
