#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include <map>

#include "Bitmap.hpp"
#include "Node.hpp"
#include "Rect.hpp"
#include "String.hpp"
#include "Processing.hpp"

std::string input;
std::string output( "." );
int size = 1024;
std::string name( "atlas" );
int edges = 0;
int path = -1;
bool potw = false;
bool poth = false;
bool align = false;
std::string prepend;
bool square = false;
bool noalpha = false;
bool splashfill = true;
bool allowFlip = true;


template<typename T>
inline T AlignPOT( T a )
{
    if( a == 0 )
    {
        return 1;
    }

    a--;
    for( int i=1; i<sizeof(T)*8; i<<=1 )
    {
        a |= a >> i;
    }
    return a + 1;
}


std::vector<BRect> LoadImages( const std::list<std::string> pngs, const std::list<std::string> names, const std::list<std::string> rectnames )
{
    std::vector<BRect> ret;

    auto nit = names.cbegin();
    auto rit = rectnames.cbegin();

    ret.reserve( pngs.size() );
    for( auto& png : pngs )
    {
        Bitmap* b = new Bitmap( png.c_str() );
        FILE* f = fopen( rit->c_str(), "rb" );
        if( !f )
        {
            ret.push_back( BRect( 0, 0, b->Size().x, b->Size().y, b, *nit ) );
        }
        else
        {
            int size;
            fread( &size, 1, 4, f );
            if( size == 0 )
            {
                ret.push_back( BRect( 0, 0, 0, 0, b, *nit ) );
            }
            else
            {
                for( int i=0; i<size; i++ )
                {
                    BRect r( 0, 0, 0, 0, b, *nit );
                    fread( &r, 1, sizeof( uint16 ) * 4, f );
                    ret.push_back( r );
                }
            }
            fclose( f );
        }
        ++nit;
        ++rit;
    }

    return ret;
}

void SortImages( std::vector<BRect>& images )
{
    std::sort( images.begin(), images.end(), []( const BRect& i1, const BRect& i2 ){ if( i1.h != i2.h ) return i1.h > i2.h; else return i1.w > i2.w; } );
}

void FindFlipped( std::vector<BRect>& images )
{
    std::for_each( images.begin(), images.end(), []( BRect& img ){ if( img.h > img.w ) { img.flip = true; std::swap( img.w, img.h ); } } );
}

