/**
 * @file SynchroniseurMultiVideo.cpp
 * @brief Implémentation de la classe SynchroniseurMultiVideo.
 *
 * Ce fichier contient le code source des méthodes de la classe SynchroniseurMultiVideo,
 * incluant l'extraction audio via FFmpeg, le chargement des données brutes,
 * le calcul de corrélation croisée et la génération de la vidéo finale.
 */

#include "SynchroniseurMultiVideo.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <cmath>

using namespace std;

SynchroniseurMultiVideo::~SynchroniseurMultiVideo() {
    remove(TEMP_AUDIO_REF.c_str());
    remove(TEMP_AUDIO_CIBLE.c_str());
}

void SynchroniseurMultiVideo::configurerAnalyse(double duree, double plage, int pas) {
    if (duree > 0) dureeAnalyse = duree;
    if (plage > 0) plageRechercheMax = plage;
    if (pas > 0) pasDePrecision = pas;
}

void SynchroniseurMultiVideo::extraireAudio(const string &fichierVideo, const string &fichierAudioSortie) const {
    stringstream cmd;

    // Construction de la commande FFmpeg pour extraire l'audio.
    // -y: Écrase le fichier de sortie s'il existe déjà.
    // -hide_banner: Supprime l'affichage de la bannière FFmpeg.
    // -loglevel error: N'affiche que les messages d'erreur graves.
    // -i: Spécifie le fichier vidéo d'entrée.
    // -vn: Ignore le flux vidéo (évite le décodage inutile).
    // -f f32le: Définit le format de sortie de l'audio en float 32-bit little-endian.
    // -ac 1: Convertit l'audio en mono.
    // -ar FREQUENCE_ECHANTILLONNAGE: Définit la fréquence d'échantillonnage de l'audio.
    // -t dureeAnalyse: Traite seulement les X premières secondes de la vidéo.

    cmd << "ffmpeg -y -hide_banner -loglevel error "
            << "-i \"" << fichierVideo << "\" "
            << "-vn "
            << "-f f32le -ac 1 -ar " << FREQUENCE_ECHANTILLONNAGE << " -t " << dureeAnalyse << " "
            << "\"" << fichierAudioSortie << "\"";

    // Exécute la commande FFmpeg.
    if (system(cmd.str().c_str()) != 0) throw runtime_error("Impossible d'extraire l'audio de : " + fichierVideo);
}

vector<float> SynchroniseurMultiVideo::chargerAudioBrut(const string &nomFichier) {
    // Ouvre le fichier audio en mode binaire et se positionne à la fin pour obtenir la taille
    ifstream fichier(nomFichier, ios::binary | ios::ate);

    vector<float> echantillons;

    if (!fichier.is_open()) throw runtime_error("Impossible d'ouvrir le fichier : " + nomFichier);

    // Récupère la taille du fichier
    streamsize taille = fichier.tellg();
    // Repositionne le curseur au début du fichier
    fichier.seekg(0, ios::beg);
    // Calcule le nombre d'éléments que le fichier contient
    int nbElements = taille / sizeof(float);

    // Redimensionne le vecteur pour accueillir tous les échantillons
    echantillons.resize(nbElements);

    // Si le fichier n'est pas vide, lit les données et remplit le vecteur "echantillons"
    if (nbElements > 0) {
        fichier.read(reinterpret_cast<char *>(echantillons.data()), taille);
        if (!fichier) throw runtime_error("Erreur de lecture du fichier : " + nomFichier);
    }

    return echantillons;
}

double SynchroniseurMultiVideo::calculerDecalage(const vector<float> &ref, const vector<float> &cible) const {
    try {
        if (ref.empty() || cible.empty()) return 0.0;

        // Détermine la taille minimale des deux vecteurs pour éviter les débordements
        const int n = min(ref.size(), cible.size());

        double maxCorr = -1.0;
        int meilleurDecalage = 0;

        // Définit la plage de recherche pour le décalage en échantillons
        const int plageRecherche = static_cast<int>(FREQUENCE_ECHANTILLONNAGE * plageRechercheMax);

        // Boucle de corrélation croisée
        for (int retard = -plageRecherche; retard < plageRecherche; retard += 20) {
            // On ne vérifie pas chaque échantillon, on saute de pasDePrecision en pasDePrecision pour aller plus vite
            const int pas = pasDePrecision;

            // On ne compare que le tiers central
            const int debutScan = n / 3;
            const int finScan = 2 * n / 3;

            double corrActuelle = 0;

            for (int i = debutScan; i < finScan; i += pas) {
                // Calcule l'indice correspondant dans le vecteur cible en appliquant le retard
                int j = i + retard;

                if (j >= 0 && j < cible.size()) {
                    // Accumule le produit des échantillons pour la corrélation
                    corrActuelle += ref[i] * cible[j];
                }
            }

            // On garde le meilleur score
            // Plus la corrélation est élevée, mieux c'est
            if (corrActuelle > maxCorr) {
                maxCorr = corrActuelle;
                meilleurDecalage = retard;
            }
        }

        // Conversion échantillons -> secondes
        return static_cast<double>(meilleurDecalage) / FREQUENCE_ECHANTILLONNAGE;
    } catch (const exception &e) {
        cerr << "[Erreur Calcul] " << e.what() << endl;
        return 0.0;
    }
}

