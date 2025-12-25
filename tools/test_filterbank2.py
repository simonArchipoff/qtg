 


import numpy as np
import matplotlib.pyplot as plt

# =========================
# Paramètres
# =========================
Fs = 8 #48000 / (25*128) # fréquence d'échantillonnage (à adapter)
print(f"{Fs=}")
# =========================
# Chargement des données
# =========================
#t = np.load("build/dump_out_dsp.npy")
t = np.load("build/dump_signal0.npy")

print("Shape du tenseur :", t.shape)

# Signal complexe
Z = t[..., 0] + 1j * t[..., 1]
N = len(Z)

# =========================
# Axes
# =========================
time = np.arange(N) / Fs

# FFT
Z_fft = np.fft.fft(Z)
freqs = np.fft.fftfreq(N, d=1/Fs)

# Centrage du spectre
Z_fft = np.fft.fftshift(Z_fft)
freqs = np.fft.fftshift(freqs)

# =========================
# Affichage
# =========================
plt.figure(figsize=(12, 6))

# --- Subplot 1 : temporel ---
plt.subplot(2, 1, 1)
plt.plot(time, Z.real, label="Réel")
plt.plot(time, Z.imag, "--", label="Imaginaire")
plt.xlabel("Temps (s)")
plt.ylabel("Amplitude")
plt.title("Signal temporel complexe")
plt.legend()
plt.grid(True)

# --- Subplot 2 : fréquentiel ---
plt.subplot(2, 1, 2)
plt.plot(freqs, np.abs(Z_fft))
plt.xlabel("Fréquence (Hz)")
plt.ylabel("Amplitude")
plt.title("Spectre fréquentiel |FFT(Z)|")
plt.grid(True)

plt.tight_layout()
plt.show()