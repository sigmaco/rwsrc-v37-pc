/*
 *
 * Functionality for 2D rendering
 *
 * Copyright (c) 2001 Criterion Software Ltd.
 */

/****************************************************************************
 *                                                                          *
 *  Module  :   2dstr.c                                                     *
 *                                                                          *
 *  Purpose :   Rt2dObjectString functions                                  *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 Includes
 */
#include <string.h>

#include "rwcore.h"
#include "rpdbgerr.h"
#include "rpworld.h"

#include "rt2d.h"
#include "path.h"
#include "font.h"
#include "tri.h"
#include "gstate.h"
#include "object.h"
#include "brush.h"

#include "2dstr.h"

/*-----------------20/08/2001 10:20 AJH -------------
 * Is that really needed ???
 * --------------------------------------------------*/
#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline off
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */

/****************************************************************************
 Local Types
 */

/****************************************************************************
 Local (Static) Prototypes
 */

/****************************************************************************
 Local Defines
 */

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

   Rt2dObjectString

   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/**
 * \ingroup rt2dobjectstring
 * \page rt2dobjectstringoverview Rt2dObjectString Overview
 *
 * An object string is an object which contains text. An object string can be
 * saved and added to a scene.
 *
 * \par Creating an Object String
 * -# \ref Rt2dObjectStringCreate creates the scene.
 * -# \ref Rt2dObjectStringGetBrush returns a brush used for rendering a string..
 *
 * \par Adding an Object String to a Scene
 * -# \ref Rt2dSceneLock locks the scene.
 * -# \ref Rt2dSceneAddChild adds the object string to the scene.
 * -# \ref Rt2dSceneUnlock unlocks the scene.
 *
 * \see Rt2dSceneCreate
 */

/****************************************************************************
 Local (Static) functions
 */

static RwUInt32
wchar_strlen( const RwChar * str )
{
    RwUInt32        l;

    RWFUNCTION(RWSTRING("wchar_strlen"));

    l = 0;

    while ((0 != str[0]) || (0 != str[1]))
    {
        l += 2;

        str += 2;
    }

    RWRETURN(l);
}


/**
 * \ingroup rt2dobjectstring
 * \ref Rt2dObjectStringCreate creates a new object string. The font is
 * identified by name, and gets looked up from the font dictionary
 *
 * \param textString Pointer to the text string to be used
 * \param font       Pointer to a font
 *
 * \return Returns the new object string on success, NULL otherwise
 *
 * \see Rt2dObjectStringDestroy
 * \see Rt2dObjectStringGetBrush
 */
Rt2dObject  *
Rt2dObjectStringCreate(const RwChar *textString, const RwChar *font)
{
    Rt2dObject           *object;

    RWAPIFUNCTION(RWSTRING("Rt2dObjectStringCreate"));
    RWASSERT( (textString && font) || ((!textString) && (!font))); /* Assert all or nothing */

    object = _rt2dObjectCreate();
    RWASSERT(object);

    _rt2dObjectInit(object, rt2DOBJECTTYPEOBJECTSTRING);

    if(!_rt2dObjectStringInit(object, textString, font))
    {
        Rt2dObjectStringDestroy(object);
        RWRETURN((Rt2dObject *)NULL);
    }

    RWRETURN(object);
}

static _rt2dObjectString *
_rt2dObjectStringInitTextString(_rt2dObjectString *objectString,
                                const RwChar *textString)
{
    RwInt32 stringLength;

    RWFUNCTION(RWSTRING("_rt2dObjectStringInitTextString"));
    RWASSERT(objectString);
    RWASSERT(textString);

    if (Rt2dFontIsUnicode((Rt2dFont *) objectString->font->font))
    {
        stringLength = wchar_strlen(textString);

        objectString->textString = (RwChar*)(RwMalloc(stringLength + 2,
                                rwMEMHINTDUR_EVENT | rwID_2DOBJECTSTRING));
        RWASSERT(objectString->textString);

        if (objectString->textString)
        {
            memcpy(objectString->textString, textString, (stringLength + 2));
        }
        else
        {
            RWRETURN((_rt2dObjectString *)NULL);
        }
    }
    else
    {
        stringLength = rwstrlen(textString);

        objectString->textString = (RwChar*)(RwMalloc(stringLength + 1,
                              rwMEMHINTDUR_EVENT | rwID_2DOBJECTSTRING));
        RWASSERT(objectString->textString);

        if (objectString->textString)
        {
            rwstrcpy(objectString->textString, textString);
        }
        else
        {
            RWRETURN((_rt2dObjectString *)NULL);
        }
    }

    objectString->maxLength = stringLength;

    RWRETURN(objectString);
}

