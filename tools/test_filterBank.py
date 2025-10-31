import numpy as np
import matplotlib.pyplot as plt

# --- Charger le tableau ---
data = np.load("build/freqs.npy")
print("Shape:", data.shape)
print("dtype:", data.dtype)

nbfreq, nframes = data.shape
print(f"{nbfreq} fréquences, {nframes} échantillons")



# --- Statistiques globales ---
means = data.mean(axis=1)
stds = data.std(axis=1)
print("Amplitude moyenne par fréquence :", means)
print("Écart-type par fréquence :", stds)


# 2. Spectre moyen pour vérifier les fréquences
plt.figure(figsize=(10, 6))
fft_len = nframes
freqs = np.fft.rfftfreq(fft_len, 1/48000)
for i in range(nbfreq):
    spec = np.abs(np.fft.rfft(data[i, :fft_len]))
    plt.plot(freqs, spec, label=f"freq {i}")
plt.xlabel("Fréquence (Hz)")
plt.ylabel("Amplitude (FFT partielle)")
plt.title("Spectres des 5 premières sinusoïdes")
plt.legend()
plt.tight_layout()

plt.show()