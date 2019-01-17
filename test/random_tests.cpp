
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stk/random/truncated_normal_distribution.hpp>
#include <geometrix/utility/assert.hpp>
#include <geometrix/numeric/constants.hpp>
#include <geometrix/utility/scope_timer.ipp>
#include <boost/math/distributions/normal.hpp>
#include <random>
#include <cmath>

inline double erf(double z0, double z1)
{
	return std::erf(z1) - std::erf(z0);
}

inline double phi(double z0, double z1)
{
	const auto invsqrt2 = 0.70710678118;
	return 0.5 * (std::erf(z1 * invsqrt2) - std::erf(z0 * invsqrt2));
}

//! integral of normal dist from [-inf, x].
inline double normal_cdf(double x, double m, double s)
{
	const auto invsqrt2 = 0.70710678118;
	return 0.5 * (1.0 + std::erf(invsqrt2*(x - m) / s));
}

inline double normal_cdf(double z)
{
	const auto invsqrt2 = 0.70710678118;
	return 0.5 * (1.0 + std::erf(invsqrt2*z));
}

inline double normal_pdf(double z)
{
	double invsqrt2pi = 0.3989422804;
	return invsqrt2pi * std::exp(-0.5 * z * z);
}

template <typename Generator>
double normal_trunc_reject(Generator&& gen, double a, double b)
{
	std::normal_distribution<> N;
	while (true) 
	{
		auto r = N(gen);
		if (r >= a && r <= b)
			return r;
	}
}

template <typename Gen, typename Real>
inline Real devroye_normal_trunc(Gen&& gen, Real a, Real b)
{
	GEOMETRIX_ASSERT(a < b);
	using namespace geometrix;
	using std::sqrt;
	using std::log;
	using std::exp;

	std::uniform_real_distribution<Real> U;

	auto K = a * a * constants::two<Real>();
	auto q = constants::one<Real>() - exp(-(b - a)*a);
	while(true)
	{
		auto u = U(gen);
		auto v = U(gen);
		auto x = -log(constants::one<Real>() - q * u);
		auto e = -log(v);
		if (x*x <= K * e)
			return a + x / a;
	}
}

template <typename Gen, typename UniformRealDist, typename Real>
inline Real rayleigh_normal_trunc(Gen&& gen, UniformRealDist&& U, Real a, Real b)
{
	GEOMETRIX_ASSERT(a < b);
	using namespace geometrix;
	using std::sqrt;
	using std::log;
	using std::exp;

	auto c = a * a * constants::one_half<Real>();
	auto b2 = b * b;
	auto q = constants::one<Real>() - exp(c - b2 * constants::one_half<Real>());
	while(true)
	{
		auto u = U(gen);
		auto v = U(gen);
		auto x = c - log(constants::one<Real>() - q * u);
		if (v*v*x <= a) 
		{
			auto two_x = constants::two<Real>() * x;
			return sqrt(two_x);
		}
	}
}

template <typename Gen, typename UniformRealDist, typename Real>
inline Real rayleigh_normal_reject(Gen&& gen, UniformRealDist&& U, Real a, Real b)
{
	GEOMETRIX_ASSERT(a < b);
	using namespace geometrix;
	using std::sqrt;
	using std::log;

	auto c = a * a * constants::one_half<Real>();
	auto b2 = b * b;
	while(true)
	{
		auto u = U(gen);
		auto v = U(gen);
		auto x = c - log(u);
		auto two_x = constants::two<Real>() * x;
		if (v*v*x <= a && two_x <= b2)
			return sqrt(two_x);
	}
}

template <typename Gen, typename UniformRealDist, typename Real>
inline Real uniform_normal_trunc(Gen&& gen, UniformRealDist&& U, Real a, Real b)
{
	GEOMETRIX_ASSERT(a < b);
	using namespace geometrix;
	using std::log;

	auto a2 = a * a;
	while(true)
	{
		auto u = U(gen);
		auto v = U(gen);
		auto x = a + (b - a)*u;
		if (constants::two<Real>() * log(v) <= a2 - x * x)
			return x;
	}
}

#include <stk/geometry/geometry_kernel.hpp>
#include <stk/sim/histogram_1d.ipp>
#include <fstream>

template <typename T>
inline void write_hist(std::ostream& os, const stk::histogram_1d<T>& hist)
{
	os << "x, y\n";
	for (std::size_t i = 0; i < hist.get_number_bins(); ++i)
	{
		os << hist.get_bin_center(i+1) << "," << hist.get_bin_content(i+1) << "\n";
	}

	os << std::endl;
}

auto nruns = 1000000ULL;

struct ks_test_fixture : ::testing::TestWithParam<std::pair<double, double>>{};

