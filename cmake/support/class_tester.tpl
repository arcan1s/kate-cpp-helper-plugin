[+ AutoGen5 template cc=%s_tester.cc
#
#
# This is the autogen template file to produce header and module for new fixture tester class.
#
+]
/**
 * \file
 *
 * \brief Class tester for \c [+ classname +]
 *
 * \date [+ (shell "LC_ALL=C date") +] -- Initial design
 */
/*
 * [+ (gpl "KateIncludeHelperPlugin" " * ") +]
 */

// Project specific includes
#include <[+ filename +].hh>
// Debugging helpers from dzen::debug namespace
#include <dzen/debug/dump_memory.hh>
#include <dzen/debug/out_any.hh>
#include <dzen/debug/type_name.hh>

// Standard includes
// ALERT The following #define must be enabled only in one translation unit
// per unit test binary (which may consists of several such modules)
// #define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/auto_unit_test.hpp>
// Include the following file if u need to validate some text results
// #include <boost/test/output_test_stream.hpp>
#include <iostream>

namespace dbg = dzen::debug;

// Uncomment if u want to use boost test output streams.
//  Then just output smth to it and valida an output by
//  BOOST_CHECK(out_stream.is_equal("Test text"))
// using boost::test_tools::output_test_stream;

// Your first test function :)
BOOST_AUTO_TEST_CASE([+ classname +]Test)
{
    // Your test code here...
}
[+(set-writable 1) +]