Rt2dObject  *
_rt2dObjectStringInit(Rt2dObject* object, const RwChar *textString, const RwChar *font)
{
    RWFUNCTION(RWSTRING("_rt2dObjectStringInit"));

    RWASSERT(object);
    RWASSERT(textString);

    object->data.objectString.height = 1.0f;

    /* Strings are always locked */
    object->flag &= ~Rt2dObjectIsLocked;

    object->data.objectString.brush = Rt2dBrushCreate();
    if (!object->data.objectString.brush)
    {
        RWRETURN((Rt2dObject *)FALSE);
    }

    if (!Rt2dObjectStringSetFont(object, font))
    {
        RWMESSAGE((RWSTRING("Unable to load font:")));
        RWMESSAGE((font));
        RWERROR((E_RW_FONTNOTFOUND));

        /* To maintain a RwTexture like behavior :
         * Flag the object so that it doesn't get rendered */
        object->flag |= Rt2dObjectStringGotNoFonts;

        /* function has not failed; only a font has not been found */
    }

    if (textString)
    {
        if (!_rt2dObjectStringInitTextString(&object->data.objectString, textString))
        {
            Rt2dBrushDestroy(object->data.objectString.brush);
            RWRETURN((Rt2dObject *)NULL);
        }
    }
    else
    {
        object->data.objectString.textString = (RwChar *)NULL;
        object->data.objectString.maxLength = 0;
    }

    RWRETURN(object);
}

RwBool
_rt2dObjectStringDestruct(Rt2dObject *object)
{
    RwBool result=TRUE;
    RWFUNCTION(RWSTRING("_rt2dObjectStringDestruct"));

    RWASSERT(object);
    RWASSERT(((object->type) == rt2DOBJECTTYPEOBJECTSTRING));

    /* Delete owned contents */
    if (object->data.objectString.textString)
        RwFree(object->data.objectString.textString);

    if (object->data.objectString.brush)
    {
        result = Rt2dBrushDestroy(object->data.objectString.brush);
    }

    /* Note - font is not owned */

    RWRETURN(result);
}

/**
 * \ingroup rt2dobjectstring
 * \ref Rt2dObjectStringDestroy
 * Deallocates the provided ObjectString
 *
 * \param object is the object string to be destroyed
 *
 * \return returns TRUE on success; FALSE otherwise
 *
 * \see Rt2dObjectStringCreate
 */
RwBool
Rt2dObjectStringDestroy(Rt2dObject *object)
{
    RwBool        result;

    RWAPIFUNCTION(RWSTRING("Rt2dObjectStringDestroy"));
    RWASSERT(object);
    RWASSERT(((object->type) == rt2DOBJECTTYPEOBJECTSTRING));

    /* NULL path is valid */
    result = (NULL != object);
    if (result)
    {
        /* Destruct contents */
        result=_rt2dObjectStringDestruct(object);

        /* Destroy base object */
        RwFreeListFree(Rt2dGlobals.objectFreeList, object);
    }

    RWRETURN(result);
}

/**
 * \ingroup rt2dobjectstring
 * \ref Rt2dObjectStringGetBrush
 * Returns the brush owned by the object string
 *
 * \param object is the object string to get the brush from
 *
 * \return returns the brush if successful, NULL otherwise
 *
 * \see Rt2dObjectStringCreate
 * \see Rt2dObjectStringSetBrush
 */
Rt2dBrush *
Rt2dObjectStringGetBrush(Rt2dObject *object)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectStringGetBrush"));
    RWASSERT(object);
    RWASSERT(rt2DOBJECTTYPEOBJECTSTRING==(object->type));

    RWRETURN(object->data.objectString.brush);
}

/**
 * \ingroup rt2dobjectstring
 * \ref Rt2dObjectStringGetText
 * Returns the pointer to the text of the object string
 *
 * \param object is the object string to get the brush from
 *
 * \return returns the text if successful, NULL otherwise
 *
 * \see Rt2dObjectStringCreate
 * \see Rt2dObjectStringSetText
 * \see Rt2dObjectStringGetFont
 * \see Rt2dObjectStringSetFont
 */
