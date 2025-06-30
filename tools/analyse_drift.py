import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.stats import linregress
import argparse

def load_data(csv_path, clock_name):
    df = pd.read_csv(csv_path)

    frames = df["frame"].astype(float)

    sec_col = f"{clock_name}_sec"
    nsec_col = f"{clock_name}_nsec"
    if sec_col not in df or nsec_col not in df:
        raise ValueError(f"Horloge {clock_name} introuvable dans le CSV.")

    clock = df[sec_col] + df[nsec_col] * 1e-9

    return frames, clock

def analyze_drift(frames, clock, sample_rate):
    # Convertir en secondes
    frames_sec = (frames - frames.iloc[0]) / sample_rate
    clock_sec = clock - clock.iloc[0]

    # Différence entre les deux horloges
    delta = clock_sec - frames_sec
    delta = delta[1:]
    frames_sec = frames_sec[1:]

    # Régression de delta(t) = slope * frames_sec + intercept
    slope, intercept, r_value, _, stderr = linregress(frames_sec, delta)

    # Dérive relative
    drift_ratio = slope
    drift_hz = drift_ratio * sample_rate
    fps_estimated = sample_rate / (1 + drift_ratio)

    # Résidus pour vérification
    fitted = slope * frames_sec + intercept
    residuals = delta - fitted
    print(f"{slope=}\n{drift_hz=}\n{fps_estimated=}\n")
    return {
        "frames_sec": frames_sec,
        "delta": delta,
        "fitted": fitted,
        "residuals": residuals,
        "slope": slope,
        "stderr": stderr,
        "drift_ratio": drift_ratio,
        "drift_hz": drift_hz,
        "fps_estimated": fps_estimated
    }

def plot_results(frames_sec, delta, fitted, residuals):
    fig, (ax1, ax2) = plt.subplots(2, 1, sharex=True)

    ax1.plot(frames_sec, delta * 1e6, label="delta = clock - frame")
    ax1.plot(frames_sec, fitted * 1e6, '--', label="Fit")
    ax1.set_ylabel("Delta (µs)")
    ax1.set_title("Drift entre horloges")
    ax1.grid()
    ax1.legend()

    ax2.plot(frames_sec, residuals * 1e6)
    ax2.set_xlabel("Temps (s)")
    ax2.set_ylabel("Résidu (µs)")
    ax2.set_title("Erreur de régression")
    ax2.grid()

    plt.tight_layout()
    plt.show()


def compute_sliding_fps(frames_sec, delta, sample_rate, window_size_sec):
    window_fps = []
    window_times = []
    window_ci95 = []

    start = 0
    total_points = len(frames_sec)

    while start < total_points:
        end = start
        while end < total_points and (frames_sec.iloc[end] - frames_sec.iloc[start]) < window_size_sec:
            end += 1
        if end - start < 2:
            break

        x = frames_sec.iloc[start:end]
        y = delta.iloc[start:end]
        slope, _, _, _, stderr = linregress(x, y)

        drift_ratio = slope
        fps_estimated = sample_rate / (1 + drift_ratio)
        ci95 = 1.96 * stderr * sample_rate  # IC95 en Hz

        window_center_time = x.iloc[0] + (x.iloc[-1] - x.iloc[0]) / 2

        window_fps.append(fps_estimated)
        window_times.append(window_center_time)
        window_ci95.append(ci95)

        start = end

    return (
        np.array(window_times),
        np.array(window_fps),
        np.array(window_ci95)
    )

def plot_sliding_fps(times, fps, sample_rate, ci95=None):
    plt.figure()
    plt.plot(times, fps, label="FPS estimé (fenêtre glissante)")
    
    if ci95 is not None:
        plt.fill_between(times, fps - ci95, fps + ci95, alpha=0.3, label="IC95")

    plt.axhline(sample_rate, color="red", linestyle="--", label="FPS nominal")
    plt.xlabel("Temps (s)")
    plt.ylabel("FPS estimé")
    plt.title("Fréquence estimée par fenêtre glissante")
    plt.grid()
    plt.legend()
    plt.tight_layout()
    plt.show()




def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("csv", help="Fichier CSV contenant les horloges")
    parser.add_argument("--clock", default="clock_tai", help="Nom de l'horloge à comparer")
    parser.add_argument("--fps", type=float, default=96000.0, help="Fréquence d'échantillonnage en Hz (par défaut: 96000)")
    parser.add_argument("--window", type=float, default=10.0, help="Taille de la fenêtre glissante en secondes")
    args = parser.parse_args()

    frames, clock = load_data(args.csv, args.clock)
    result = analyze_drift(frames, clock, args.fps)

    print(f"\n--- Analyse de dérive ---")
    print(f"Slope (drift ratio):     {result['slope']:.3e}")
    print(f"Drift (Hz):              {result['drift_hz']:.6f}")
    print(f"Estimated FPS:           {result['fps_estimated']:.6f}")
    print(f"Std error on slope:      {result['stderr']:.3e}\n")

    plot_results(result["frames_sec"], result["delta"], result["fitted"], result["residuals"])
    times, fps,ic  = compute_sliding_fps(result["frames_sec"], result["delta"], args.fps, args.window)
    plot_sliding_fps(times, fps, args.fps,ic)

if __name__ == "__main__":
    main()
