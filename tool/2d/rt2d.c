/*
 * Functionality for 2D rendering
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 */

/**
 * \ingroup rt2d
 * \page rt2doverview Rt2d Toolkit Overview
 *
 * \par Requirements
 * \li \b Headers: rwcore.h, rpworld.h, rt2d.h
 * \li \b Libraries: rwcore, rpworld, rt2d
 * \li \b Plugin \b attachments: \ref RpWorldPluginAttach
 *
 * \subsection 2doverview Overview
 * The Rt2d Toolkit provides a suite of powerful functions and datatypes
 * to assist in the programming of 2D graphics.
 *
 * The facilities include advanced font handling; textured and colored
 * brushes; fills; full support for outline and filled paths, as well as
 * an assortment of utility functions.
 *
 * The Rt2d Toolkit also provides tools to create, stream and render complex
 * scenes based on 2d pathes and fonts.
 * This is handled through the use of 4 objects : Shapes, ObjectString, PickRegion
 * and Scenes.
 */

/**
 * \ingroup rt2dfont
 * \page rt2dfontformat Rt2d Font Format
 *
 * The Rt2d Toolkit supports three font formats, one outline and two
 * bitmap fonts. Each font description is contained in a text file with .met
 * extension.
 *
 * The text file must be in UTF-8 format. This is to provide support for Unicode
 * characters.
 *
 * \par Metric 1
 *
 * Metric 1 is a bitmap font and requires a bitmap image. An optional
 * mask can be specified after the image file. The bitmap and mask filenames
 * must not contain any spaces.
 *
 * Metrics 1 uses the .met file to provide the characters available and their
 * dimension. The position values are the pixel co-ordinates in the image.
 *
 * The format of a metric 1 file is as follows,
 * \verbatim
       METRICS1
       <font bitmap> [<font mask bitmap>]
       <base line>
       <character code> <left pos> <top pos> <right pos> <bot pos>
       <character code> <left pos> <top pos> <right pos> <bot pos>
   \endverbatim
 * \par Metric 2
 *
 * Metric 2 is also a bitmap requiring a bitmap image. An optional mask
 * can be specified after the image file. The bitmap and mask filenames must
 * not contain any spaces.
 *
 * The metric 2 .met file only list the characters available in the bitmap. Each
 * character's dimension are encoded in the image font.
 *
 * Each character in the image is surrounded by a boundary. This marks the dimension
 * of the character's bitmap. The start of a character's bitmap is denoted by a
 * marker pixel at the top left of each boundary. It is therefore important that the
 * color values of the marker pixel and the boundary are not used elsewhere.
 * Otherwise the character will use an incorrect area of the bitmap for the character.
 *
 * The same marker pixel must also be present at the bottom left corner for the first
 * character's bitmap. This is used to determine the height of the font set. Otherwise
 * the font will not be loaded correctly.
 *
 * The area used for displaying the character is inset by two pixels from the four
 * boundaries. This is to prevent the boundary pixels from appearing when displaying
 * the character.
 *
 * Metric 2 also support multiple bitmaps for the font. The font can spread over more
 * than one bitmap. This can be used to break up a large image into smaller sections.
 * Or it can be used to support font that have a large number of characters, such as
 * Kanji.
 *
 * Up to four image bitmaps can be specified.
 *
 * The format of a metric 2 file is as follows,
 * \verbatim
       METRICS2
       <font bitmap> [<font mask bitmap>]
       <base line>
       <characters>
       [<font bitmap>] [<font mask bitmap>]
       [<base line>]
       [<characters>]
   \endverbatim
 * \par Metric 3
 *
 * Metric 3 is an outline font similar to Adobe Type 1 fonts. Each character uses a
 * series of 2d vector commands to describe the geometric shape of the character.
 *
 * Each font character begins with the character string. The geometric description
 * begins with the begin keyword and ends with end keyword. There is no limit to
 * number of 2d commands for the font. A final moveto command is used to set the width
 * of the character.
 *
 * The format of a metric 3 file is as follows,
 * \verbatim
       METRICS3
       <font name>
       '<character>'
       begin
       moveto <x> <y>
       lineto <x> <y>
       curveto <x0> <y0> <x1> <y1> <x2> <y2>
       closepath
       moveto <x> <y>
       end
   \endverbatim
 */

/**
 * \ingroup rt2dfont
 * \page rt2dfontunicode Rt2d Font Unicode Support
 *
 * The Rt2d Toolkit provides support for Unicode characters.
 *
 * A font is considered to be Unicode if it contains characters outside the
 * ASCII character set. Otherwise it is considered to be a plain ASCII font.
 *
 * Unicode code characters in a font are encoded using the UTF-8 format in
 * the font metrics file. This is so it remains compatible with a plain
 * ASCII character set, which can be encoded in a single byte UTF-8 format.
 *
 * Rendering a Unicode string is done as normal using the standard API
 * functions. Depending on the font being used, the string will be treated
 * either as Unicode or plain ASCII. Strings using a Unicode font must be
 * in double byte format. Strings using a plain ASCII font must be in
 * single byte format.
 *
 */

/**
 * \defgroup rt2dobjectids Rt2d Object IDs
 * \ingroup rt2d
 *
 * Rt2d list of fundamental object IDs used for streaming.
 *
 * \li rwID_2DBRUSH used by \ref Rt2dBrushStreamRead
 * \li rwID_2DFONT used by \ref Rt2dFontStreamRead
 * \li rwID_2DOBJECTSTRING used by \ref Rt2dObjectStringStreamRead
 * \li rwID_2DPATH used by \ref Rt2dPathStreamRead
 * \li rwID_2DPICKREGION used by \ref Rt2dPickRegionStreamRead
 * \li rwID_2DSCENE used by \ref Rt2dSceneStreamRead
 * \li rwID_2DSHAPE used by \ref Rt2dShapeStreamRead
 *
 * \see RwStreamFindChunk
 * \see RwStreamWriteChunkHeader
 */

#include "rwcore.h"
#include "rpdbgerr.h"
#include "rpworld.h"

#include "rt2d.h"

const RwRGBAReal _rt2dColorMultUnity = {1.0f, 1.0f, 1.0f, 1.0f};
const RwRGBAReal _rt2dColorOffsZero = {0.0f, 0.0f, 0.0f, 0.0f};
