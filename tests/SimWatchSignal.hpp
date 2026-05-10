#pragma once

#include <vector>
#include <deque>
#include <cmath>
#include <cstdint>

class WatchSimSignal {
public:
    /**
     * @param sr   Échantillonnage (Hz)
     * @param freq Fréquence des impulsions (Hz)
     */
    WatchSimSignal(int sr, int freq)
        : sr_(sr)
        , freq_(freq)
        , drift_ppm_(0)
        , correction_(0.0)
        , ticks_(0)
        , total_frames_(0)
    {
        // Longueur de l'impulsion en échantillons : sr/150 secondes
        double duration = static_cast<double>(sr) / 150.0;
        int num_samples = static_cast<int>(duration);
        if (num_samples < 1) num_samples = 1;
        impulse_len_ = num_samples;

        // Génération de l'impulsion (fenêtre de Hann * sin(2*pi*5000*t))
        impulse_.resize(impulse_len_);
        double step = (impulse_len_ > 1) ? duration / (impulse_len_ - 1) : 0.0;
        for (int i = 0; i < impulse_len_; ++i) {
            double t = i * step;
            double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (impulse_len_ - 1)));
            double value = window * std::sin(2.0 * M_PI * 10000.0 * t);
            impulse_[i] = static_cast<float>(value);
        }

        // Écart nominal entre deux impulsions (échantillons de silence)
        double period_samples = static_cast<double>(sr) / freq;
        impulse_delta_ = static_cast<int>(period_samples - impulse_len_);
        // Sécurité : ne pas descendre en dessous de 0
        if (impulse_delta_ < 0) impulse_delta_ = 0;
    }

    /** Définit la dérive en ppm (parties par million). */
    void setDrift(int drift_ppm) {
        drift_ppm_ = drift_ppm;
        correction_ = 0.0;
    }

    /** Retourne le nombre d'impulsions générées jusqu'à présent. */
    int getTicks() const {
        return ticks_;
    }

    /** Retourne la fréquence moyenne réelle des impulsions. */
    double getFrequency() const {
        if (total_frames_ == 0) return 0.0;
        return static_cast<double>(ticks_) / (static_cast<double>(total_frames_) / sr_);
    }

    /**
     * Récupère N échantillons du signal.
     * @param N Nombre d'échantillons demandés
     * @return Vecteur contenant les N échantillons (ou moins si fin ? ici toujours N)
     */
    std::vector<float> getSignal(int N) {
        // Remplir la file d'attente jusqu'à avoir au moins N échantillons
        while (audio_queue_.size() < static_cast<size_t>(N)) {
            fillQueue();
        }

        // Copier les N premiers échantillons
        std::vector<float> result(audio_queue_.begin(), audio_queue_.begin() + N);
        // Supprimer ces N échantillons de la file
        audio_queue_.erase(audio_queue_.begin(), audio_queue_.begin() + N);
        return result;
    }

private:
    void fillQueue() {
        // Nombre nominal d'échantillons par période (silence + impulsion)
        double nominal_period = static_cast<double>(impulse_delta_) + impulse_len_;

        // Ajout de la correction due à la dérive
        correction_ += nominal_period * drift_ppm_ / 1'000'000.0;
        int c = static_cast<int>(correction_);   // troncature vers zéro (comme int() en Python)
        correction_ -= static_cast<double>(c);

        // Nombre d'échantillons de silence à insérer avant l'impulsion
        int zeros_to_add = impulse_delta_ - c;
        if (zeros_to_add < 0) zeros_to_add = 0;   // Sécurité

        // Ajout du silence
        for (int i = 0; i < zeros_to_add; ++i) {
            audio_queue_.push_back(0.0f);
        }
        // Ajout de l'impulsion
        for (float s : impulse_) {
            audio_queue_.push_back(s);
        }

        // Mise à jour des compteurs
        ticks_++;
        total_frames_ += zeros_to_add + impulse_len_;
    }

    // Paramètres
    int sr_;
    int freq_;
    int impulse_len_;
    int impulse_delta_;
    int drift_ppm_;

    // État interne
    double correction_;
    int ticks_;
    std::int64_t total_frames_;

    // Forme d'onde de l'impulsion et file d'attente du signal
    std::vector<float> impulse_;
    std::deque<float> audio_queue_;
};
