#pragma once

#include <cstdint>
#include <memory>

class RenderTexture {
public:
    RenderTexture(uint32_t width, uint32_t height);
    ~RenderTexture();

    void begin();
    void end();

    [[nodiscard]] std::unique_ptr<uint8_t[]> getData() const;

private:
    uint32_t m_width, m_height;
    int32_t m_oldFBO = -1;
    uint32_t m_fbo = 0;
    uint32_t m_texture = 0;
    cocos2d::CCSize m_oldScale{};
    cocos2d::CCSize m_oldResolution{};
    cocos2d::CCSize m_oldScreenSize{};
};