bool DoWork()
{
    FILE* f = fopen( input.c_str(), "r" );
    if( !f ) return false;

    std::list<std::string> names, pngnames, rectnames;
    std::string line;
    while( ReadLine( f, line ) )
    {
        names.push_back( line );
        pngnames.push_back( line.substr( 0, line.rfind( '.' ) ) + ".png" );
        rectnames.push_back( line + ".csr" );
        line.clear();
    }

    fclose( f );

    std::vector<BRect> images( LoadImages( pngnames, names, rectnames ) );
    if( allowFlip )
    {
        FindFlipped( images );
    }
    SortImages( images );

    Bitmap* b = new Bitmap( size, size );
    Node* tree = new Node( Rect( 0, 0, size, size ) );

    int bw = 0;
    int bh = 0;

    std::vector<Rect> rects;
    for( auto& img : images )
    {
        Node* uv = tree->Insert( Rect( edges, edges, img.w + edges * 2, img.h + edges * 2 ), align );
        if( !uv )
        {
            delete b;
            delete tree;
            return false;
        }

        bw = std::max( bw, uv->rect.x + uv->rect.w + edges );
        bh = std::max( bh, uv->rect.y + uv->rect.h + edges );

        rects.push_back( uv->rect );
        Blit( b, img, uv->rect );
        for( int i=0; i<edges; i++ )
        {
            BRect br( img );
            if( br.flip )
            {
                br.y = std::max( 0, img.y - i - 1 );
            }
            else
            {
                br.x = std::max( 0, img.x - i - 1 );
            }
            Blit( b, br, Rect( uv->rect.x - i - 1, uv->rect.y, 1, uv->rect.h ) );

            if( br.flip )
            {
                br.y = std::min( img.b->Size().y - 1, img.y + img.w + i );
            }
            else
            {
                br.x = std::min( img.b->Size().x - 1, img.x + img.w + i );
            }
            Blit( b, br, Rect( uv->rect.x + uv->rect.w + i, uv->rect.y, 1, uv->rect.h ) );
        }
        for( int i=0; i<edges; i++ )
        {
            BRect br( img );
            if( br.flip )
            {
                br.x = std::max( 0, img.x - i - 1 );
            }
            else
            {
                br.y = std::max( 0, img.y - i - 1 );
            }
            Blit( b, br, Rect( uv->rect.x, uv->rect.y - i - 1, uv->rect.w, 1 ) );

            if( br.flip )
            {
                br.x = std::min( img.b->Size().x - 1, img.x + img.h + i );
            }
            else
            {
                br.y = std::min( img.b->Size().y - 1, img.y + img.h + i );
            }
            Blit( b, br, Rect( uv->rect.x, uv->rect.y + uv->rect.h + i, uv->rect.w, 1 ) );
        }
        for( int i=0; i<edges; i++ )
        {
            {
                BRect br( uv->rect.x, uv->rect.y - edges, 1, edges, b, "" );
                Blit( b, br, Rect( uv->rect.x - i - 1, uv->rect.y - edges, 1, edges ) );
            }
            {
                BRect br( uv->rect.x, uv->rect.y + uv->rect.h, 1, edges, b, "" );
                Blit( b, br, Rect( uv->rect.x - i - 1, uv->rect.y + uv->rect.h, 1, edges ) );
            }
            {
                BRect br( uv->rect.x + uv->rect.w - 1, uv->rect.y - edges, 1, edges, b, "" );
                Blit( b, br, Rect( uv->rect.x + uv->rect.w + i, uv->rect.y - edges, 1, edges ) );
            }
            {
                BRect br( uv->rect.x + uv->rect.w - 1, uv->rect.y + uv->rect.h, 1, edges, b, "" );
                Blit( b, br, Rect( uv->rect.x + uv->rect.w + i, uv->rect.y + uv->rect.h, 1, edges ) );
            }
        }
    }

    if( potw )
    {
        bw = AlignPOT( bw );
    }
    if( poth )
    {
        bh = AlignPOT( bh );
    }

    if( square )
    {
        int sq = std::max( bw, bh );
        bw = sq;
        bh = sq;
    }

    if( b->Size().x != bw || b->Size().y != bh )
    {
        Bitmap* tmp = new Bitmap( bw, bh );
        Blit( tmp, BRect( 0, 0, bw, bh, b, "" ), Rect( 0, 0, bw, bh ) );
        delete b;
        b = tmp;
    }

    struct Data
    {
        Data( const BRect* _br, const Rect* _pr ) : br( _br ), pr( _pr ) {}
        const BRect* br;
        const Rect* pr;
    };

    std::map<std::string, std::list<Data> > irmap;
    auto rit = rects.begin();
    for( auto& img : images )
    {
        irmap[img.name].push_back( Data( &img, &(*rit) ) );
        ++rit;
    }

    f = fopen( ( output + "/" + name + ".xml" ).c_str(), "w" );
    fprintf( f, "<?xml version=\"1.0\"?>\n" );
    fprintf( f, "<atlas height=\"%i\" width=\"%i\">\n", b->Size().x, b->Size().y );
    for( auto& data : irmap )
    {
        std::string id;
        if( path == -1 )
        {
            id = prepend + data.first;
        }
        else
        {
            size_t pos = 0;
            for( int i=0; i<path; i++ )
            {
                pos = data.first.find( '/', pos ) + 1;
            }
            id = prepend + data.first.substr( pos );
        }
        fprintf( f, "  <asset id=\"%s\" rects=\"%i\" w=\"%i\" h=\"%i\">\n", id.c_str(), data.second.size(), data.second.begin()->br->b->Size().x, data.second.begin()->br->b->Size().y );
        for( auto dit = data.second.cbegin(); dit != data.second.cend(); ++dit )
        {
            fprintf( f, "    <rect f=\"%i\" ax=\"%i\" ay=\"%i\" x=\"%i\" y=\"%i\" w=\"%i\" h=\"%i\"/>\n",
                dit->br->flip ? 1 : 0,
                dit->pr->x,
                dit->pr->y,
                dit->br->x,
                dit->br->y,
                dit->br->flip ? dit->br->h : dit->br->w,
                dit->br->flip ? dit->br->w : dit->br->h
                );
        }
        fprintf( f, "  </asset>\n" );
    }
    fprintf( f, "</atlas>\n" );
    fclose( f );

    if( splashfill )
    {
        SplashFill( b );
    }

    b->Write( ( output + "/" + name + ".png" ).c_str(), !noalpha );

    delete b;
    delete tree;

    return true;
}