RwChar *
Rt2dObjectStringGetText(Rt2dObject *object)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectStringGetText"));
    RWASSERT(object);
    RWASSERT(rt2DOBJECTTYPEOBJECTSTRING==(object->type));

    RWRETURN(object->data.objectString.textString);
}

/**
 * \ingroup rt2dobjectstring
 * \ref Rt2dObjectStringGetFont
 * Returns the pointer to the name of the font used
 * by the object string
 *
 * \param object is the object string to get the brush from
 *
 * \return returns the font if successful, NULL otherwise
 *
 * \see Rt2dObjectStringCreate
 * \see Rt2dObjectStringSetFont
 * \see Rt2dObjectStringGetText
 * \see Rt2dObjectStringSetText
 */
RwChar *
Rt2dObjectStringGetFont(Rt2dObject *object)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectStringGetFont"));
    RWASSERT(object);
    RWASSERT(rt2DOBJECTTYPEOBJECTSTRING==(object->type));

    RWRETURN(object->data.objectString.font->name);
}

/**
 * \ingroup rt2dobjectstring
 * \ref Rt2dObjectStringGetHeight
 * Returns the height used to render the object string
 *
 * \param object is the object string to get the brush from
 *
 * \return returns the height if successful
 *
 * \see Rt2dObjectStringCreate
 * \see Rt2dObjectStringSetHeight
 */
RwReal
Rt2dObjectStringGetHeight(Rt2dObject *object)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectStringGetHeight"));
    RWASSERT(object);
    RWASSERT(rt2DOBJECTTYPEOBJECTSTRING==(object->type));

    RWRETURN(object->data.objectString.height);
}

/**
 * \ingroup rt2dobjectstring
 * \ref Rt2dObjectStringSetBrush
 * Sets the brush used by the object string.
 *
 * \param object is the object string of which to change the text
 * \param brush is the brush that's copied into the string
 *
 * \return returns the object string if successful, NULL otherwise
 *
 * \see Rt2dObjectStringCreate
 * \see Rt2dObjectStringGetBrush
 * \see Rt2dObjectStringGetText
 * \see Rt2dObjectStringSetText
 */
Rt2dObject *
Rt2dObjectStringSetBrush(Rt2dObject *object, Rt2dBrush *brush)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectStringSetBrush"));
    RWASSERT(object);
    RWASSERT(rt2DOBJECTTYPEOBJECTSTRING==(object->type));
    RWASSERT(brush);

    memcpy(object->data.objectString.brush,brush, sizeof(Rt2dBrush));

    RWRETURN(object);
}

/**
 * \ingroup rt2dobjectstring
 * \ref Rt2dObjectStringSetText
 * Sets the text string within the object string.
 *
 * \param object is the object string of which to change the text
 * \param text is the text that's copied into the string
 *
 * \return returns the object string if successful, NULL otherwise
 *
 * \see Rt2dObjectStringCreate
 * \see Rt2dObjectStringGetText
 * \see Rt2dObjectStringGetFont
 * \see Rt2dObjectStringSetFont
 */
extern Rt2dObject *
Rt2dObjectStringSetText(Rt2dObject *object, const RwChar *text)
{
    Rt2dFont    *font;
    RwInt32 newSize;

    RWAPIFUNCTION(RWSTRING("Rt2dObjectStringSetText"));
    RWASSERT(object);
    RWASSERT(rt2DOBJECTTYPEOBJECTSTRING==(object->type));
    RWASSERT(text);

    /* Get the new text length */
    font = object->data.objectString.font->font;

    if ((font) && Rt2dFontIsUnicode(font))
    {
        newSize = wchar_strlen(text);

        /* if the length is insufficient, reallocation necessary */
        if (newSize > object->data.objectString.maxLength)
        {
            if( NULL != object->data.objectString.textString)
            {        
                RwFree(object->data.objectString.textString);
            }

            object->data.objectString.textString =
                (RwChar *)RwMalloc(newSize + 2,
                rwMEMHINTDUR_EVENT | rwID_2DOBJECTSTRING);
            RWASSERT(object->data.objectString.textString);

            object->data.objectString.maxLength = newSize;
        }

        /* replace text */
        memcpy(object->data.objectString.textString, text,
            (newSize + 2));
    }
    else
    {
        newSize = rwstrlen(text);

        /* if the length is insufficient, reallocation necessary */
        if (newSize > object->data.objectString.maxLength)
        {
            if( NULL != object->data.objectString.textString )
            {        
                RwFree(object->data.objectString.textString);
            }

            object->data.objectString.textString =
                (RwChar *)RwMalloc(newSize + 1,
                rwMEMHINTDUR_EVENT | rwID_2DOBJECTSTRING);
            RWASSERT(object->data.objectString.textString);
            object->data.objectString.maxLength = newSize;
        }

        /* replace text */
        rwstrcpy(object->data.objectString.textString, text);
    }

    RWRETURN(object);
}