bool SynchroniseurMultiVideo::genererVideo(const vector<InfoVideo> &listeVideos, const string &fichierSortie,
                                           const string &fichierAudioRef) const {
    cout << "[3/3] Génération de la vidéo finale..." << endl;

    stringstream cmd;

    // Construction de la commande FFmpeg.
    // -y : Écrase le fichier de sortie s'il existe déjà.
    // -hide_banner : Masque la bannière de copyright/version de FFmpeg au démarrage.
    // -loglevel warning : Affiche uniquement les avertissements et erreurs (réduit le bruit dans la console).
    cmd << "ffmpeg -y -hide_banner -loglevel warning ";

    // Si un fichier audio de référence externe est fourni, on l'ajoute comme première entrée (index 0).
    if (!fichierAudioRef.empty()) {
        cmd << "-i \"" << fichierAudioRef << "\" ";
    }

    // Ajout de chaque vidéo source à la commande.
    for (const auto &vid: listeVideos) {
        // Si un retard (décalage) est détecté pour cette vidéo, on utilise l'option -ss (seek start).
        // Cela permet de caler la vidéo au bon moment par rapport à l'audio de référence.
        if (vid.retardSecondes > 0) {
            cmd << "-ss " << vid.retardSecondes << " ";
        }
        // Ajout du fichier vidéo comme entrée.
        cmd << "-i \"" << vid.chemin << "\" ";
    }

    // Début de la définition du filtre complexe (-filter_complex).
    // Ce filtre va permettre de redimensionner et d'assembler les vidéos en une grille.
    cmd << "-filter_complex \"";

    // Détermination de l'index de départ des vidéos dans la liste des entrées FFmpeg.
    // Si un audio externe est utilisé (index 0), la première vidéo est à l'index 1.
    // Sinon, la première vidéo est à l'index 0.
    int indexVideoStart = fichierAudioRef.empty() ? 0 : 1;
    int nbVideos = listeVideos.size();

    // Calcul des dimensions de la grille (lignes x colonnes).
    // On cherche à obtenir une grille la plus carrée possible (ex: 4 vidéos -> 2x2, 6 vidéos -> 3x2).
    int cols = ceil(sqrt(nbVideos));
    int rows = ceil((double) nbVideos / cols);

    // Étape 1 : Redimensionnement de chaque vidéo.
    // Chaque vidéo est redimensionnée à la taille cible (LARGEUR_CIBLE x HAUTEUR_CIBLE).
    // On attribue une étiquette temporaire [v0], [v1], etc. à chaque sortie redimensionnée.
    for (int i = 0; i < nbVideos; ++i) {
        cmd << "[" << (i + indexVideoStart) << ":v]scale=" << LARGEUR_CIBLE << ":" << HAUTEUR_CIBLE << "[v" << i <<
                "];";
    }

    // Étape 2 : Assemblage des vidéos avec le filtre xstack.
    // On liste d'abord toutes les étiquettes vidéo à assembler : [v0][v1]...
    for (int i = 0; i < nbVideos; ++i) {
        cmd << "[v" << i << "]";
    }

    // Configuration du filtre xstack.
    // inputs=N : nombre d'entrées vidéo.
    // fill=black : couleur de fond pour les espaces vides (si la grille n'est pas complète).
    // layout : définition de la position (x,y) de chaque vidéo.
    cmd << "xstack=inputs=" << nbVideos << ":fill=black:layout=";

    // Génération dynamique de la disposition (layout).
    // Le format est : x0_y0|x1_y1|x2_y2|...
    // w0 représente la largeur de la première vidéo (toutes identiques ici), h0 la hauteur.
    for (int i = 0; i < nbVideos; ++i) {
        int r = i / cols; // Indice de ligne
        int c = i % cols; // Indice de colonne

        // Séparateur entre les définitions de position
        if (i > 0) cmd << "|";

        // Calcul de la position X (horizontale)
        if (c == 0) {
            cmd << "0"; // Première colonne : x = 0
        } else {
            // Colonnes suivantes : x = somme des largeurs précédentes
            // Ici simplifié car toutes les largeurs sont w0.
            // Ex: pour la 3ème colonne (c=2), on écrit w0+w0.
            for (int k = 0; k < c; ++k) {
                if (k > 0) cmd << "+";
                cmd << "w0";
            }
        }

        cmd << "_"; // Séparateur x_y

        // Calcul de la position Y (verticale)
        if (r == 0) {
            cmd << "0"; // Première ligne : y = 0
        } else {
            // Lignes suivantes : y = somme des hauteurs précédentes
            // Ex: pour la 2ème ligne (r=1), on écrit h0.
            for (int k = 0; k < r; ++k) {
                if (k > 0) cmd << "+";
                cmd << "h0";
            }
        }
    }

    // Étiquette de sortie du filtre complexe
    cmd << "[vout]\" ";

    // Mapping des flux pour le fichier de sortie.
    // -map "[vout]" : Utilise la vidéo générée par le filtre complexe.
    // -map 0:a : Utilise l'audio de la première entrée (fichier de référence ou première vidéo).
    cmd << "-map \"[vout]\" -map 0:a ";

    // Options d'encodage vidéo :

    cmd << "-c:v libx264" // Encodeur H.264.
            << "-profile:v baseline" // Profil simple pour la compatibilité.
            << "-tune zerolatency" // Optimisation pour réduire la latence.
            << "-g 60" // 60 fps.
            << "-pix_fmt yuv420p" // Format de pixel standard pour la compatibilité.
            << "-preset fast" // (ultrafast, superfast, veryfast, fast, medium, slow...)
            << "-movflags +faststart \"" // Déplace les métadonnées au début du fichier.
            << fichierSortie << "\"";

    // Exécution de la commande système.
    if (system(cmd.str().c_str()) == 0) {
        cout << "[Succès] Fichier généré : " << fichierSortie << endl;
        return true;
    }

    throw runtime_error("Une erreur est survenue lors de l'encodage FFmpeg.");
}

