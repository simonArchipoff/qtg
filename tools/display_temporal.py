import numpy as np
import matplotlib.pyplot as plt
import argparse

def main():
    parser = argparse.ArgumentParser(description="Visualisation temporelle + fréquentielle d'un fichier .npy")
    parser.add_argument("fichier", type=str, help="Chemin du fichier .npy")
    parser.add_argument("--sr", type=float, default=None, help="Fréquence d'échantillonnage (Hz) pour les axes temps et fréquence")
    args = parser.parse_args()

    data = np.load(args.fichier)

    # Construction du signal (réel ou complexe)
    if data.ndim == 2 and data.shape[1] >= 2:
        signal = data[:, 0] + 1j * data[:, 1]
        is_complex = True
    elif np.iscomplexobj(data):
        signal = data
        is_complex = True
    else:
        if data.ndim == 2:
            signal = data[:, 0]
        else:
            signal = data
        is_complex = False

    N = len(signal)
    fs = args.sr

    # Création des subplots
    fig, (ax_time, ax_freq) = plt.subplots(2, 1, figsize=(12, 8))

    # ---------- Domaine temporel ----------
    if fs is not None:
        t = np.arange(N) / fs
        xlabel_time = "Temps (s)"
    else:
        t = np.arange(N)
        xlabel_time = "Échantillons"

    if is_complex:
        ax_time.plot(t, signal.real, label='Partie réelle', alpha=0.7)
        ax_time.plot(t, signal.imag, label='Partie imaginaire', alpha=0.7)
        ax_time.plot(t, np.abs(signal),label="amplitude", alpha=0.7)
        ax_time.legend()
    else:
        ax_time.plot(t, signal)
    ax_time.set_title(f"Temporel - {args.fichier}")
    ax_time.set_xlabel(xlabel_time)
    ax_time.set_ylabel("Amplitude")
    ax_time.grid(True)

    # ---------- Domaine fréquentiel ----------
    fft_vals = np.fft.fft(signal)
    fft_shifted = np.fft.fftshift(fft_vals)
    magnitude = np.abs(fft_shifted)

    if fs is not None:
        freqs = np.fft.fftfreq(N, d=1/fs)
        freqs_shifted = np.fft.fftshift(freqs)
        xlabel_freq = "Fréquence (Hz)"
    else:
        freqs_shifted = np.fft.fftshift(np.arange(N))
        xlabel_freq = "Bins (fréquence normalisée)"

    ax_freq.plot(freqs_shifted, magnitude)
    ax_freq.set_title("Spectre (magnitude)")
    ax_freq.set_xlabel(xlabel_freq)
    ax_freq.set_ylabel("Magnitude")
    ax_freq.grid(True)

    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()