/**
 * \ingroup rt2dobjectstring
 * \ref Rt2dObjectStringSetFont
 * Sets the font used to render the object string.
 *
 * \param object is the object string of which to change the font
 * \param font is the new font to be used
 *
 * \return returns the object string if successful, NULL otherwise
 *
 * \see Rt2dObjectStringCreate
 * \see Rt2dObjectStringGetFont
 * \see Rt2dObjectStringGetText
 * \see Rt2dObjectStringSetText
 */
extern Rt2dObject *
Rt2dObjectStringSetFont(Rt2dObject *object, const RwChar *font)
{
    _rt2dFontDictionaryNode *result;
    Rt2dObject *returnVal = object;
    RWAPIFUNCTION(RWSTRING("Rt2dObjectStringSetFont"));
    RWASSERT(object);
    RWASSERT(((object->type) == rt2DOBJECTTYPEOBJECTSTRING));
    RWASSERT(font);

    result = _rt2dFontDictionaryLookup(font);
    if (!result)
    {
        returnVal = (Rt2dObject *)NULL;
    }

    object->data.objectString.font = result;

    RWRETURN(returnVal);
}

/**
 * \ingroup rt2dobjectstring
 * \ref Rt2dObjectStringSetHeight
 * Sets the characters height used to render the object string.
 *
 * \param object is the object string of which to change the font
 * \param height is the new height to be used
 *
 * \return returns the object string if successful, NULL otherwise
 *
 * \see Rt2dObjectStringCreate
 * \see Rt2dObjectStringGetHeight
 */
extern Rt2dObject *
Rt2dObjectStringSetHeight(Rt2dObject *object, const RwReal height)
{
    RWAPIFUNCTION(RWSTRING("Rt2dObjectStringSetHeight"));
    RWASSERT(object);
    RWASSERT(((object->type) == rt2DOBJECTTYPEOBJECTSTRING));

    object->data.objectString.height = height;

    RWRETURN(object);
}

#define BrushSetup()                                                 \
MACRO_START                                                          \
{                                                                    \
    memcpy(&brushColors[0], &brush->bottom.col, sizeof(RwRGBAReal)); \
    memcpy(&brushColors[1], &brush->dbottom.col, sizeof(RwRGBAReal)); \
    memcpy(&brushColors[2], &brush->dtop.col, sizeof(RwRGBAReal));   \
    memcpy(&brushColors[3], &brush->top.col, sizeof(RwRGBAReal));    \
    _rt2dBrushTransformRGBARealDirect(                                \
                                brush,                                \
                                &brushColors[0],                      \
                                &brushColors[1],                      \
                                &brushColors[2],                      \
                                &brushColors[3],                      \
                                &multColor,                           \
                                &offsColor );                         \
}                                                                    \
MACRO_STOP

#define BrushUnsetup()                                                \
MACRO_START                                                           \
{                                                                     \
    memcpy(&brush->bottom.col,  &brushColors[0], sizeof(RwRGBAReal)); \
    memcpy(&brush->dbottom.col, &brushColors[1], sizeof(RwRGBAReal)); \
    memcpy(&brush->dtop.col, &brushColors[2], sizeof(RwRGBAReal));    \
    memcpy(&brush->top.col, &brushColors[3], sizeof(RwRGBAReal));     \
}                                                                     \
MACRO_STOP

/**
 * \ingroup rt2dobjectstring
 * \ref Rt2dObjectStringRender
 * Renders a string object.
 * If an object string is part of a scene, the \ref Rt2dSceneRender
 * renders all visible objects.
 *
 * \param object the string to be rendered
 *
 * \return returns the object string that was rendered
 *
 * \see Rt2dObjectStringCreate
 * \see Rt2dSceneRender
 */
