#define BOOST_TEST_MODULE ButlerVolmerKinetics
#define BOOST_TEST_MAIN
#include <cap/resistor_capacitor.h>
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <string>
#include <tuple>
#include <cmath>
#include <iostream>



double const I          =  0.006;
double const U          =  2.1;
double const R_SERIES   = 55.0e-3;
double const R_PARALLEL =  2.5e6;
double const C          =  3.0;
double const P          =  0.0017;
double const TOLERANCE  = 1.0e-8;    // in percentage units

std::shared_ptr<boost::property_tree::ptree> initialize_database()
{
    std::shared_ptr<boost::property_tree::ptree> database =
        std::make_shared<boost::property_tree::ptree>();
    database->put("series_resistance"  , R_SERIES  );
    database->put("parallel_resistance", R_PARALLEL);
    database->put("capacitance"        , C         );
    return database;
}

BOOST_AUTO_TEST_CASE( test_series_rc_constant_voltage )
{
    double const TAU     = R_SERIES * C;
    double const DELTA_T = 0.1 * TAU;
    std::vector<double> time;
    for (double t = 0.0; t <= 5.0 * TAU; t += DELTA_T)
        time.push_back(t);

    cap::SeriesRC rc(std::make_shared<cap::Parameters>(initialize_database()));

    // CHARGE
    BOOST_FOREACH(double const & t, time)
    {
        BOOST_CHECK_CLOSE(rc.U_C, U * (1.0 - std::exp(-t/TAU)), TOLERANCE);
        rc.evolve_one_time_step_constant_voltage(DELTA_T, U);
    }

    // DISCHARGE
    rc.reset_voltage(U);
    BOOST_FOREACH(double const & t, time)
    {
        BOOST_CHECK_CLOSE(rc.U_C, U * std::exp(-t/TAU), TOLERANCE);
        rc.evolve_one_time_step_constant_voltage(DELTA_T, 0.0);
    }
}



BOOST_AUTO_TEST_CASE( test_series_rc_constant_current )
{
    double const TAU     = R_SERIES * C;
    double const DELTA_T = 0.1 * TAU;

    std::vector<double> time;
    for (double t = 0.0; t <= 5.0 * TAU; t += DELTA_T)
        time.push_back(t);

    cap::SeriesRC rc(std::make_shared<cap::Parameters>(initialize_database()));

    // CHARGE
    rc.reset_voltage(0.0);
    rc.reset_current(I  );
    BOOST_FOREACH(double const & t, time)
    {
        BOOST_CHECK_CLOSE(rc.U, I * (R_SERIES + t / C), TOLERANCE);
        rc.evolve_one_time_step_constant_current(DELTA_T, I);
    }

    // DISCHARGE
    rc.reset_voltage( U);
    rc.reset_current(-I);
    BOOST_FOREACH(double const & t, time)
    {
        BOOST_CHECK_CLOSE(rc.U, U - I * (R_SERIES + t / C), TOLERANCE);
        rc.evolve_one_time_step_constant_current(DELTA_T, -I);
    }

}



BOOST_AUTO_TEST_CASE( test_series_rc_constant_power )
{
    double const TAU     = R_SERIES * C;
    double const DELTA_T = 0.1 * TAU;

    std::vector<double> time;
    for (double t = 0.0; t <= 5.0 * TAU; t += DELTA_T)
        time.push_back(t);

    std::shared_ptr<cap::Parameters const> params =
        std::make_shared<cap::Parameters const>(initialize_database());
    cap::SeriesRC rc_newton     (params);
    cap::SeriesRC rc_fixed_point(params);

    // CHARGE
    rc_newton     .reset_current(0.0);
    rc_newton     .reset_voltage(U  );
    rc_fixed_point.reset_current(0.0);
    rc_fixed_point.reset_voltage(U  );

    BOOST_FOREACH(double const & t, time)
    {
        std::ignore = t; 
        BOOST_CHECK_CLOSE(rc_newton.U,   rc_fixed_point.U,   TOLERANCE);
        BOOST_CHECK_CLOSE(rc_newton.I,   rc_fixed_point.I,   TOLERANCE);
        BOOST_CHECK_CLOSE(rc_newton.U_C, rc_fixed_point.U_C, TOLERANCE);
        rc_newton     .evolve_one_time_step_constant_power(DELTA_T, P, "NEWTON"     );
        rc_fixed_point.evolve_one_time_step_constant_power(DELTA_T, P, "FIXED_POINT");
    }

   BOOST_CHECK_THROW(rc_newton.evolve_one_time_step_constant_power(DELTA_T, P, "INVALID_ROOT_FINDING_METHOD"), std::runtime_error);

    // DISCHARGE
    rc_newton     .reset_current(0.0);
    rc_newton     .reset_voltage(U  );
    rc_fixed_point.reset_current(0.0);
    rc_fixed_point.reset_voltage(U  );
    BOOST_FOREACH(double const & t, time)
    {
        std::ignore = t; 
        BOOST_CHECK_CLOSE(rc_newton.U,   rc_fixed_point.U,   TOLERANCE);
        BOOST_CHECK_CLOSE(rc_newton.I,   rc_fixed_point.I,   TOLERANCE);
        BOOST_CHECK_CLOSE(rc_newton.U_C, rc_fixed_point.U_C, TOLERANCE);
        rc_newton     .evolve_one_time_step_constant_power(DELTA_T, -P, "NEWTON"     );
        rc_fixed_point.evolve_one_time_step_constant_power(DELTA_T, -P, "FIXED_POINT");
    }
}