void Usage()
{
    printf( "Usage: TextureAtlas [options]\n\n" );
    printf( "-i, --input        input text file with input files list\n" );
    printf( "-o, --output       output path (default: current dir)\n" );
    printf( "-s, --size         target atlas size (default: 1024)\n" );
    printf( "-e, --edges        duplicate image edges (default: 0)\n" );
    printf( "-n, --name         name of generated files (default: atlas)\n" );
    printf( "-h, --help         prints this message\n" );
    printf( "-P, --path         path strip depth\n" );
    printf( "-W, --potw         make width of atlas a power of two\n" );
    printf( "-H, --poth         make height of atlas a power of two\n" );
    printf( "-a, --align    *   align textures to 4x4 blocks\n" );
    printf( "-p, --prepend      prepend given string to all asset paths\n" );
    printf( "-q, --square       make width equal to height\n" );
    printf( "-N, --noalpha      no alpha channel\n" );
    printf( "--nosplashfill     disable splash fill\n" );
    printf( "--noflip           disable fragment flipping\n" );
}

void Error()
{
    Usage();
    exit( 1 );
}

int main( int argc, char** argv )
{
#define CSTR(x) strcmp( argv[i], x ) == 0

    for( int i=1; i<argc; i++ )
    {
        if( CSTR( "-i" ) || CSTR( "--input" ) )
        {
            input = argv[i+1];
            i++;
        }
        else if( CSTR( "-o" ) || CSTR( "--output" ) )
        {
            output = argv[i+1];
            i++;
        }
        else if( CSTR( "-s" ) || CSTR( "--size" ) )
        {
            size = atoi( argv[i+1] );
            i++;
        }
        else if( CSTR( "-e" ) || CSTR( "--edges" ) )
        {
            edges = atoi( argv[i+1] );
            i++;
        }
        else if( CSTR( "-n" ) || CSTR( "--name" ) )
        {
            name = argv[i+1];
            i++;
        }
        else if( CSTR( "-h" ) || CSTR( "--help" ) )
        {
            Usage();
            return 0;
        }
        else if( CSTR( "-P" ) || CSTR( "--path" ) )
        {
            path = atoi( argv[i+1] );
            i++;
        }
        else if( CSTR( "-W" ) || CSTR( "--potw" ) )
        {
            potw = true;
        }
        else if( CSTR( "-H" ) || CSTR( "--poth" ) )
        {
            poth = true;
        }
        else if( CSTR( "-a" ) || CSTR( "--align" ) )
        {
            align = true;
        }
        else if( CSTR( "-p" ) || CSTR( "--prepend" ) )
        {
            prepend = argv[i+1];
            i++;
        }
        else if( CSTR( "-q" ) || CSTR( "--square" ) )
        {
            square = true;
        }
        else if( CSTR( "-N" ) || CSTR( "--noalpha" ) )
        {
            noalpha = true;
        }
        else if( CSTR( "--nosplashfill" ) )
        {
            splashfill = false;
        }
        else if( CSTR( "--noflip" ) )
        {
            allowFlip = false;
        }
        else
        {
            Error();
        }
    }

#undef CSTR

    if( !DoWork() )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