bool SynchroniseurMultiVideo::genererVideoSynchronisee(const vector<string> &fichiersEntree,
                                                       const string &fichierSortie) const {
    try {
        if (fichiersEntree.size() < 2) {
            throw runtime_error("Il faut fournir au moins 2 fichiers vidéos.");
        }

        cout << "Traitement de " << fichiersEntree.size() << " vidéos" << endl;

        cout << "[1/3] Analyse de la référence vidéo..." << endl;

        try {
            extraireAudio(fichiersEntree[0], TEMP_AUDIO_REF);
        } catch (const exception &e) {
            throw runtime_error(string("Erreur référence : ") + e.what());
        }

        vector<float> audioRef = chargerAudioBrut(TEMP_AUDIO_REF);

        if (audioRef.empty()) {
            throw runtime_error("Fichier audio référence vide ou illisible.");
        }

        vector<InfoVideo> listeVideos;

        // Ajoute la vidéo de référence à la liste avec un décalage de 0.
        listeVideos.push_back({fichiersEntree[0], 0.0});

        // Boucle sur les vidéos cibles (à partir de la deuxième).
        for (int i = 1; i < fichiersEntree.size(); ++i) {
            cout << "[2/3] Analyse vidéo " << i + 1 << " : " << flush;

            try {
                // Extrait l'audio de la vidéo cible actuelle dans un fichier temporaire.
                extraireAudio(fichiersEntree[i], TEMP_AUDIO_CIBLE);

                vector<float> audioCible = chargerAudioBrut(TEMP_AUDIO_CIBLE);

                double decalage = calculerDecalage(audioRef, audioCible);

                // Ajoute la vidéo à la liste avec son décalage.
                listeVideos.push_back({fichiersEntree[i], decalage});

                cout << "OK (Retard : " << fixed << setprecision(3) << decalage << "s)" << endl;
            } catch (const exception &e) {
                cout << "Échec (" << e.what() << ") - Vidéo ignorée" << endl;
            }
        }

        return genererVideo(listeVideos, fichierSortie);
    } catch (const exception &e) {
        cerr << "[Erreur] " << e.what() << endl;
        return false;
    }
}

bool SynchroniseurMultiVideo::genererVideoSynchronisee(const string &fichierAudioRef,
                                                       const vector<string> &fichiersVideo,
                                                       const string &fichierSortie) const {
    try {
        if (fichiersVideo.empty()) {
            throw runtime_error("Il faut fournir au moins 1 fichier vidéo.");
        }

        cout << "[1/3] Analyse de la référence audio..." << endl;

        try {
            extraireAudio(fichierAudioRef, TEMP_AUDIO_REF);
        } catch (const exception &e) {
            throw runtime_error(string("Erreur référence audio : ") + e.what());
        }

        vector<float> audioRef = chargerAudioBrut(TEMP_AUDIO_REF);

        if (audioRef.empty()) {
            throw runtime_error("Fichier audio référence vide ou illisible.");
        }

        vector<InfoVideo> listeVideos;

        // Boucle sur les vidéos cibles.
        for (int i = 0; i < fichiersVideo.size(); ++i) {
            cout << "[2/3] Analyse vidéo " << i + 1 << " : " << flush;

            try {
                // Extrait l'audio de la vidéo cible actuelle dans un fichier temporaire.
                extraireAudio(fichiersVideo[i], TEMP_AUDIO_CIBLE);

                vector<float> audioCible = chargerAudioBrut(TEMP_AUDIO_CIBLE);

                double decalage = calculerDecalage(audioRef, audioCible);

                // Ajoute la vidéo à la liste avec son décalage.
                listeVideos.push_back({fichiersVideo[i], decalage});

                cout << "OK (Retard : " << fixed << setprecision(3) << decalage << "s)" << endl;
            } catch (const exception &e) {
                cout << "Échec (" << e.what() << ") - Vidéo ignorée" << endl;
            }
        }

        return genererVideo(listeVideos, fichierSortie, fichierAudioRef);
    } catch (const exception &e) {
        cerr << "[Erreur] " << e.what() << endl;
        return false;
    }
}
