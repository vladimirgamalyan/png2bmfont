#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include "lodepng/lodepng.h"
#include "MaxRectsBinPack/MaxRectsBinPack.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;


class SrcBitmap
{
public:
    SrcBitmap(const fs::path& fileName)
    {
        unsigned error = lodepng::decode(data, width, height, fileName.generic_string().c_str());
        if ( error )
            throw std::runtime_error( std::string("png decoder error ") + lodepng_error_text(error) );
    }

    unsigned getWidth() const
    {
        return width;
    }

    unsigned getHeight() const
    {
        return height;
    }

    const unsigned char* getData() const
    {
        return data.data();
    }
private:
    unsigned width;
    unsigned height;
    std::vector<unsigned char> data;
};

class Bitmap
{
public:
    Bitmap(unsigned width, unsigned height) : width(width), height(height)
    {
        data.resize( width * height * 4 );
    }

    void saveToFile( const fs::path& fileName )
    {
        unsigned error = lodepng_encode32_file(fileName.generic_string().c_str(), data.data(), width, height);
        if (error)
            throw std::runtime_error(std::string("png encode error: ") + lodepng_error_text(error));
    }

    void paste(const unsigned char* img, int x, int y, unsigned w, unsigned h)
    {
        unsigned char* d = data.data() + ( ( x + y * width ) << 2 );
        const unsigned char* s = img;

        unsigned srcWidth = w << 2;
        unsigned dstWidth = width << 2;

        while (h)
        {
            std::memcpy( d, s, srcWidth );
            d += dstWidth;
            s += srcWidth;
            h--;
        }
    }

    void paste(const SrcBitmap& srcBitmap, int x, int y)
    {
        paste(srcBitmap.getData(), x, y, srcBitmap.getWidth(), srcBitmap.getHeight());
    }

private:
    unsigned width;
    unsigned height;
    std::vector< unsigned char > data;
};

void makeFnt( const fs::path& dir )
{
    std::vector<fs::path> files;
    for( fs::recursive_directory_iterator it(dir); it != fs::recursive_directory_iterator(); ++it )
        if ( fs::is_regular_file(*it) && it->path().extension() == ".png" )
            files.push_back(it->path());

    if ( files.empty() )
        throw std::runtime_error("images not found");

    std::vector< SrcBitmap > srcImages;
    for ( std::vector<fs::path>::iterator it = files.begin(); it != files.end(); ++it )
        srcImages.push_back(SrcBitmap(*it));

    std::cout << srcImages.size() << " images found" << std::endl;

    std::vector< rbp::RectSize > srcRects;
    for ( size_t i = 0; i < srcImages.size(); ++i )
    {
        srcRects.push_back(rbp::RectSize());
        srcRects.back().tag = i;
        srcRects.back().width = srcImages[i].getWidth();
        srcRects.back().height = srcImages[i].getHeight();
    }

    const int DST_WIDTH = 256;
    const int DST_HEIGHT = 256;

    for (int page = 0;; ++page)
    {
        rbp::MaxRectsBinPack mrbp(DST_WIDTH, DST_HEIGHT);
        std::vector<rbp::Rect> readyRects;
        mrbp.Insert( srcRects, readyRects, rbp::MaxRectsBinPack::RectBestAreaFit );

        std::cout << readyRects.size() << " images placed" << std::endl;
        if ( readyRects.empty() )
            break;

        Bitmap bitmap(DST_WIDTH, DST_HEIGHT);
        for ( std::vector<rbp::Rect>::iterator it = readyRects.begin(); it != readyRects.end(); ++it )
            bitmap.paste(srcImages[it->tag], it->x, it->y);

        std::stringstream ss;
        ss << "page" << page << ".png";
        bitmap.saveToFile(ss.str());
    }
}

int main( int argc, char** argv )
{
    try
    {
        fs::path dir;
        po::options_description desc( "Allowed options" );
        desc.add_options()
                ( "help", "produce help message" )
                ( "dir", po::value< fs::path >( &dir )->required(), "glyphs directory" );
        po::variables_map vm;
        po::store( po::parse_command_line( argc, argv, desc ), vm );

        if ( vm.count( "help" ) )
        {
            std::cout << desc << std::endl;
            return 1;
        }

        po::notify( vm );

        if ( !fs::is_directory(dir) )
            throw std::runtime_error("specified path is not a directory");

        makeFnt( dir );
    }
    catch ( std::exception& e )
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown error!" << std::endl;
        return 1;
    }

    return 0;
}
