#pragma once
#include <vector>
#include <cstddef>
#include <iomanip>
#include <cassert>
#include <cmath>


/*
  code chatgpt pour la reg. lin.

  todo : change for a real implementation

*/
// Approximation de la valeur t* pour un IC à 95% selon N
static double t_critical_95(size_t df)
{
    // Pour N > 30, t ≈ 1.96
    if (df >= 100)
        return 1.984;
    if (df >= 50)
        return 2.009;
    if (df >= 30)
        return 2.042;
    if (df >= 20)
        return 2.086;
    if (df >= 10)
        return 2.228;
    if (df >= 5)
        return 2.571;
    return 12.706; // df = 1
}

struct LinearFitWithCI
{
    double alpha;
    double beta;
    double r_squared;
    double alpha_stderr;
    double alpha_ci;
};

inline std::ostream &operator<<(std::ostream &os, const LinearFitWithCI &fit)
{
    os << std::fixed << std::setprecision(10); // Affichage avec 10 décimales
    os << "Pente (alpha)       : " << fit.alpha << "\n"
       << "Décalage (beta)     : " << fit.beta << "\n"
       << "Erreur std (alpha)  : " << fit.alpha_stderr << "\n"
       << "IC 95% (alpha)      : " << fit.alpha_ci << "\n"
       << "Coefficient R²      : " << fit.r_squared;
    return os;
}



struct LinearFitResult
{
    double alpha;        // pente
    double beta;         // intercept
    double r_squared;    // coefficient de détermination
    double alpha_stderr; // erreur standard sur alpha
    double alpha_ci95;
};
static double get_t_critical_95(int dof)
{
    assert(dof > 0);
    if (dof >= 30) return 2.0;  // approximation grossière pour dof > 30
    
    // Sinon, on peut stocker quelques valeurs critiques en dur, par exemple:
    // dof : valeur t pour 95% bilateral
    static const double t_table[] = {
        12.706, 4.303, 3.182, 2.776, 2.571, 2.447, 2.365, 2.306, 2.262, 2.228,
        2.201, 2.179, 2.160, 2.145, 2.131, 2.120, 2.110, 2.101, 2.093, 2.086,
        2.080, 2.074, 2.069, 2.064, 2.060, 2.056, 2.052, 2.048, 2.045, 2.042
    };
    int index = dof - 1;
    if (index >= 30) return 2.0;
    if (index < 0) index = 0;
    return t_table[index];
}

static LinearFitResult linear_regression(const std::vector<double> &x, const std::vector<double> &y)
{
    assert(x.size() == y.size());
    size_t N = x.size();
    if (N < 2)
        throw std::invalid_argument("Au moins 2 points nécessaires");

    double sum_x = 0, sum_y = 0;
    double sum_xx = 0, sum_xy = 0, sum_yy = 0;

    for (size_t i = 0; i < N; ++i)
    {
        sum_x += x[i];
        sum_y += y[i];
        sum_xx += x[i] * x[i];
        sum_xy += x[i] * y[i];
        sum_yy += y[i] * y[i];
    }

    double mean_x = sum_x / N;
    double mean_y = sum_y / N;

    double Sxx = sum_xx - N * mean_x * mean_x;
    double Sxy = sum_xy - N * mean_x * mean_y;
    double Syy = sum_yy - N * mean_y * mean_y;

    double alpha = Sxy / Sxx;
    double beta = mean_y - alpha * mean_x;

    // Calcul des résidus
    double ss_res = 0;
    for (size_t i = 0; i < N; ++i)
    {
        double y_pred = alpha * x[i] + beta;
        double diff = y[i] - y_pred;
        ss_res += diff * diff;
    }

    // Variance résiduelle
    double variance = ss_res / (N - 2);

    // Erreur standard sur la pente alpha
    double alpha_stderr = std::sqrt(variance / Sxx);

    // Coefficient de détermination R²
    double r_squared = 1 - (ss_res / Syy);
    double t_crit = N>2 ? get_t_critical_95(static_cast<int>(N - 2)):INFINITY;
    double alpha_ci95 = t_crit * alpha_stderr;
    return {alpha, beta, r_squared, alpha_stderr, alpha_ci95};
}
