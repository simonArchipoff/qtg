
import numpy as np
import matplotlib.pyplot as plt

# --- Charger les données ---
t = np.load("build/state.npy")  # adapte le nom du fichier si besoin
print("Shape du tenseur :", t.shape)

# Reconstruction du signal complexe
Z = t[...,0] + 1j*t[...,1]

# Calcul amplitude et phase
amplitude = np.abs(Z)
phase = np.angle(Z)


# Axes
nbfreq, nframes = amplitude.shape

# --- Affichage côte à côte ---
fig, axs = plt.subplots(1, 2, figsize=(14,6))

# Amplitude
im0 = axs[0].imshow(amplitude, aspect='auto', origin='lower', interpolation='none')
axs[0].set_title("Amplitude")
axs[0].set_xlabel("Échantillons")
axs[0].set_ylabel("Indice de fréquence")
fig.colorbar(im0, ax=axs[0])

# Phase
im1 = axs[1].imshow(phase, aspect='auto', origin='lower', interpolation='none', cmap='twilight')
axs[1].set_title("Phase")
axs[1].set_xlabel("Échantillons")
axs[1].set_ylabel("Indice de fréquence")
fig.colorbar(im1, ax=axs[1])

plt.tight_layout()
plt.show()

