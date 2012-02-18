#include <string.h>
#include <math.h>

#include "Config.h"
#include "Core.h"
#include "Gfx.h"
#include "Parser.h"
#include "Map.h"
#include "Anim.h"

namespace bx 
{
/*
#define EXPECT(NAME, TYPE) token NAME = next_token(tokenEnd, end); assert(NAME.mTokenType == TYPE); tokenEnd = NAME.mEnd
#define INT(NAME) token NAME ## Token = next_token(tokenEnd, end); assert(NAME ## Token.mTokenType == kTokenTypeNumber); tokenEnd = NAME ## Token.mEnd; int NAME = strtol(NAME ## Token.mBegin, NULL, 0)
#define FLOAT(NAME) token NAME ## Token = next_token(tokenEnd, end); assert(NAME ## Token.mTokenType == kTokenTypeNumber); tokenEnd = NAME ## Token.mEnd; float NAME = static_cast<float>(strtod(NAME ## Token.mBegin, NULL))
#define SYMBOL(NAME) token NAME ## Token = next_token(tokenEnd, end); assert(NAME ## Token.mTokenType == kTokenTypeSymbol); tokenEnd = NAME ## Token.mEnd; const char *NAME = NAME ## Token.mBegin
#define STRING(NAME) token NAME ## Token = next_token(tokenEnd, end); assert(NAME ## Token.mTokenType == kTokenTypeString); tokenEnd = NAME ## Token.mEnd; const char *NAME = NAME ## Token.mBegin


    namespace map
    {
        static const char *map_proc_file_parser(const char *nextTokenStart, const char *)
        {
            return nextTokenStart;
        }

        static const char *model_parser(const char *nextTokenStart, const char *end)
        {
            const char *tokenEnd = nextTokenStart;

            EXPECT(blockBegin, bx::kTokenTypeBlockBegin);
            EXPECT(modelName, bx::kTokenTypeString);
            INT(numSurfaces);

            bg::surface *surfaces = static_cast<bg::surface *>(bc::temp_alloc(sizeof(bg::surface) * numSurfaces));

            for (int i = 0; i < numSurfaces; ++i)
            {
                EXPECT(sfcBegin, bx::kTokenTypeBlockBegin);
                EXPECT(sfcMaterial, bx::kTokenTypeString);
                INT(numVerts);
                INT(numIndices);
                bg::draw_element_buffer *elementBuffer = bg::deb_create(numVerts, numIndices);
                surfaces[i].element_buffer = elementBuffer;

                void *mem = bc::temp_get_mark();
                bg::vertex *vertices = static_cast<bg::vertex *>(mem);
                memset(vertices, 0, sizeof(bg::vertex) * numVerts);
                                
                for (int ii = 0; ii < numVerts; ++ii)
                {
                    EXPECT(openParen, bx::kTokenTypeTupleBegin);
                    FLOAT(x); FLOAT(y); FLOAT(z);
                    FLOAT(u); FLOAT(v);
                    FLOAT(nx); FLOAT(ny); FLOAT(nz);
                    EXPECT(closeParen, bx::kTokenTypeTupleEnd);

                    vertices[ii].position[0] = x;
                    vertices[ii].position[1] = y;
                    vertices[ii].position[2] = z;
                    vertices[ii].uv[0] = u;
                    vertices[ii].uv[1] = v;
                    vertices[ii].normal[0] = nx;
                    vertices[ii].normal[1] = ny;
                    vertices[ii].normal[2] = nz;
                }

                bg::deb_set_vertices(elementBuffer, vertices);

                unsigned short *indices = static_cast<unsigned short *>(mem);
                memset(indices, 0, sizeof(unsigned short) * numIndices);

                for (int ii = 0; ii < numIndices; ++ii)
                {
                    INT(index);
                    assert(index < 65536);
                    indices[ii] = static_cast<unsigned short>(index);
                }

                bg::deb_set_indices(elementBuffer, indices);

                bc::temp_free_to_mark(mem);

                EXPECT(sfcEnd, bx::kTokenTypeBlockEnd);
            }

            EXPECT(blockEnd, bx::kTokenTypeBlockEnd);
            return blockEnd.mEnd;
        }

        static const char *inter_area_portal_parser(const char *nextTokenStart, const char *end)
        {
            UNUSED(nextTokenStart);
            UNUSED(end);
            return NULL;
        }

        static const char *nodes_parser(const char *nextTokenStart, const char *end)
        {
            UNUSED(nextTokenStart);
            UNUSED(end);
            return NULL;
        }

        static const char *shadow_model_parser(const char *nextTokenStart, const char *end)
        {
            UNUSED(nextTokenStart);
            UNUSED(end);
            return NULL;
        }

        enum section_header
        {
            kMapInvalid,
            kMapSignature,
            kMapModel,
            kInterAreaPortals,
            kMapNodes,
            kMapShadowModel
        };

        static const char *gMapSectionHeaderSymbols[] =
        {
            NULL,
            "mapProcFile003",
            "model",
            "interAreaPortals",
            "nodes",
            "shadowModel",
        };

        typedef const char *(*SectionParserFn)(const char *, const char *);

        SectionParserFn gSectionParsers[] = {
            NULL,
            map_proc_file_parser,
            model_parser,
            inter_area_portal_parser,
            nodes_parser,
            shadow_model_parser
        };

        static section_header classify_symbol(const char *start)
        {
            for (size_t i = 1; i < array_sizeof(gMapSectionHeaderSymbols); ++i)
                if (string_equal(gMapSectionHeaderSymbols[i], start))
                    return static_cast<section_header>(i);
            return kMapInvalid;
        }
    }
*/
   
    const bg::map_data *load_map(const char *file_name)
    {
        UNUSED(file_name);
        return NULL;
    }
}