Rt2dObject *
Rt2dObjectStringRender(Rt2dObject *object)
{
    RwRGBAReal multColor,offsColor;
    RwRGBAReal brushColors[4];
    Rt2dBrush  *brush;

    RWAPIFUNCTION(RWSTRING("Rt2dObjectStringRender"));
    RWASSERT(object);
    RWASSERT(object->type == rt2DOBJECTTYPEOBJECTSTRING);

    /* Object level stuff */
    if((object->flag & (Rt2dObjectVisible|Rt2dObjectStringGotNoFonts)) == Rt2dObjectVisible)
    {
        RwV2d anchor = {0.0, 0.0};

        if((object->flag & Rt2dObjectDirtyLTM) == Rt2dObjectDirtyLTM)
        {
            _rt2dCTMPush(&object->MTM);
            RwMatrixCopy(&object->LTM, _rt2dCTMGetDirect());

            object->flag &= ~Rt2dObjectDirtyLTM;

        }
        else
        {
            _rt2dCTMSet(&object->LTM);
        }

        memcpy(&multColor,&object->colorMult,sizeof(RwRGBAReal));

        memcpy(&offsColor,&object->colorOffs,sizeof(RwRGBAReal));

        brush = object->data.objectString.brush;

        BrushSetup();

        Rt2dFontShow (  object->data.objectString.font->font,
                        object->data.objectString.textString,
                        object->data.objectString.height,
                        &anchor,
                        brush );




        Rt2dCTMPop();

        BrushUnsetup();
    }

    RWRETURN(object);
}

Rt2dObject  *
_rt2dObjectStringStreamReadTo( Rt2dObject *object, RwStream *stream )
{
    RwUInt32 size, version;
    RwUInt32 stringLength;
    RwUInt32 fontLength;
    RwReal height;
    RwChar *stringBuffer = (RwChar *)NULL;
    RwChar *fontBuffer = (RwChar *)NULL;

    RWFUNCTION(RWSTRING("_rt2dObjectStringStreamReadTo"));
    RWASSERT(object);
    RWASSERT(rt2DOBJECTTYPEOBJECTSTRING == object->type);
    RWASSERT(stream);

    /* Read the string length and font height*/
    if (    (RwStreamRead(stream, &stringLength, sizeof(stringLength)) == sizeof(stringLength))
         && (RwStreamRead(stream, &height, sizeof(height)) == sizeof(height)))
    {
        /* Convert to machine format */
        (void)RwMemNative32(&stringLength, sizeof(stringLength));

        /* Convert  */
        (void)RwMemNative32(&height, sizeof(height));

        /* Convert1  */
        (void)RwMemFloat32ToReal(&height, sizeof(height));

        /* Get a buffer for the string */
        stringBuffer = (RwChar*)(RwMalloc(stringLength+2,
            rwMEMHINTDUR_FUNCTION | rwID_2DOBJECTSTRING));
        /* Read in the string and font length*/
        if (    stringBuffer
             && RwStreamRead(stream, stringBuffer, stringLength+1) == (stringLength+1)
             && RwStreamRead(stream, &fontLength, sizeof(fontLength)) == sizeof(fontLength))
        {
            RwBool noFontOrReadFontOk = FALSE;

            /* Make sure lastchar of stringBuffer is NULL to prevent overruns */
            stringBuffer[stringLength + 1] = 0;

            /* Convert to machine format */
            (void)RwMemNative32(&fontLength, sizeof(fontLength));

            if (fontLength)
            {
                /* Get a buffer for the font */
                fontBuffer = (RwChar*)(RwMalloc(fontLength+1,
                    rwMEMHINTDUR_FUNCTION | rwID_2DOBJECTSTRING));

                /* Read in the font */
                if (    fontBuffer
                    &&  (RwStreamRead(stream, fontBuffer, fontLength+1) == (fontLength+1))
                   )
                {
                    noFontOrReadFontOk = TRUE;
                }
            }
            else
            {
                noFontOrReadFontOk = TRUE;
            }

            /* Initialise the object string */
            if (    noFontOrReadFontOk
                 && _rt2dObjectStringInit(object, stringBuffer, fontBuffer)
                 && RwStreamFindChunk(stream, rwID_2DBRUSH, &size, &version)
                 && _rt2dBrushStreamReadTo( Rt2dObjectStringGetBrush(object), stream )
                )
            {
                object->data.objectString.height = height;

                /* Deallocate temporaries */
                RwFree(fontBuffer);
                RwFree(stringBuffer);

                /* Everything's worked OK */
                RWRETURN(object);
            }
            RwFree(fontBuffer);
        }
        RwFree(stringBuffer);
    }

    /* Didn't get an object */
    RWRETURN((Rt2dObject *)NULL);
}

