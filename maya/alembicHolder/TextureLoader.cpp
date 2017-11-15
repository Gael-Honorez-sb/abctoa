#include "TextureLoader.h"
#include "Cache.h"
#include "Foundation.h"
#include <maya/MImage.h>
#include <maya/MGlobal.h>
#include <maya/MStatus.h>
#include <maya/MTextureManager.h>
#include <tbb/mutex.h>

namespace AlembicHolder {

MHWRender::MTextureManager* getTextureManager()
{
    auto renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer)
        return nullptr;
    return renderer->getTextureManager();
}

struct MTextureDeleter {
    void operator()(MHWRender::MTexture* ptr) const {
        auto texture_manager = getTextureManager();
        if (!texture_manager)
            return;
        texture_manager->releaseTexture(ptr);
    }
};

typedef Cache<FileRef, MHWRender::MTexture, std::hash<FileRef>, std::equal_to<FileRef>, MTextureDeleter> TextureCache;
TextureCache s_texture_cache;
tbb::mutex s_texture_cache_mutex;

TexturePtr loadTexture(const std::string& texture_path)
{
    if (texture_path.empty())
        return nullptr;

    const auto file_ref = FileRef(texture_path);

    tbb::mutex::scoped_lock guard(s_texture_cache_mutex);
    auto ptr = s_texture_cache.get(file_ref);
    if (ptr)
        return ptr;
    // Not in cache.
    auto texture_manager = getTextureManager();
    auto raw_ptr = texture_manager->acquireTexture(file_ref.path().c_str());
    if (!raw_ptr)
        return nullptr;
    return s_texture_cache.put(file_ref, std::move(raw_ptr));
}

static MGLFunctionTable *gGLFT = nullptr;

struct VP1TextureDeleter {
    void operator()(MGLuint* ptr) const
    {
        if (!ptr)
            return;
        if (!gGLFT)
            gGLFT = MHardwareRenderer::theRenderer()->glFunctionTable();
        gGLFT->glDeleteTextures(1, ptr);
        delete ptr;
    }
};

typedef Cache<FileRef, MGLuint, std::hash<FileRef>, std::equal_to<FileRef>, VP1TextureDeleter> VP1TextureCache;
VP1TextureCache s_vp1_texture_cache;
tbb::mutex s_vp1_texture_cache_mutex;

VP1TexturePtr loadVP1Texture(const std::string& texture_path)
{
    if (texture_path.empty())
        return nullptr;

    const auto file_ref = FileRef(texture_path);

    tbb::mutex::scoped_lock guard(s_vp1_texture_cache_mutex);
    auto ptr = s_vp1_texture_cache.get(file_ref);
    if (ptr)
        return ptr;

    // Not in cache.
    auto texture_ptr = std::unique_ptr<MGLuint>(new MGLuint);

    if (!gGLFT)
        gGLFT = MHardwareRenderer::theRenderer()->glFunctionTable();

    gGLFT->glGenTextures(1, texture_ptr.get());
    gGLFT->glBindTexture(MGL_TEXTURE_2D, *texture_ptr);
    MImage image;
    const MStatus status = image.readFromFile(file_ref.path().c_str());
    unsigned int width, height;
    image.getSize(width, height);

    if (!status) {
        MGlobal::displayWarning("In MTexture::load(), file not found:");
    }
    gGLFT->glTexImage2D(MGL_TEXTURE_2D, 0, MGL_RGBA8, width, height, 0, MGL_RGBA, MGL_UNSIGNED_BYTE, image.pixels());
    gGLFT->glTexParameteri(MGL_TEXTURE_2D, MGL_TEXTURE_MIN_FILTER, MGL_LINEAR);
    gGLFT->glTexParameteri(MGL_TEXTURE_2D, MGL_TEXTURE_MAG_FILTER, MGL_LINEAR);
    gGLFT->glTexParameteri(MGL_TEXTURE_2D, MGL_TEXTURE_WRAP_S, MGL_REPEAT);
    gGLFT->glTexParameteri(MGL_TEXTURE_2D, MGL_TEXTURE_WRAP_T, MGL_REPEAT);
    gGLFT->glBindTexture(MGL_TEXTURE_2D, 0); // deactivate the texture

    return s_vp1_texture_cache.put(file_ref, texture_ptr.release());
}

} // namespace AlembicHolder
