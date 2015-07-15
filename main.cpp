#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include "lodepng/lodepng.h"
#include "MaxRectsBinPack/MaxRectsBinPack.h"
#include "tinyxml2/tinyxml2.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

//TODO: remove transparent border around glyphs.

class SrcBitmap
{
public:
    SrcBitmap(const fs::path& fileName)
    {
        unsigned error = lodepng::decode(data, width, height, fileName.generic_string().c_str());
        if ( error )
            throw std::runtime_error( std::string("png decoder error ") + lodepng_error_text(error) );
        charId = fileName.stem().generic_string();
        if ( charId.length() > 1 )
            charId.erase(0, 1);
        else
            charId = boost::lexical_cast<std::string>(charId.at(0));
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
    std::string getCharId() const
    {
        return charId;
    }
private:
    std::string charId;
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

void makeFnt( const fs::path& dir, fs::path output, unsigned int textureWidth, unsigned int textureHeight )
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

    std::vector< rbp::RectSize > srcRects;
    for ( size_t i = 0; i < srcImages.size(); ++i )
    {
        srcRects.push_back(rbp::RectSize());
        srcRects.back().tag = i;
        srcRects.back().width = srcImages[i].getWidth();
        srcRects.back().height = srcImages[i].getHeight();
    }


    tinyxml2::XMLDocument doc;
    tinyxml2::XMLDeclaration* decl = doc.NewDeclaration();
    doc.InsertFirstChild(decl);

    tinyxml2::XMLElement* root = doc.NewElement("font");
    doc.InsertEndChild(root);

    tinyxml2::XMLElement* pagesElement = doc.NewElement("pages");
    root->InsertEndChild(pagesElement);

    tinyxml2::XMLElement* chars = doc.NewElement("chars");
    root->InsertEndChild(chars);


    rbp::MaxRectsBinPack mrbp;

    int charsCount = 0;
    for (int page = 0;; ++page)
    {
        const rbp::MaxRectsBinPack::FreeRectChoiceHeuristic choiceHeuristics[ 5 ] = {
                rbp::MaxRectsBinPack::RectBestShortSideFit,
                rbp::MaxRectsBinPack::RectBestLongSideFit,
                rbp::MaxRectsBinPack::RectBestAreaFit,
                rbp::MaxRectsBinPack::RectBottomLeftRule,
                rbp::MaxRectsBinPack::RectContactPointRule
        };


        float bestOccupancy = 0.f;
        rbp::MaxRectsBinPack::FreeRectChoiceHeuristic bestChoiceHeuristic = rbp::MaxRectsBinPack::RectBestShortSideFit;
        for ( int i = 0; i < 5; ++i )
        {
            mrbp.Init(textureWidth, textureHeight);
            std::vector<rbp::Rect> readyRects;
            std::vector< rbp::RectSize > tempRects( srcRects );
            mrbp.Insert( tempRects, readyRects, choiceHeuristics[ i ] );
            if ( readyRects.empty() )
                continue;
            float occupancy = mrbp.Occupancy();
            if ( occupancy > bestOccupancy )
            {
                bestOccupancy = occupancy;
                bestChoiceHeuristic = choiceHeuristics[ i ];
            }
        }

        mrbp.Init(textureWidth, textureHeight);
        std::vector<rbp::Rect> readyRects;
        mrbp.Insert( srcRects, readyRects, bestChoiceHeuristic );

        if ( readyRects.empty() )
        {
            if ( !srcRects.empty() )
                throw std::runtime_error("can not fit glyphs to texture");
            break;
        }

        Bitmap bitmap(textureWidth, textureHeight);
        for ( std::vector<rbp::Rect>::iterator it = readyRects.begin(); it != readyRects.end(); ++it )
        {
            bitmap.paste(srcImages[it->tag], it->x, it->y);

            tinyxml2::XMLElement* charElement = doc.NewElement("char");
            charElement->SetAttribute("id", srcImages[it->tag].getCharId().c_str());
            charElement->SetAttribute("x", it->x);
            charElement->SetAttribute("y", it->y);
            charElement->SetAttribute("width", it->width);
            charElement->SetAttribute("height", it->height);
            charElement->SetAttribute("xoffset", 0);
            charElement->SetAttribute("yoffset", 0);
            charElement->SetAttribute("xadvance", it->width);
            charElement->SetAttribute("page", page);
            charElement->SetAttribute("chnl", 15);


            chars->InsertEndChild(charElement);
            charsCount++;
        }

        std::stringstream ss;
        ss << output.generic_string() << "_" << page << ".png";
        bitmap.saveToFile(ss.str());

        ss.str(std::string());
        ss << output.filename().generic_string() << "_" << page << ".png";

        tinyxml2::XMLElement* pageElement = doc.NewElement("page");
        pageElement->SetAttribute("id", page);
        pageElement->SetAttribute("file", ss.str().c_str());
        pagesElement->InsertEndChild(pageElement);
    }

    chars->SetAttribute("count", charsCount);

    output.replace_extension("fnt");
    tinyxml2::XMLError err = doc.SaveFile(output.generic_string().c_str(), false);
    if (err)
        throw std::runtime_error("xml write to file error");
}

int main( int argc, char** argv )
{
    try
    {
        fs::path dir;
        fs::path output;
        unsigned int textureWidth;
        unsigned int textureHeight;
        po::options_description desc( "Allowed options" );
        desc.add_options()
                ( "help", "produce help message" )
                ( "dir", po::value< fs::path >( &dir )->required(), "glyphs directory" )
                ( "width", po::value< unsigned int >( &textureWidth )->required(), "texture width" )
                ( "height", po::value< unsigned int >( &textureHeight )->required(), "texture height" )
                ( "output", po::value< fs::path >( &output )->required(), "path to output files without extenstion" )
                ;
        po::variables_map vm;
        po::store( po::parse_command_line( argc, argv, desc ), vm );

        if ( vm.count( "help" ) )
        {
            std::cout << desc << std::endl;
            return 1;
        }

        po::notify( vm );

        if ( !fs::is_directory(dir) )
            throw std::runtime_error("specified dir path is not a directory");

        //if ( !fs::is_directory( output.parent_path() ) )
        //    throw std::runtime_error("specified output path is not valid");

        makeFnt( dir, output, textureWidth, textureHeight );
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