/**
 * \ingroup rt2dobjectstring
 * \ref Rt2dObjectStringStreamRead
 * Reads an object string from a stream. Note that prior to this function
 * call, a binary shape chunk must be found in the stream using the
 * \ref RwStreamFindChunk API function.
 *
 * The sequence to locate and read an object string from a binary stream is
 * as follows:
 * \code
   RwStream *stream;
   Rt2dObject *new2dObjectString;

   stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "mybinary.xxx");
   if( stream )
   {
       if( RwStreamFindChunk(stream, rwID_2DOBJECTSTRING, NULL, NULL) )
       {
           new2dObjectString = Rt2dObjectStringStreamRead(stream);
       }

       RwStreamClose(stream, NULL);
   }
   \endcode
 *
 * \param stream is the stream to obtain the object string from
 *
 * \return returns the new object string
 *
 * \see Rt2dObjectStringStreamWrite
 * \see Rt2dObjectStringStreamGetSize
 * \see RwStreamOpen
 * \see RwStreamClose
 * \see RwStreamFindChunk
 */
Rt2dObject *
Rt2dObjectStringStreamRead(RwStream *stream)
{
    Rt2dObject *object = _rt2dObjectCreate();
    Rt2dObject *result = (Rt2dObject *)NULL;
    RWAPIFUNCTION(RWSTRING("Rt2dObjectStringStreamRead"));
    RWASSERT(stream);
    RWASSERT(object);

    _rt2dObjectInit(object, rt2DOBJECTTYPEOBJECTSTRING);

    /* Read base class information */
    if (!_rt2dObjectBaseStreamReadTo(object, stream))
    {
        RWRETURN((Rt2dObject*)NULL);
    }

    result = _rt2dObjectStringStreamReadTo(object, stream);

    if( NULL == result)
    {
        Rt2dObjectStringDestroy(object);
    }

    RWRETURN(result);
}


Rt2dObject *
_rt2dObjectStringStreamWrite(Rt2dObject *object, RwStream *stream)
{
    RwUInt32   length;
    RwReal     height;

    RWFUNCTION(RWSTRING("_rt2dObjectStringStreamWrite"));
    RWASSERT(object);
    RWASSERT(rt2DOBJECTTYPEOBJECTSTRING == object->type);
    RWASSERT(stream);

    /* Write base class information */
    if (!_rt2dObjectBaseStreamWrite(object, stream))
    {
        RWRETURN((Rt2dObject*)NULL);
    }

    /* Get the string length */
    if (Rt2dFontIsUnicode(object->data.objectString.font->font))
    {
        length = wchar_strlen(object->data.objectString.textString);
    }
    else
    {
        length = rwstrlen(object->data.objectString.textString);
    }

    /* Write length */
    if (!RwStreamWriteInt32(stream, (RwInt32 *)&length, sizeof(length)))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    /* Get the string height */
    height = object->data.objectString.height;

    /* Convert1  */
    (void)RwMemRealToFloat32(&height, sizeof(height));

    /* Convert  */
    (void)RwMemLittleEndian32(&height, sizeof(height));

    /* Write length*/
    if (!RwStreamWrite(stream, &height, sizeof(height)))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    /* Write string itself */
    if (!RwStreamWrite(stream, object->data.objectString.textString, length+1))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    /* Get the font name length */
    if(object->data.objectString.font != NULL)
    {
        length = rwstrlen(object->data.objectString.font->name);
    }
    else
    {
        length = 0;
    }

    /* Convert  */
    (void)RwMemLittleEndian32(&length, sizeof(length));

    /* Write length*/
    if (!RwStreamWrite(stream, &length, sizeof(length)))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    /* Write font name itself */
    if( NULL != object->data.objectString.font )
    {
        if (!RwStreamWrite(stream, object->data.objectString.font->name, length+1))
        {
            RWRETURN((Rt2dObject *)NULL);
        }
    }

    /* Write the brush info */
    if (!Rt2dBrushStreamWrite(object->data.objectString.brush, stream))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    RWRETURN(object);
}

