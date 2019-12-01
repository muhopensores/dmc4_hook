#ifndef HACKLIB_DRAWERD3D_H
#define HACKLIB_DRAWERD3D_H

#include "d3d9.h"
#include "d3dx9.h"
#include "hacklib/IDrawer.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>


namespace hl
{
/// Vertex structure for transformed colorizable verts.
struct VERTEX_2D_COL
{
    float x, y, z, rhw;
    static const DWORD FVF = D3DFVF_XYZRHW;
};
/// Vertex structure for transformed colorized verts.
struct VERTEX_2D_DIF
{
    float x, y, z, rhw;
    D3DCOLOR color;
    static const DWORD FVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE;
};
/// Vertex strucutre for transformed texture mapped verts.
struct VERTEX_2D_TEX
{
    float x, y, z, rhw;
    float u, v;
    static const DWORD FVF = D3DFVF_XYZRHW | D3DFVF_TEX1;
};
/// Vertex structure for transformable colorizable verts.
struct VERTEX_3D_COL
{
    float x, y, z;
    static const DWORD FVF = D3DFVF_XYZ;
};
/// Vertex structure for transformable colorized verts.
struct VERTEX_3D_DIF
{
    float x, y, z;
    D3DCOLOR color;
    static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
};


/// Represents a prerendered set of glyphs of a font.
class Font
{
    friend class DrawerD3D;

public:
    Font(ID3DXFont* d3dobj) : m_pFont(d3dobj) {}
    ~Font();

private:
    ID3DXFont* m_pFont;
};
/// Represents a loaded texture.
class Texture
{
    friend class DrawerD3D;

public:
    Texture(IDirect3DTexture9* d3dobj) : m_pTexture(d3dobj) {}
    ~Texture();

private:
    IDirect3DTexture9* m_pTexture;
};
/// Represents a loaded vertex buffer.
class VertexBuffer
{
    friend class DrawerD3D;

public:
    VertexBuffer(DWORD format, IDirect3DVertexBuffer9* d3dobj, unsigned int verts)
        : m_format(format), m_pVertexBuffer(d3dobj), m_numVertices(verts)
    {
    }
    ~VertexBuffer();

private:
    DWORD m_format;
    IDirect3DVertexBuffer9* m_pVertexBuffer;
    unsigned int m_numVertices;
};
/// Represents a loaded index buffer.
class IndexBuffer
{
    friend class DrawerD3D;

public:
    IndexBuffer(DWORD format, IDirect3DIndexBuffer9* d3dobj, unsigned int indices)
        : m_format(format), m_pIndexBuffer(d3dobj), m_numIndices(indices)
    {
    }
    ~IndexBuffer();

private:
    DWORD m_format;
    IDirect3DIndexBuffer9* m_pIndexBuffer;
    unsigned int m_numIndices;
};
/// Represents a loaded sprite.
class Sprite
{
    friend class DrawerD3D;

public:
    Sprite(ID3DXSprite* d3dobj) : m_pSprite(d3dobj) {}
    void SetTransform(const D3DXMATRIX& transform);
    bool Begin();
    void End();
    ~Sprite();

private:
    ID3DXSprite* m_pSprite;
};


/**
 * \brief Drawer implementation for Direct3D 9.
 */
class DrawerD3D : public hl::IDrawer
{
public:
    void onLostDevice() override;
    void onResetDevice() override;

    void drawLine(float x1, float y1, float x2, float y2, hl::Color color) const override;
    void drawRect(float x, float y, float w, float h, hl::Color color) const override;
    void drawRectFilled(float x, float y, float w, float h, hl::Color color) const override;
    void drawCircle(float mx, float my, float r, hl::Color color) const override;
    void drawCircleFilled(float mx, float my, float r, hl::Color color) const override;

    /// Allocates a hl::Font object.
    /// \param fontname The name of the font to load.
    /// \param size The size of the font.
    /// \param bold Set to false to load a non-bold font.
    /// \return A Font object or nullptr.
    const Font* allocFont(std::string fontname, int size, bool bold = true);
    /// Draws text at the screen coordinates x,y with color.
    void drawFont(const Font* pFont, float x, float y, hl::Color color, std::string format, va_list valist) const;
    /// \overload
    void drawFont(const Font* pFont, float x, float y, hl::Color color, std::string format, ...) const;
    /// Returns the size in screen coordinates that the given text would occupy with the given hl::Font.
    hl::Vec2 getTextSize(const Font* pFont, std::string str) const;
    /// Releases the given hl::Font.
    void releaseFont(const Font* pFont);

