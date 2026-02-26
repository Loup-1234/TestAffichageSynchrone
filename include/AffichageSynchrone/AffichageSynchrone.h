#pragma once

#include "raylib.h"
#include <vlc/vlc.h>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>

static void *verrouiller(void *donnees, void **p_pixels);
static void deverrouiller(void *donnees);
static void afficherPixels(const void *donnees);

class AffichageSynchrone {
    libvlc_instance_t *instanceVLC = nullptr;
    libvlc_media_player_t *lecteurVLC = nullptr;
    Texture2D textureVideo{};
    std::vector<unsigned char> pixelsVideo;
    std::mutex mutexImage;
    unsigned int largeurVideo = 0;
    unsigned int hauteurVideo = 0;

    friend void *verrouiller(void *donnees, void **p_pixels);
    friend void deverrouiller(void *donnees);
    friend void afficherPixels(const void *donnees);

    std::string cheminVideoComplexe = "sortie_synchro.mp4";

    float duree{};
    int tailleTampon = 500;

    int indexListeVides = 0;
    int etatListeVideos = 0;

    float valeurSliderProgression = 0.0f;

    float valeurSliderSon = 100.0f;
    float valeurSliderSonPrecedent{};
    bool estMuet = false;

    std::atomic<bool> videoGeneree{false};
    std::atomic<bool> generationEnCours{false};
    std::atomic<bool> framePrete{false};
    std::thread threadGeneration;

    std::string listeVideos;
    std::vector<std::string> fichiersVideo;
    std::vector<bool> videosSelectionnees;
    std::vector<int> ordreSelection;
    Vector2 positionDefilement = {0, 0};

    const char *TEXTE_BOUTON_GENERER = "Générer";
    const char *TEXTE_BOUTON_OUVRIR_DOSSIER = "Ouvrir dossier";

    const char *ZONE_VIDEOS = "";

    const char *TEXTE_BOUTON_LECTURE_PAUSE = "#131#";
    const char *HORODATAGE = "";
    const char *BARRE_PROGRESSION = "";

    const char *TEXTE_BOUTON_SON = "#122#";
    const char *VOLUME = "";
    const char *BARRE_SON = "";

    const char *MSG_ERREUR = "Erreur: Média non chargé";

    Rectangle zones[11]{};

    void chargerListeVideos();

    void chargerVideo();

    void generer();

    static void ouvrirDossierVideos();

    void lecturePause() const;

    void son();

    void barreProgression(bool &enGlissement, bool &etaitEnLecture, float &delaiRecherche);

    void barreVolume();

    void afficherVideo();

    void afficherListeFichiers();

    void miseAJourDisposition();

public:
    explicit AffichageSynchrone();

    ~AffichageSynchrone();

    void executer();

    void setTailleTampon(int taille);

    void setVolume(float volume);
};