/**
 * \ingroup rt2dobjectstring
 * \ref Rt2dObjectStringStreamWrite
 * Writes an object string to a stream. Note that the stream will have
 * been opened prior to this function call.
 *
 * \param object is the string to write to the stream
 * \param stream is the stream to write the object string to
 *
 * \return if successful, returns the object string written, otherwise NULL
 *
 * \see Rt2dObjectStringStreamGetSize
 * \see Rt2dObjectStringStreamRead
 * \see RwStreamOpen
 * \see RwStreamClose
 */
Rt2dObject *
Rt2dObjectStringStreamWrite(Rt2dObject *object, RwStream *stream)
{
    RwUInt32   size;

    RWAPIFUNCTION(RWSTRING("Rt2dObjectStringStreamWrite"));
    RWASSERT(object);
    RWASSERT(rt2DOBJECTTYPEOBJECTSTRING == object->type);
    RWASSERT(stream);

    /* Write header */
    size = Rt2dObjectStringStreamGetSize(object);
    if (!RwStreamWriteChunkHeader(stream, rwID_2DOBJECTSTRING, size))
    {
        RWRETURN((Rt2dObject *)NULL);
    }

    /* Write out body */
    _rt2dObjectStringStreamWrite(object, stream);

    RWRETURN(object);
}

RwUInt32
_rt2dObjectStringStreamGetSize(Rt2dObject *object)
{
    Rt2dFont    *font;
    RwUInt32 size;
    RWFUNCTION(RWSTRING("_rt2dObjectStringStreamGetSize"));
    RWASSERT(object);
    RWASSERT(rt2DOBJECTTYPEOBJECTSTRING == object->type);

    /* the size of the base class data */
    size = _rt2dObjectBaseStreamGetSize(object);

    /* ...plus the text length */
    size += sizeof(RwInt32);

    /* ...plus the font height */
    size += sizeof(RwReal);

    /* ...plus the size of the text */
    if (object->data.objectString.textString)
    {
        font = object->data.objectString.font->font;

        if ((font) && (Rt2dFontIsUnicode(font)))
        {
            size += wchar_strlen(object->data.objectString.textString);
        }
        else
        {
            size += rwstrlen(object->data.objectString.textString);
        }
    }

    /* ...plus the null for the text */
    size +=1;

    /* ...plus the font identification length */
    size += sizeof(RwInt32);

    /* ...plus the font identification */
    if( NULL != object->data.objectString.font )
    {
        if (object->data.objectString.font->name)
        {
            size += rwstrlen(object->data.objectString.font->name);
        }
    }

    /* ...plus the null for the font name */
    size +=1;

    RWRETURN(size);
}


/**
 * \ingroup rt2dobjectstring
 * \ref Rt2dObjectStringStreamGetSize is used to determine the size in bytes
 * of the binary representation of the given object string. This is used in
 * the binary chunk header to indicate the size of the chunk. The size does
 * include the size of the chunk header.
 *
 * \param object Pointer to the object.
 *
 * \return Returns a \ref RwUInt32 value equal to the chunk size (in bytes) of
 * the object string
 *
 * \see Rt2dObjectStringStreamRead
 * \see Rt2dObjectStringStreamWrite
 *
 */
RwUInt32
Rt2dObjectStringStreamGetSize(Rt2dObject *object)
{
    RwUInt32 size;
    RWAPIFUNCTION(RWSTRING("Rt2dObjectStringStreamGetSize"));
    RWASSERT(object);
    RWASSERT(rt2DOBJECTTYPEOBJECTSTRING == object->type);

    /* size of the chunk header... */
    size =  rwCHUNKHEADERSIZE;

    /* ...plus the size of the string body data */
    size += _rt2dObjectStringStreamGetSize(object);

    RWRETURN(size);
}


/****************************************************************************/

#if defined (__MWERKS__)
#if (defined(RWVERBOSE))
#pragma message (__FILE__ "/" _SKY_EXPAND(__LINE__) ": __MWERKS__ == " _SKY_EXPAND(__MWERKS__))
#endif /* (defined (__MWERKS__)) */
#if (__option (global_optimizer))
#pragma always_inline on
#endif /* (__option (global_optimizer)) */
#endif /*  defined (__MWERKS__) */
