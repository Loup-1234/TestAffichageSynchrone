#ifndef AFFICHAGE_SYNCHRONE_H
#define AFFICHAGE_SYNCHRONE_H

#include "raylib.h"
#include "../raymedia/raymedia.h"
#include <string>
#include <vector>
#include <atomic>

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

    atomic<bool> videoGeneree{false};
    atomic<bool> generationEnCours{false};

    string listeVideos;
    vector<string> fichiersVideo;
    vector<bool> videosSelectionnees;
    vector<int> ordreSelection;
    Vector2 positionDefilement = {0, 0};

    const char *BOUTON_GENERER = "Générer";

    const char *ZONE_VIDEOS = "";

    const char *BOUTON_LECTURE_PAUSE = "#131#";
    const char *HORODATAGE = "";
    const char *BARRE_PROGRESSION = "";

    const char *BOUTON_SON = "#122#";
    const char *VOLUME = "";
    const char *BARRE_SON = "";

    const char *MSG_ERREUR = "Erreur: Média non chargé";

    Rectangle rectangles[10]{};

    void chargerListeVideos();

    void chargerVideo();

    void generer();

    void lecturePause() const;

    void son();

    void barreProgression(bool &enGlissement, bool &etaitEnLecture, float &delaiRecherche);

    void barreVolume();

    void afficherVideo() const;

    void afficherListeFichiers();

    void miseAJourDisposition();

public:
    explicit AffichageSynchrone();

    ~AffichageSynchrone();

    void executer();

    void setTailleTampon(int taille);

    void setVolume(float volume);
};

#endif