BOOST_AUTO_TEST_CASE( test_parallel_rc_constant_current )
{
    double const TAU     = R_PARALLEL * C;
    double const DELTA_T = 0.1 * TAU;
    std::vector<double> time;
    for (double t = 0.0; t <= 5.0 * TAU; t += 0.1*TAU)
        time.push_back(t);

    cap::ParallelRC rc(std::make_shared<cap::Parameters>(initialize_database()));
    rc.R_series = 0.0;

    // CHARGE
    rc.reset_voltage(0.0);
    rc.reset_current(I  );
    BOOST_FOREACH(double const & t, time)
    {
        BOOST_CHECK_CLOSE(rc.U_C, R_PARALLEL * I * (1.0 - std::exp(-t/(R_PARALLEL*C))), TOLERANCE);
        rc.evolve_one_time_step_constant_current(DELTA_T, I);
    }

    // RELAXATION
    rc.reset_voltage(U  );
    rc.reset_current(0.0);
    BOOST_FOREACH(double const & t, time)
    {
        BOOST_CHECK_CLOSE(rc.U_C, U * std::exp(-t/(R_PARALLEL*C)), TOLERANCE);
        rc.evolve_one_time_step_constant_current(DELTA_T, 0.0);
    }
}



BOOST_AUTO_TEST_CASE( test_parallel_rc_constant_voltage )
{
    double const TAU     = R_SERIES * C;
    double const DELTA_T = 0.1 * TAU;

    std::vector<double> time;
    for (double t = 0.0; t <= 5.0 * TAU; t += DELTA_T)
        time.push_back(t);

    cap::ParallelRC rc(std::make_shared<cap::Parameters>(initialize_database()));

    // CHARGE
    rc.reset_voltage(0.0);
    BOOST_FOREACH(double const & t, time)
    {
        BOOST_CHECK_CLOSE(rc.U_C, U * R_PARALLEL/(R_SERIES+R_PARALLEL) * (1.0 - std::exp(-t/((R_SERIES*R_PARALLEL)/(R_SERIES+R_PARALLEL)*C))), TOLERANCE);
        rc.evolve_one_time_step_constant_voltage(DELTA_T, U);
    }

}



BOOST_AUTO_TEST_CASE( test_parallel_rc_constant_power )
{
    double const TAU     = R_SERIES * C;
    double const DELTA_T = 0.1 * TAU;

    std::vector<double> time;
    for (double t = 0.0; t <= 5.0 * TAU; t += DELTA_T)
        time.push_back(t);

    std::shared_ptr<cap::Parameters const> params =
        std::make_shared<cap::Parameters const>(initialize_database());
        std::make_shared<cap::Parameters const>(initialize_database());
    cap::SeriesRC rc_newton     (params);
    cap::SeriesRC rc_fixed_point(params);

    // CHARGE
    rc_newton     .reset_current(0.0);
    rc_newton     .reset_voltage(U  );
    rc_fixed_point.reset_current(0.0);
    rc_fixed_point.reset_voltage(U  );

    BOOST_FOREACH(double const & t, time)
    {
        std::ignore = t;
        BOOST_CHECK_CLOSE(rc_newton.U,   rc_fixed_point.U,   TOLERANCE);
        BOOST_CHECK_CLOSE(rc_newton.I,   rc_fixed_point.I,   TOLERANCE);
        BOOST_CHECK_CLOSE(rc_newton.U_C, rc_fixed_point.U_C, TOLERANCE);
        rc_newton     .evolve_one_time_step_constant_power(DELTA_T, P, "NEWTON"     );
        rc_fixed_point.evolve_one_time_step_constant_power(DELTA_T, P, "FIXED_POINT");
    }

    BOOST_CHECK_THROW(rc_newton.evolve_one_time_step_constant_power(DELTA_T, P, "INVALID_ROOT_FINDING_METHOD"), std::runtime_error);

    // DISCHARGE
    rc_newton     .reset_current(0.0);
    rc_newton     .reset_voltage(U  );
    rc_fixed_point.reset_current(0.0);
    rc_fixed_point.reset_voltage(U  );

    BOOST_FOREACH(double const & t, time)
    {
        std::ignore = t;
        BOOST_CHECK_CLOSE(rc_newton.U,   rc_fixed_point.U,   TOLERANCE);
        BOOST_CHECK_CLOSE(rc_newton.I,   rc_fixed_point.I,   TOLERANCE);
        BOOST_CHECK_CLOSE(rc_newton.U_C, rc_fixed_point.U_C, TOLERANCE);
        rc_newton     .evolve_one_time_step_constant_power(DELTA_T, -P, "NEWTON"     );
        rc_fixed_point.evolve_one_time_step_constant_power(DELTA_T, -P, "FIXED_POINT");
    }
}
