#pragma once

#include <maya/MGLdefinitions.h>
#include <memory>
#include <string>

namespace MHWRender {
class MTexture;
}

namespace AlembicHolder {

typedef std::shared_ptr<MHWRender::MTexture> TexturePtr;
TexturePtr loadTexture(const std::string& texture_path);

typedef std::shared_ptr<MGLuint> VP1TexturePtr;
VP1TexturePtr loadVP1Texture(const std::string& texture_path);

} // namespace AlembicHolder
