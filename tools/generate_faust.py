#!/usr/bin/env python3
"""
Faust envelope detector code generator (analytic signal via Hilbert filter).
Uses windowed FIR design (Hamming) – no special functions, easy to port.
"""

import argparse
import sys
import numpy as np

def design_filters(sample_rate, lowcut, highcut, fir_size, fir_thres=None):
    """
    Compute coefficients for the two FIR branches using windowed design.
    Returns (b_re, b_im)
        b_re : bandpass filter coefficients (real part)
        b_im : bandpass Hilbert filter coefficients (imaginary part)
    Both filters have the same length and no extra delay is required.
    """
    if fir_size % 2 == 0:
        fir_size += 1
    N = fir_size

    nyq = sample_rate / 2.0
    if not (0 < lowcut < highcut < nyq):
        raise ValueError(f"Band [{lowcut}, {highcut}] must lie within ]0, {nyq}[.")

    M = (N - 1) // 2
    n = np.arange(-M, M + 1)
    fs = sample_rate

    # Ideal infinite impulse responses
    with np.errstate(divide='ignore', invalid='ignore'):
        h_re_ideal = np.where(
            n != 0,
            (np.sin(2 * np.pi * highcut * n / fs) - np.sin(2 * np.pi * lowcut * n / fs)) / (np.pi * n),
            2.0 * (highcut - lowcut) / fs
        )
        h_im_ideal = np.where(
            n != 0,
            (np.cos(2 * np.pi * lowcut * n / fs) - np.cos(2 * np.pi * highcut * n / fs)) / (np.pi * n),
            0.0
        )

    # Hamming window (explicit, no external function)
    window = 0.54 - 0.46 * np.cos(2 * np.pi * np.arange(N) / (N - 1))

    # Apply window and return causal coefficients (list)
    b_re = (h_re_ideal * window)
    b_im = (h_im_ideal * window)
    if fir_thres:
        b_re[np.abs(b_re) < fir_thres] = 0.0
        b_im[np.abs(b_im) < fir_thres] = 0.0
    
    return b_re, b_im

def format_faust_list(coeffs, decimals=10):
    """Format a numpy array as a Faust list (c0, c1, ...)."""
    formatted = ", ".join(f"{c:.{decimals}f}" for c in coeffs)
    return f"({formatted})"

def format_C_list(coeffs, decimals=10):
    """Format a numpy array as a Faust list (c0, c1, ...)."""
    formatted = ", ".join(f"{c:.{decimals}f}" for c in coeffs)
    return f"{{{formatted}}}"




def generate_faust(b_re, b_im):
    """Generate the complete Faust program without symmetry optimization."""
    code = f"""// Envelope detector via Hilbert filter (analytic signal)
// Auto-generated – do not edit manually.

import("stdfaust.lib");

// FIR coefficients (real and imaginary branches)
b_re = {format_faust_list(b_re)};
b_im = {format_faust_list(b_im)};

// Module of the analytic signal
square_sum = \\(x, y).(x * x + y * y) : sqrt;

// Real branch: bandpass FIR
// Imaginary branch: bandpass Hilbert FIR
process = _ <: (fi.fir(b_re), fi.fir(b_im)) : square_sum;
"""
    return code

def compute_analytic_spectrum(b_re, b_im, sample_rate, nfft=8192):
    """
    Compute the complex frequency response of the analytic filter.
    Returns (freqs, magnitude) where freqs covers -fs/2 .. fs/2.
    """
    h_analytic = b_re + 1j * b_im   # note: sign kept as in original
    H = np.fft.fft(h_analytic, n=nfft)
    freqs = np.fft.fftfreq(nfft, d=1.0 / sample_rate)
    H_shifted = np.fft.fftshift(H)
    freqs_shifted = np.fft.fftshift(freqs)
    magnitude = np.abs(H_shifted)
    return freqs_shifted, magnitude

def plot_spectrum(freqs, magnitude, sample_rate, lowcut, highcut, output_file=None):
    """Plot the magnitude response of the analytic filter."""
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        print("Matplotlib is required for plotting. Please install it.", file=sys.stderr)
        sys.exit(1)

    plt.figure(figsize=(10, 5))
    plt.plot(freqs, 20 * np.log10(magnitude + 1e-30) , 'b-', linewidth=1.5)
    plt.xlabel("Frequency (Hz)")
    plt.ylabel("Magnitude")
    plt.title("Analytic filter magnitude response (Dirac input)")
    plt.axvspan(-highcut, -lowcut, alpha=0.2, color='red', label='Negative band')
    plt.axvspan(lowcut, highcut, alpha=0.2, color='green', label='Positive band')
    plt.legend()
    plt.grid(True)
    plt.xlim(-sample_rate / 2, sample_rate / 2)

    if output_file:
        plt.savefig(output_file, dpi=150, bbox_inches='tight')
        print(f"Plot saved to {output_file}")
    else:
        plt.show()

def main():
    parser = argparse.ArgumentParser(
        description="Generate a Faust envelope detector using analytic signal."
    )
    parser.add_argument(
        "--sample-rate", type=float, default=44100.0,
        help="Sampling frequency in Hz (default: 44100)"
    )
    parser.add_argument(
        "--lowcut", type=float, required=True,
        help="Lower cutoff frequency in Hz"
    )
    parser.add_argument(
        "--highcut", type=float, required=True,
        help="Upper cutoff frequency in Hz"
    )
    parser.add_argument(
        "--fir-size", type=int, default=255,
        help="Number of FIR coefficients (odd, forced odd if even; default: 255)"
    )
    parser.add_argument(
        "--fir-thres", type=float, default=None,
        help="seiul"
    )
    parser.add_argument(
        "-o", "--output", type=str,
        help="Output Faust file (default: stdout)"
    )
    parser.add_argument(
        "--plot", action="store_true",
        help="Plot the analytic filter magnitude spectrum"
    )
    parser.add_argument(
        "--plot-file", type=str, default=None,
        help="Save plot to a file instead of showing (requires --plot)"
    )

    args = parser.parse_args()

    try:
        b_re, b_im = design_filters(
            args.sample_rate, args.lowcut, args.highcut, args.fir_size,args.fir_thres
        )
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

    faust_code = generate_faust(b_re, b_im)

    if args.output:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(faust_code)
        print(f"Faust code written to {args.output}")
    else:
        print(faust_code)

    if args.plot:
        freqs, magnitude = compute_analytic_spectrum(
            b_re, b_im, args.sample_rate
        )
        plot_spectrum(
            freqs, magnitude, args.sample_rate,
            args.lowcut, args.highcut, args.plot_file
        )

if __name__ == "__main__":
    main()
