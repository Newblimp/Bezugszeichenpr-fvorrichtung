# MuPDFConfig.cmake - MuPDF Configuration Module
# This file contains the complete MuPDF build configuration
# It is kept separate from the submodule to allow version control

function(setup_mupdf)
    set(MUPDF_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/mupdf)

    # Configuration options - disable features we don't need to reduce build time/size
    set(MUPDF_DISABLE_XPS ON CACHE BOOL "Disable XPS support")
    set(MUPDF_DISABLE_SVG ON CACHE BOOL "Disable SVG support")
    set(MUPDF_DISABLE_CBZ ON CACHE BOOL "Disable CBZ support")
    set(MUPDF_DISABLE_HTML ON CACHE BOOL "Disable HTML support")
    set(MUPDF_DISABLE_EPUB ON CACHE BOOL "Disable EPUB support")
    set(MUPDF_DISABLE_JS ON CACHE BOOL "Disable JavaScript support")
    set(MUPDF_DISABLE_BROTLI ON CACHE BOOL "Disable Brotli support")
    set(MUPDF_DISABLE_ICC ON CACHE BOOL "Disable ICC color profiles")
    set(MUPDF_DISABLE_OCR ON CACHE BOOL "Disable OCR support")
    set(MUPDF_DISABLE_BARCODE ON CACHE BOOL "Disable barcode support")

    # ==============================================================================
    # ZLIB - Compression library
    # ==============================================================================
    set(ZLIB_SRC
        ${MUPDF_SOURCE_DIR}/thirdparty/zlib/adler32.c
        ${MUPDF_SOURCE_DIR}/thirdparty/zlib/compress.c
        ${MUPDF_SOURCE_DIR}/thirdparty/zlib/crc32.c
        ${MUPDF_SOURCE_DIR}/thirdparty/zlib/deflate.c
        ${MUPDF_SOURCE_DIR}/thirdparty/zlib/inffast.c
        ${MUPDF_SOURCE_DIR}/thirdparty/zlib/inflate.c
        ${MUPDF_SOURCE_DIR}/thirdparty/zlib/inftrees.c
        ${MUPDF_SOURCE_DIR}/thirdparty/zlib/trees.c
        ${MUPDF_SOURCE_DIR}/thirdparty/zlib/uncompr.c
        ${MUPDF_SOURCE_DIR}/thirdparty/zlib/zutil.c
    )

    add_library(mupdf_zlib STATIC ${ZLIB_SRC})
    target_include_directories(mupdf_zlib PUBLIC ${MUPDF_SOURCE_DIR}/thirdparty/zlib)
    if(MSVC)
        target_compile_definitions(mupdf_zlib PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()

    # ==============================================================================
    # LIBJPEG - JPEG image support
    # ==============================================================================
    set(LIBJPEG_SRC
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jaricom.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jcapimin.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jcapistd.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jcarith.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jccoefct.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jccolor.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jcdctmgr.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jchuff.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jcinit.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jcmainct.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jcmarker.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jcmaster.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jcomapi.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jcparam.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jcprepct.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jcsample.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jdapimin.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jdapistd.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jdarith.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jdatadst.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jdatasrc.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jdcoefct.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jdcolor.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jddctmgr.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jdhuff.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jdinput.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jdmainct.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jdmarker.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jdmaster.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jdmerge.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jdpostct.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jdsample.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jdtrans.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jerror.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jfdctflt.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jfdctfst.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jfdctint.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jidctflt.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jidctfst.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jidctint.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jmemmgr.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jquant1.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jquant2.c
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg/jutils.c
    )

    add_library(mupdf_libjpeg STATIC ${LIBJPEG_SRC})
    target_include_directories(mupdf_libjpeg PUBLIC
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg
        ${MUPDF_SOURCE_DIR}/scripts/libjpeg
    )
    if(MSVC)
        target_compile_definitions(mupdf_libjpeg PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()

    # ==============================================================================
    # FREETYPE2 - Font rendering
    # ==============================================================================
    set(FREETYPE_SRC
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/base/ftbase.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/base/ftbbox.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/base/ftbitmap.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/base/ftdebug.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/base/ftfstype.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/base/ftgasp.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/base/ftglyph.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/base/ftinit.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/base/ftstroke.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/base/ftsynth.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/base/ftsystem.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/base/fttype1.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/cff/cff.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/cid/type1cid.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/psaux/psaux.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/pshinter/pshinter.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/psnames/psnames.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/raster/raster.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/sfnt/sfnt.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/smooth/smooth.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/truetype/truetype.c
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/src/type1/type1.c
    )

    add_library(mupdf_freetype STATIC ${FREETYPE_SRC})
    target_include_directories(mupdf_freetype PUBLIC
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/include
        ${MUPDF_SOURCE_DIR}/scripts/freetype
    )
    target_compile_definitions(mupdf_freetype PRIVATE
        FT_CONFIG_MODULES_H="slimftmodules.h"
        FT_CONFIG_OPTIONS_H="slimftoptions.h"
        FT2_BUILD_LIBRARY
    )
    if(MSVC)
        target_compile_definitions(mupdf_freetype PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()

    # ==============================================================================
    # JBIG2DEC - JBIG2 image compression
    # ==============================================================================
    set(JBIG2DEC_SRC
        ${MUPDF_SOURCE_DIR}/thirdparty/jbig2dec/jbig2.c
        ${MUPDF_SOURCE_DIR}/thirdparty/jbig2dec/jbig2_arith.c
        ${MUPDF_SOURCE_DIR}/thirdparty/jbig2dec/jbig2_arith_iaid.c
        ${MUPDF_SOURCE_DIR}/thirdparty/jbig2dec/jbig2_arith_int.c
        ${MUPDF_SOURCE_DIR}/thirdparty/jbig2dec/jbig2_generic.c
        ${MUPDF_SOURCE_DIR}/thirdparty/jbig2dec/jbig2_halftone.c
        ${MUPDF_SOURCE_DIR}/thirdparty/jbig2dec/jbig2_huffman.c
        ${MUPDF_SOURCE_DIR}/thirdparty/jbig2dec/jbig2_hufftab.c
        ${MUPDF_SOURCE_DIR}/thirdparty/jbig2dec/jbig2_image.c
        ${MUPDF_SOURCE_DIR}/thirdparty/jbig2dec/jbig2_mmr.c
        ${MUPDF_SOURCE_DIR}/thirdparty/jbig2dec/jbig2_page.c
        ${MUPDF_SOURCE_DIR}/thirdparty/jbig2dec/jbig2_refinement.c
        ${MUPDF_SOURCE_DIR}/thirdparty/jbig2dec/jbig2_segment.c
        ${MUPDF_SOURCE_DIR}/thirdparty/jbig2dec/jbig2_symbol_dict.c
        ${MUPDF_SOURCE_DIR}/thirdparty/jbig2dec/jbig2_text.c
    )

    add_library(mupdf_jbig2dec STATIC ${JBIG2DEC_SRC})
    target_include_directories(mupdf_jbig2dec PUBLIC
        ${MUPDF_SOURCE_DIR}/thirdparty/jbig2dec
        ${MUPDF_SOURCE_DIR}/include
    )
    target_compile_definitions(mupdf_jbig2dec PRIVATE
        HAVE_STDINT_H
        JBIG_EXTERNAL_MEMENTO_H="mupdf/memento.h"
    )
    if(MSVC)
        target_compile_definitions(mupdf_jbig2dec PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()

    # ==============================================================================
    # OPENJPEG - JPEG2000 support
    # ==============================================================================
    set(OPENJPEG_SRC
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/bio.c
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/cio.c
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/dwt.c
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/event.c
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/function_list.c
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/ht_dec.c
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/image.c
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/invert.c
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/j2k.c
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/jp2.c
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/mct.c
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/mqc.c
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/openjpeg.c
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/pi.c
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/sparse_array.c
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/t1.c
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/t2.c
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/tcd.c
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/tgt.c
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2/thread.c
    )

    add_library(mupdf_openjpeg STATIC ${OPENJPEG_SRC})
    target_include_directories(mupdf_openjpeg PUBLIC
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2
    )
    target_compile_definitions(mupdf_openjpeg PRIVATE
        OPJ_STATIC
        OPJ_HAVE_INTTYPES_H
        OPJ_HAVE_STDINT_H
        MUTEX_pthread=0
    )
    if(MSVC)
        target_compile_definitions(mupdf_openjpeg PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()

    # ==============================================================================
    # HARFBUZZ - Text shaping engine
    # ==============================================================================
    set(HARFBUZZ_SRC
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/graph/gsubgpos-context.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-aat-layout.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-aat-map.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-blob.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-buffer.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-buffer-verify.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-buffer-serialize.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-common.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-face.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-fallback-shape.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-font.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ft.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-map.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-number.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-cff1-table.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-cff2-table.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-color.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-face.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-font.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-layout.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-map.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-math.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-meta.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-metrics.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-name.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-shape.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-shape-fallback.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-shape-normalize.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-shaper-arabic.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-shaper-default.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-shaper-hangul.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-shaper-hebrew.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-shaper-indic.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-shaper-indic-table.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-shaper-khmer.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-shaper-myanmar.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-shaper-syllabic.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-shaper-thai.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-shaper-use.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-shaper-vowel-constraints.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-tag.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ot-var.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-set.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-shape.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-shape-plan.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-shaper.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-static.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-subset.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-subset-cff1.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-subset-cff2.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-subset-cff-common.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-subset-input.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-subset-plan.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-ucd.cc
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src/hb-unicode.cc
    )

    add_library(mupdf_harfbuzz STATIC ${HARFBUZZ_SRC})
    target_include_directories(mupdf_harfbuzz PUBLIC
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src
        ${MUPDF_SOURCE_DIR}/include/mupdf
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/include
        ${MUPDF_SOURCE_DIR}/scripts/freetype
    )
    target_compile_definitions(mupdf_harfbuzz PRIVATE
        HAVE_FALLBACK=1
        HAVE_FREETYPE
        HAVE_OT
        HAVE_ROUND
        HAVE_UCDN
        HB_NO_MT
        HB_NO_PRAGMA_GCC_DIAGNOSTIC
        hb_malloc_impl=fz_hb_malloc
        hb_calloc_impl=fz_hb_calloc
        hb_free_impl=fz_hb_free
        hb_realloc_impl=fz_hb_realloc
    )
    if(MSVC)
        target_compile_definitions(mupdf_harfbuzz PRIVATE _CRT_SECURE_NO_WARNINGS)
        target_compile_options(mupdf_harfbuzz PRIVATE /w)
    else()
        target_compile_options(mupdf_harfbuzz PRIVATE -w -fno-exceptions -fno-rtti -fno-threadsafe-statics)
    endif()
    set_target_properties(mupdf_harfbuzz PROPERTIES CXX_STANDARD 11)

    # ==============================================================================
    # LCMS2 - Color management
    # ==============================================================================
    set(LCMS2_SRC
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmsalpha.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmscam02.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmscgats.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmscnvrt.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmserr.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmsgamma.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmsgmt.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmshalf.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmsintrp.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmsio0.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmsio1.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmslut.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmsmd5.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmsmtrx.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmsnamed.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmsopt.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmspack.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmspcs.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmsplugin.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmsps2.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmssamp.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmssm.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmstypes.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmsvirt.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmswtpnt.c
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/src/cmsxform.c
    )

    add_library(mupdf_lcms2 STATIC ${LCMS2_SRC})
    target_include_directories(mupdf_lcms2 PUBLIC
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/include
    )
    target_compile_definitions(mupdf_lcms2 PRIVATE
        HAVE_LCMS2MT
        LCMS2MT_PREFIX=lcms2mt_
    )
    if(MSVC)
        target_compile_definitions(mupdf_lcms2 PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()

    # ==============================================================================
    # MUPDF RESOURCES - Font and CMap resources
    # ==============================================================================
    file(GLOB MUPDF_RESOURCES_FONTS
        ${MUPDF_SOURCE_DIR}/generated/resources/fonts/urw/*.c
    )

    set(MUPDF_RESOURCES_SRC
        ${MUPDF_RESOURCES_FONTS}
    )

    if(MUPDF_RESOURCES_SRC)
        add_library(mupdf_resources STATIC ${MUPDF_RESOURCES_SRC})
        target_include_directories(mupdf_resources PUBLIC ${MUPDF_SOURCE_DIR}/include)
        if(MSVC)
            target_compile_definitions(mupdf_resources PRIVATE _CRT_SECURE_NO_WARNINGS)
            # Disable optimization for resource files (they're just data)
            target_compile_options(mupdf_resources PRIVATE /Od)
        endif()
    endif()

    # ==============================================================================
    # MUPDF CORE (FITZ) - The main MuPDF library
    # ==============================================================================
    file(GLOB MUPDF_FITZ_SRC
        ${MUPDF_SOURCE_DIR}/source/fitz/*.c
    )

    # Remove files we don't need (optional features that require extra dependencies)
    list(FILTER MUPDF_FITZ_SRC EXCLUDE REGEX ".*tessocr\\.c.*")
    list(FILTER MUPDF_FITZ_SRC EXCLUDE REGEX ".*leptonica-wrap\\.c.*")
    list(FILTER MUPDF_FITZ_SRC EXCLUDE REGEX ".*barcode\\.c.*")
    # Remove jmemcust.c as we're using shared JPEG (with wxWidgets' libjpeg)
    list(FILTER MUPDF_FITZ_SRC EXCLUDE REGEX ".*jmemcust\\.c.*")
    # Note: brotli.c and filter-brotli.c are included - they have stub fallbacks when FZ_ENABLE_BROTLI=0

    add_library(mupdf_fitz STATIC ${MUPDF_FITZ_SRC})
    target_include_directories(mupdf_fitz PUBLIC
        ${MUPDF_SOURCE_DIR}/include
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/include
        ${MUPDF_SOURCE_DIR}/scripts/freetype
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg
        ${MUPDF_SOURCE_DIR}/scripts/libjpeg
        ${MUPDF_SOURCE_DIR}/thirdparty/jbig2dec
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2
        ${MUPDF_SOURCE_DIR}/thirdparty/zlib
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/include
    )

    target_compile_definitions(mupdf_fitz PRIVATE
        # Disable features we don't need
        FZ_ENABLE_XPS=0
        FZ_ENABLE_SVG=0
        FZ_ENABLE_CBZ=0
        FZ_ENABLE_IMG=0
        FZ_ENABLE_HTML=0
        FZ_ENABLE_EPUB=0
        FZ_ENABLE_FB2=0
        FZ_ENABLE_MOBI=0
        FZ_ENABLE_TXT=0
        FZ_ENABLE_OFFICE=0
        FZ_ENABLE_JS=0
        FZ_ENABLE_BROTLI=0
        FZ_ENABLE_BARCODE=0
        FZ_ENABLE_OCR_OUTPUT=0
        FZ_ENABLE_DOCX_OUTPUT=0
        FZ_ENABLE_ODT_OUTPUT=0
        FZ_ENABLE_HYPHEN=0
        FZ_ENABLE_HYPHEN_ALL=0
        FZ_ENABLE_HTML_ENGINE=0
        OCR_DISABLED
        # Keep PDF and basic features
        FZ_ENABLE_PDF=1
        FZ_ENABLE_JPX=1
        FZ_ENABLE_ICC=1
        # Share libjpeg with wxWidgets to avoid symbol conflicts
        SHARE_JPEG=1
        # LCMS2 configuration
        HAVE_LCMS2MT
        LCMS2MT_PREFIX=lcms2mt_
        # OpenJPEG configuration
        OPJ_STATIC
        # MuPDF memory handling
        MEMENTO_MUPDF_HACKS
        # Disable fonts we don't need
        TOFU
        TOFU_CJK
        TOFU_CJK_EXT
        TOFU_CJK_LANG
        TOFU_EMOJI
        TOFU_HISTORIC
        TOFU_SYMBOL
        TOFU_SIL
    )

    if(MSVC)
        target_compile_definitions(mupdf_fitz PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()

    # ==============================================================================
    # MUPDF PDF - PDF document handling
    # ==============================================================================
    file(GLOB MUPDF_PDF_SRC
        ${MUPDF_SOURCE_DIR}/source/pdf/*.c
    )

    add_library(mupdf_pdf STATIC ${MUPDF_PDF_SRC})
    target_include_directories(mupdf_pdf PUBLIC
        ${MUPDF_SOURCE_DIR}/include
        ${MUPDF_SOURCE_DIR}/thirdparty/freetype/include
        ${MUPDF_SOURCE_DIR}/scripts/freetype
        ${MUPDF_SOURCE_DIR}/thirdparty/harfbuzz/src
        ${MUPDF_SOURCE_DIR}/thirdparty/libjpeg
        ${MUPDF_SOURCE_DIR}/scripts/libjpeg
        ${MUPDF_SOURCE_DIR}/thirdparty/jbig2dec
        ${MUPDF_SOURCE_DIR}/thirdparty/openjpeg/src/lib/openjp2
        ${MUPDF_SOURCE_DIR}/thirdparty/zlib
        ${MUPDF_SOURCE_DIR}/thirdparty/lcms2/include
    )

    target_compile_definitions(mupdf_pdf PRIVATE
        FZ_ENABLE_XPS=0
        FZ_ENABLE_SVG=0
        FZ_ENABLE_CBZ=0
        FZ_ENABLE_IMG=0
        FZ_ENABLE_HTML=0
        FZ_ENABLE_EPUB=0
        FZ_ENABLE_FB2=0
        FZ_ENABLE_MOBI=0
        FZ_ENABLE_TXT=0
        FZ_ENABLE_OFFICE=0
        FZ_ENABLE_JS=0
        FZ_ENABLE_BROTLI=0
        FZ_ENABLE_BARCODE=0
        FZ_ENABLE_OCR_OUTPUT=0
        FZ_ENABLE_DOCX_OUTPUT=0
        FZ_ENABLE_ODT_OUTPUT=0
        FZ_ENABLE_HYPHEN=0
        FZ_ENABLE_HYPHEN_ALL=0
        FZ_ENABLE_HTML_ENGINE=0
        OCR_DISABLED
        FZ_ENABLE_PDF=1
        FZ_ENABLE_JPX=1
        FZ_ENABLE_ICC=1
        HAVE_LCMS2MT
        LCMS2MT_PREFIX=lcms2mt_
        OPJ_STATIC
        TOFU
        TOFU_CJK
        TOFU_CJK_EXT
        TOFU_CJK_LANG
        TOFU_EMOJI
        TOFU_HISTORIC
        TOFU_SYMBOL
        TOFU_SIL
    )

    if(MSVC)
        target_compile_definitions(mupdf_pdf PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()

    # ==============================================================================
    # COMBINED MUPDF LIBRARY - Single library target for easier linking
    # ==============================================================================
    add_library(mupdf INTERFACE)
    # Handle circular dependency between mupdf_fitz and mupdf_pdf
    # by listing them twice (traditional solution for circular deps in static libs)
    target_link_libraries(mupdf INTERFACE
        mupdf_fitz
        mupdf_pdf
        mupdf_fitz
        mupdf_pdf
        mupdf_harfbuzz
        mupdf_freetype
        mupdf_lcms2
        mupdf_openjpeg
        mupdf_jbig2dec
        # Note: We don't link mupdf_libjpeg here - we use wxWidgets' libjpeg (wxjpeg)
        # to avoid symbol conflicts. SHARE_JPEG=1 is set to enable this.
        mupdf_zlib
    )

    # Add resources if available
    if(TARGET mupdf_resources)
        target_link_libraries(mupdf INTERFACE mupdf_resources)
    endif()

    # Include directory for consumers
    target_include_directories(mupdf INTERFACE ${MUPDF_SOURCE_DIR}/include)

    # Compile definitions for consumers
    target_compile_definitions(mupdf INTERFACE
        FZ_ENABLE_XPS=0
        FZ_ENABLE_SVG=0
        FZ_ENABLE_CBZ=0
        FZ_ENABLE_IMG=0
        FZ_ENABLE_HTML=0
        FZ_ENABLE_EPUB=0
        FZ_ENABLE_FB2=0
        FZ_ENABLE_MOBI=0
        FZ_ENABLE_TXT=0
        FZ_ENABLE_OFFICE=0
        FZ_ENABLE_JS=0
        FZ_ENABLE_BROTLI=0
        FZ_ENABLE_BARCODE=0
        FZ_ENABLE_OCR_OUTPUT=0
        FZ_ENABLE_DOCX_OUTPUT=0
        FZ_ENABLE_ODT_OUTPUT=0
        FZ_ENABLE_HYPHEN=0
        FZ_ENABLE_HTML_ENGINE=0
        OCR_DISABLED
        FZ_ENABLE_PDF=1
        FZ_ENABLE_JPX=1
        FZ_ENABLE_ICC=1
        HAVE_LCMS2MT
        LCMS2MT_PREFIX=lcms2mt_
        OPJ_STATIC
        TOFU
        TOFU_CJK
    )

endfunction()