    /// Allocates a hl::Texture object.
    const Texture* allocTexture(std::string filename);
    /// \overload
    const Texture* allocTexture(const void* buffer, size_t size);
    /// Draws the texture at the screen coordinates x,y with width w and height h.
    void drawTexture(const Texture* pTexture, float x, float y, float w, float h) const;
    /// Releases the given hl::Texture.
    void releaseTexture(const Texture* pTexture);

    /// Allocates a hl::VectexBuffer object.
    /// \tparam T The type of the vertex structure. Use the predefined hl::VERTEX_ structures.
    /// \param vertices A vector containing the vertex data.
    /// \return A hl::VertexBuffer or nullptr.
    template <class T>
    const VertexBuffer* allocVertexBuffer(const std::vector<T>& vertices);
    /// Allocates a hl::IndexBuffer.
    const IndexBuffer* allocIndexBuffer(const std::vector<unsigned int>& indices);
    /// Draws a primitive. The IndexBuffer is optional. See d3d documentation for primitive types.
    void drawPrimitive(const VertexBuffer* pVertBuf, const IndexBuffer* pIndBuf, D3DPRIMITIVETYPE type,
                       const D3DXMATRIX& worldMatrix) const;
    /// \overload
    void drawPrimitive(const VertexBuffer* pVertBuf, const IndexBuffer* pIndBuf, D3DPRIMITIVETYPE type,
                       const D3DXMATRIX& worldMatrix, D3DCOLOR color) const;
    /// Releases the given hl::VertexBuffer.
    void releaseVertexBuffer(const VertexBuffer* pVertBuf);
    /// Releases the given hl::IndexBuffer.
    void releaseIndexBuffer(const IndexBuffer* pIndBuf);

    /// Allocates a hl::Sprite object.
    const Sprite* allocSprite();
    /// Releases the given hl::Sprite.
    void releaseSprite(const Sprite* pSprite);

private:
    template <typename T, typename... Ts>
    const T* alloc(std::vector<std::unique_ptr<T>>& container, Ts&&... vs);
    template <typename T>
    void release(std::vector<std::unique_ptr<T>>& container, const T* instance);

protected:
    void updateDimensions() override;

protected:
    std::vector<std::unique_ptr<Font>> m_fonts;
    std::vector<std::unique_ptr<Texture>> m_textures;
    std::vector<std::unique_ptr<VertexBuffer>> m_vertexBuffers;
    std::vector<std::unique_ptr<IndexBuffer>> m_indexBuffers;
    std::vector<std::unique_ptr<Sprite>> m_sprites;
};


template <typename T, typename... Ts>
const T* DrawerD3D::alloc(std::vector<std::unique_ptr<T>>& container, Ts&&... vs)
{
    auto resource = std::make_unique<T>(std::forward<Ts>(vs)...);

    auto result = resource.get();
    container.push_back(std::move(resource));
    return result;
}


template <typename T>
void DrawerD3D::release(std::vector<std::unique_ptr<T>>& container, const T* instance)
{
    container.erase(std::remove_if(container.begin(), container.end(),
                                   [instance](const auto& uptr) { return uptr.get() == instance; }),
                    container.end());
}


template <class T>
const VertexBuffer* DrawerD3D::allocVertexBuffer(const std::vector<T>& vertices)
{
    IDirect3DVertexBuffer9* pd3dVertexBuffer;
    if (m_context->CreateVertexBuffer(static_cast<UINT>(vertices.size() * sizeof(T)), 0, T::FVF, D3DPOOL_MANAGED,
                                      &pd3dVertexBuffer, NULL) != D3D_OK)
        return nullptr;

    T* pData = nullptr;
    pd3dVertexBuffer->Lock(0, 0, reinterpret_cast<void**>(&pData), 0);
    memcpy(pData, vertices.data(), vertices.size() * sizeof(T));
    pd3dVertexBuffer->Unlock();

    return alloc(m_vertexBuffers, T::FVF, pd3dVertexBuffer, static_cast<unsigned int>(vertices.size()));
}
}

#endif
