#ifndef AFFICHAGE_SYNCHRONE_H
#define AFFICHAGE_SYNCHRONE_H

#include "raylib.h"
#include "../raymedia/raymedia.h"
#include <string>
#include <vector>

using namespace std;

class AffichageSynchrone {
    MediaStream video{};
    string cheminVideoComplexe = "sortie_synchro.mp4";

    float duree{};
    int tailleTampon = 500;

    int indexListeVides = 0;
    int etatListeVideos = 0;

    float valeurSliderProgression = 0.0f;

    float valeurSliderSon = 100.0f;
    float valeurSliderSonPrecedent{};
    bool estMuet = false;

    string listeVideos;
    vector<string> videoFiles;
    vector<bool> videoSelected;
    Vector2 scrollPosition = {0, 0};

    const char *BOUTTON_GENERER = "Générer";

    const char *ZONE_VIDEOS = "";

    const char *BOUTTON_PLAY_PAUSE = "#131#";
    const char *HORODATAGE = "";
    const char *SLIDER_PROGRESSION = "";

    const char *BOUTTON_SON = "#122#";
    const char *VOLUME = "";
    const char *SLIDER_SON = "";

    const char *MSG_ERREUR = "Erreur: Média non chargé";

    Rectangle rectangles[10]{};

    void chargerListeVideos();

    void chargerVideo();

    void generer();

    void playPause() const;

    void son();

    void sliderProgression(bool &enGlissement, bool &etaitEnLecture, float &delaiRecherche);

    void sliderVolume();

    void afficherVideo() const;

public:
    explicit AffichageSynchrone();

    ~AffichageSynchrone();

    void executer();

    void setTailleTampon(int taille);

    void setVolume(float volume);
};

#endif
