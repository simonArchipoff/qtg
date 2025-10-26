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

# --- Visualisation ---
# 1. Tracer quelques sinusoïdes dans le temps
plt.figure(figsize=(10, 6))
t = np.arange(nframes) / 48000.0  # échelle en secondes
for i in range( min(5,nbfreq)):
    plt.plot(t[:48000], data[i, :48000], label=f"freq {i}")
plt.xlabel("Temps (s)")
plt.ylabel("Amplitude")
plt.title("Extraits des 5 premières sinusoïdes")
plt.legend()
plt.tight_layout()

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