TEST_P(ks_test_fixture, compare_truncated_dist_against_normal_sampler)
{
	double l, h;
	std::tie(l, h) = GetParam();
	stk::histogram_1d<double> chist(1000, l - .1*l, h + .1*h);
	stk::histogram_1d<double> nhist(1000, l - .1*l, h + .1*h);
	stk::truncated_normal_distribution<> cdist(l, h);

	std::mt19937 gen(42UL);
	for (auto i = 0ULL; i < nruns; ++i) {
		{
			auto v = normal_trunc_reject(gen, l, h);
			nhist.fill(v);
		}
		{
			auto v = cdist(gen);
			chist.fill(v);
		}
	}

	chist.scale(1.0 / chist.integral());
	nhist.scale(1.0 / nhist.integral());

#define STK_EXPORT_HISTS
#ifdef STK_EXPORT_HISTS
	{
		std::stringstream nname;
		nname << "e:/data_chopin" << l << "_" << h << ".csv";
		std::ofstream ofs(nname.str());
		write_hist(ofs, chist);
	}
	{
		std::stringstream nname;
		nname << "e:/data_control" << l << "_" << h << ".csv";
		std::ofstream ofs(nname.str());
		write_hist(ofs, nhist);
	}
#endif

	double d, p;
	std::tie(d, p) = chist.ks_test(nhist);
	EXPECT_GT(p, 0.99);
}

INSTANTIATE_TEST_CASE_P(validate_chopin, ks_test_fixture, ::testing::Values(
    std::make_pair(-3., 2.)
  , std::make_pair(-4., 4.)
  , std::make_pair(-9.0, -2.0)
  , std::make_pair(2.0, 9.0)
  , std::make_pair(-0.48, 0.1)
  , std::make_pair(-0.1, 0.48)
  //! Slow test below
  //, std::make_pair(3.49, 100.0)
  //, std::make_pair(-100.0, -3.49)
));

struct time_chopin_fixture : ::testing::TestWithParam<std::pair<double, double>>{};
TEST_P(time_chopin_fixture, truncated_chopin)
{
	double l, h;
	std::tie(l, h) = GetParam();
	stk::truncated_normal_distribution<> cdist(l, h);

	std::vector<double> r(nruns);
	std::mt19937 gen(42UL);
	for (auto i = 0ULL; i < nruns; ++i) 
	{
		r[i] = cdist(gen);
	}

	EXPECT_TRUE(!r.empty());
}

INSTANTIATE_TEST_CASE_P(time_chopin, time_chopin_fixture, ::testing::Values(
    std::make_pair(-3., 2.)
  , std::make_pair(-4., 4.)
  , std::make_pair(-9.0, -2.0)
  , std::make_pair(2.0, 9.0)
  , std::make_pair(-0.48, 0.1)
  , std::make_pair(-0.1, 0.48)
  //! Slow test below
  , std::make_pair(3.49, 100.0)
  , std::make_pair(-100.0, -3.49)
));

TEST(truncated_normal_test_suite, brute_normal_distribution)
{
	stk::histogram_1d<double> hist(1000, -9.1, -1.8);
	std::normal_distribution<> dist;

	std::mt19937 gen(42UL);
	for (auto i = 0ULL; i < nruns; ++i)
	{
		auto v = normal_trunc_reject(gen, -9.0, -2.0);// -0.46, 0.84);
		hist.fill(v);
	}

	hist.scale(1.0 / hist.integral());

	std::ofstream ofs("e:/data_control.csv");
	write_hist(ofs, hist);
	EXPECT_TRUE(true);
}

TEST(truncated_normal_test_suite, brute_hueristic_uniform)
{
	std::mt19937 gen(42UL);
	std::uniform_real_distribution<> dist;

	for (auto i = 0ULL; i < nruns; ++i)
	{
		auto v = uniform_normal_trunc(gen, dist, 7.0, 8.0);
		EXPECT_TRUE(v >= 7.0 && v <= 8.0);
	}
}

TEST(truncated_normal_test_suite, brute_hueristic_rayleigh_trunc)
{
	std::mt19937 gen(42UL);
	std::uniform_real_distribution<> dist;

	for (auto i = 0ULL; i < nruns; ++i)
	{
		auto v = rayleigh_normal_trunc(gen, dist, 7.0, 8.0);
		EXPECT_TRUE(v >= 7.0 && v <= 8.0);
	}
}

TEST(truncated_normal_test_suite, brute_hueristic_rayleigh_reject)
{
	std::mt19937 gen(42UL);
	std::uniform_real_distribution<> dist;

	for (auto i = 0ULL; i < nruns; ++i)
	{
		auto v = rayleigh_normal_reject(gen, dist, 7.0, 8.0);
		EXPECT_TRUE(v >= 7.0 && v <= 8.0);
	}
}

TEST(truncated_normal_test_suite, brute_hueristic_timing)
{
	std::mt19937 gen(42UL);
	std::uniform_real_distribution<> dist;

	auto sum = 0;
	{
		GEOMETRIX_MEASURE_SCOPE_TIME("uniform_normal_trunc");
		for (auto i = 0ULL; i < nruns; ++i)
		{
			auto v = uniform_normal_trunc(gen, dist, 7.0, 8.0);
			sum += v;
		}
	}

	EXPECT_TRUE(sum > 0.0);
}

