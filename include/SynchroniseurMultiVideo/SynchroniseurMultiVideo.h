#pragma once

#include <string>
#include <vector>

using namespace std;

/**
 * @class SynchroniseurMultiVideo
 * @brief Classe permettant de synchroniser plusieurs vidéos basées sur leur piste audio.
 *
 * Cette classe analyse les pistes audio de plusieurs fichiers vidéo pour déterminer
 * le décalage temporel entre elles et générer une vidéo synchronisée (par exemple, une vue mosaïque).
 */
class SynchroniseurMultiVideo {
    /**
     * @brief Fréquence d'échantillonnage audio utilisée pour l'analyse (en Hz).
     */
    const int FREQUENCE_ECHANTILLONNAGE = 40000;

    /**
     * @brief Durée de l'audio à extraire pour l'analyse (en secondes).
     */
    double dureeAnalyse = 60.0;

    /**
     * @brief Plage de recherche maximale pour le décalage (en secondes).
     */
    double plageRechercheMax = 30.0;

    /**
    * @brief Pas de précision pour l'analyse (plus petit = plus précis).
    */
    int pasDePrecision = 100;

    /**
     * @brief Hauteur cible pour le redimensionnement des vidéos (en pixels).
     */
    const int HAUTEUR_CIBLE = 480;

    /**
     * @brief Largeur cible pour le redimensionnement des vidéos (en pixels).
     */
    const int LARGEUR_CIBLE = 854;

    /**
     * @brief Nom du fichier temporaire pour l'audio de référence.
     */
    const string TEMP_AUDIO_REF = "temp_ref.raw";

    /**
     * @brief Nom du fichier temporaire pour l'audio cible.
     */
    const string TEMP_AUDIO_CIBLE = "temp_cible.raw";

    /**
     * @struct InfoVideo
     * @brief Structure stockant les informations relatives à une vidéo.
     */
    struct InfoVideo {
        string chemin;          /**< Chemin d'accès au fichier vidéo. */
        double retardSecondes;  /**< Retard calculé en secondes par rapport à la vidéo de référence. */
    };

    /**
     * @brief Extrait la piste audio d'un fichier vidéo vers un fichier brut.
     *
     * Utilise FFmpeg pour extraire l'audio, le convertir en mono, float 32-bit little-endian,
     * à la fréquence d'échantillonnage définie.
     *
     * @param fichierVideo Chemin du fichier vidéo source.
     * @param fichierAudioSortie Chemin du fichier audio de sortie (format brut).
     * @throws runtime_error Si l'extraction échoue.
     */
    void extraireAudio(const string &fichierVideo, const string &fichierAudioSortie) const;

    /**
     * @brief Charge des données audio brutes depuis un fichier.
     *
     * Lit un fichier binaire contenant des échantillons audio au format float.
     *
     * @param nomFichier Chemin du fichier audio brut.
     * @return Un vecteur de flottants représentant les échantillons audio.
     * @throws runtime_error Si le fichier ne peut pas être ouvert ou lu.
     */
    static vector<float> chargerAudioBrut(const string &nomFichier);

    /**
     * @brief Calcule le décalage temporel entre deux signaux audio.
     *
     * Utilise une méthode de corrélation croisée simplifiée pour estimer le décalage
     * temporel entre le signal de référence et le signal cible.
     *
     * @param ref Vecteur des échantillons de l'audio de référence.
     * @param cible Vecteur des échantillons de l'audio à synchroniser.
     * @return Le décalage en secondes (positif ou négatif). Retourne 0.0 en cas d'erreur.
     */
    double calculerDecalage(const vector<float> &ref, const vector<float> &cible) const;

    /**
     * @brief Exécute la commande FFmpeg pour générer la vidéo finale.
     *
     * @param listeVideos Liste des vidéos à assembler.
     * @param fichierSortie Chemin du fichier de sortie.
     * @param fichierAudioRef Chemin du fichier audio de référence (optionnel).
     * @return true si succès, false sinon.
     */
    bool genererVideo(const vector<InfoVideo> &listeVideos, const string &fichierSortie, const string &fichierAudioRef = "") const;

public:
    /**
     * @brief Destructeur de la classe SynchroniseurMultiVideo.
     *
     * Supprime les fichiers temporaires créés lors de l'analyse audio.
     */
    ~SynchroniseurMultiVideo();

    /**
     * @brief Configure les paramètres d'analyse.
     * @param duree Durée de l'audio à analyser (en secondes).
     * @param plage Plage de recherche maximale (en secondes).
     * @param pas Pas de précision (1 pour précision maximale).
     */
    void configurerAnalyse(double duree, double plage, int pas);

    /**
     * @brief Génère une vidéo synchronisée à partir de plusieurs fichiers d'entrée.
     *
     * Orchestre le processus complet : extraction audio, calcul des décalages,
     * et génération de la vidéo finale avec FFmpeg.
     *
     * @param fichiersEntree Liste des chemins des fichiers vidéo à synchroniser.
     * @param fichierSortie Chemin du fichier vidéo de sortie.
     * @return true si la génération a réussi, false sinon.
     */
    bool genererVideoSynchronisee(const vector<string> &fichiersEntree, const string &fichierSortie) const;

    /**
     * @brief Génère une vidéo synchronisée en utilisant un fichier audio externe comme référence.
     *
     * @param fichierAudioRef Chemin du fichier audio de référence.
     * @param fichiersVideo Liste des chemins des fichiers vidéo à synchroniser.
     * @param fichierSortie Chemin du fichier vidéo de sortie.
     * @return true si la génération a réussi, false sinon.
     */
    bool genererVideoSynchronisee(const string &fichierAudioRef, const vector<string> &fichiersVideo, const string &fichierSortie) const;
};
