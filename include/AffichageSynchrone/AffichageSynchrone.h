#pragma once

#include "raylib.h"
#include <vlc/vlc.h>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>

// Forward declarations pour les callbacks C de VLC
static void *verrouiller(void *donnees, void **p_pixels);
static void deverrouiller(void *donnees);
static void afficherPixels(const void *donnees);
static unsigned configurerVideo(void **donnees, char *chroma, const unsigned *largeur, const unsigned *hauteur, unsigned *pas, unsigned *lignes);
static void nettoyerVideo(void *donnees);

/**
 * @class AffichageSynchrone
 * @brief Gère l'affichage synchronisé de vidéos et l'interface utilisateur associée.
 *
 * Cette classe utilise Raylib pour l'interface graphique et libVLC pour la lecture vidéo.
 * Elle permet de sélectionner des vidéos, de générer une vidéo synchronisée via un traitement externe,
 * et de lire le résultat.
 */
class AffichageSynchrone {
public:
    /**
     * @brief Constructeur de la classe AffichageSynchrone.
     *
     * Initialise la fenêtre Raylib, le périphérique audio, l'instance VLC et charge la liste des vidéos disponibles.
     */
    explicit AffichageSynchrone();

    /**
     * @brief Destructeur de la classe AffichageSynchrone.
     *
     * Libère les ressources VLC, ferme la fenêtre, arrête le thread de génération s'il est actif
     * et nettoie les textures.
     */
    ~AffichageSynchrone();

    /**
     * @brief Exécute la boucle principale de l'application.
     *
     * Gère les événements d'entrée, la mise à jour de l'affichage vidéo et l'interface utilisateur (GUI).
     * Cette méthode bloque jusqu'à la fermeture de la fenêtre.
     */
    void executer();

    /**
     * @brief Définit le volume de la lecture vidéo.
     *
     * @param volume Le volume à définir (valeur entre 0.0 et 100.0).
     */
    void setVolume(float volume);

private:
    // --- VLC & Gestion Vidéo ---
    libvlc_instance_t *instanceVLC = nullptr;
    libvlc_media_player_t *lecteurVLC = nullptr;
    Texture2D textureVideo{};
    std::vector<unsigned char> pixelsVideo;
    std::mutex mutexImage; ///< Mutex pour protéger l'accès aux pixels de la vidéo entre le thread VLC et le thread principal.
    unsigned int largeurVideo = 0;
    unsigned int hauteurVideo = 0;
    std::string cheminVideoComplexe = "sortie_synchro.mp4";
    float duree{};

    // --- Callbacks VLC (Amis) ---
    friend void *verrouiller(void *donnees, void **p_pixels);
    friend void deverrouiller(void *donnees);
    friend void afficherPixels(const void *donnees);
    friend unsigned configurerVideo(void **donnees, char *chroma, const unsigned *largeur, const unsigned *hauteur, unsigned *pas, unsigned *lignes);
    friend void nettoyerVideo(void *donnees);

    // --- État de l'Interface Utilisateur ---
    float valeurSliderProgression = 0.0f;
    float valeurSliderSon = 100.0f;
    float valeurSliderSonPrecedent{};
    bool estMuet = false;

    // --- Gestion de la Génération ---
    std::atomic<bool> videoGeneree{false};
    std::atomic<bool> generationEnCours{false};
    std::atomic<bool> framePrete{false};
    std::atomic<bool> textureDoitEtreRedimensionnee{false};
    std::thread threadGeneration;

    // --- Gestion des Fichiers ---
    std::vector<std::string> fichiersVideo;
    std::vector<bool> videosSelectionnees;
    std::vector<int> ordreSelection;
    Vector2 positionDefilement = {0, 0};

    // --- Constantes UI ---
    const char *TEXTE_BOUTON_GENERER = "Générer";
    const char *TEXTE_BOUTON_OUVRIR_DOSSIER = "Ouvrir dossier";

    // --- Layout ---
    Rectangle zones[11]{};

    // --- Méthodes Internes ---
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
};
