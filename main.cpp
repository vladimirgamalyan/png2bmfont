#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <iostream>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

int main( int argc, char* argv[] )
{
    fs::path d;
    po::options_description desc( "Allowed options" );
    desc.add_options()
            ( "help", "produce help message" )
            ( "dir", po::value< fs::path >( &d ), "glyphs directory" );
    po::variables_map vm;
    po::store( po::parse_command_line( argc, argv, desc ), vm );

    if ( vm.count( "help" ) )
    {
        std::cout << desc << std::endl;
        return 1;
    }

    po::notify( vm );

    return 0;
}
